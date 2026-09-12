#!/usr/bin/env python3
"""
build_loader.py - Generates boottrs.bin for booting Daniel Paul Martin's
TRS-OS (TRSDOS 6.3.1 for eZ80) on the Olimex Agon Light 2.

How it works:
1. MOS loads trsos.dat (480 KB) to external SRAM at &40000 (&040000 - &0B7FFF).
2. MOS loads boottrs.bin to external SRAM at &B8000 and jumps to it (in ADL=1 mode).
3. The launcher enables the eZ80F92 on-chip 8 KB SRAM at 0xFFE000.
4. It sets the stack pointer to 0xFFFFFF (inside on-chip SRAM).
5. It copies the relocation stub to on-chip SRAM (0xFFE000) and jumps to it.
6. Running safely from on-chip SRAM, the stub:
   a. Disables all eZ80 hardware timers (TMR0-TMR5) and clears pending flags.
   b. Disables UART0 and UART1 interrupt enables.
   c. Disables GPIO interrupts (PB_ALT1, PC_ALT1, PD_ALT1).
   d. Remaps on-chip Flash out of the way to 0x200000 (FLASH_ADDR_U = 0x20).
   e. Remaps external 512 KB SRAM CS0 to 0x000000 - 0x07FFFF (CS0_LBR=0x00, CS0_UBR=0x07).
      This shifts trsos.dat from 0x040000 down to 0x000000!
   f. Relocates the eZ80F92 interrupt vector table to 0x00F600 (96 bytes).
   g. Sets the CPU I register to 0xF6.
   h. Patches STRAY.handler at 0x00F786 with 'EI; RETI' (FB ED 4D) so stray IRQs return cleanly.
   i. Sets UART0_SPR to 0x03 so TRS-OS identifies the platform as AGON.
   j. Executes RSMIX (ED 7E) to reset Mixed-Memory Mode (MADL = 0).
   k. Clears registers BC, DE, HL to 0.
   l. Executes JP.SIS 000000h (40 C3 00 00) to switch to ADL=0 (Z80 mode) and jump to 0x000000.
"""

import sys
from pathlib import Path

def build_loader():
    # Relocation stub executed from on-chip SRAM at 0xFFE000 (in ADL=1 mode)
    stub = bytearray([
        0xF3,                   # DI

        # Disable all timers (TMR0 to TMR5)
        0xAF,                   # XOR A
        0xED, 0x39, 0x80,       # OUT0 (80h), A  ; TMR0_CTL = 0
        0xED, 0x39, 0x83,       # OUT0 (83h), A  ; TMR1_CTL = 0
        0xED, 0x39, 0x86,       # OUT0 (86h), A  ; TMR2_CTL = 0
        0xED, 0x39, 0x89,       # OUT0 (89h), A  ; TMR3_CTL = 0
        0xED, 0x39, 0x8C,       # OUT0 (8Ch), A  ; TMR4_CTL = 0
        0xED, 0x39, 0x8F,       # OUT0 (8Fh), A  ; TMR5_CTL = 0

        # Disable UART0 and UART1 interrupts
        0xED, 0x39, 0xC1,       # OUT0 (C1h), A  ; UART0_IER = 0
        0xED, 0x39, 0xD1,       # OUT0 (D1h), A  ; UART1_IER = 0

        # Disable GPIO interrupts (PB_ALT1, PC_ALT1, PD_ALT1)
        0xED, 0x39, 0x9C,       # OUT0 (9Ch), A  ; PB_ALT1 = 0
        0xED, 0x39, 0xA0,       # OUT0 (A0h), A  ; PC_ALT1 = 0
        0xED, 0x39, 0xA4,       # OUT0 (A4h), A  ; PD_ALT1 = 0

        # Read timers to clear any pending interrupt flags
        0xED, 0x38, 0x80,       # IN0 A, (80h)
        0xED, 0x38, 0x83,       # IN0 A, (83h)
        0xED, 0x38, 0x86,       # IN0 A, (86h)
        0xED, 0x38, 0x89,       # IN0 A, (89h)
        0xED, 0x38, 0x8C,       # IN0 A, (8Ch)
        0xED, 0x38, 0x8F,       # IN0 A, (8Fh)

        # Move on-chip Flash to 0x200000
        0x3E, 0x20,             # LD A, 20h
        0xED, 0x39, 0xF7,       # OUT0 (F7h), A  ; FLASH_ADDR_U = 20h

        # Map external SRAM (512 KB) to 000000h - 07FFFFh
        0x3E, 0x00,             # LD A, 00h
        0xED, 0x39, 0xA8,       # OUT0 (A8h), A  ; CS0_LBR = 00h
        0x3E, 0x07,             # LD A, 07h
        0xED, 0x39, 0xA9,       # OUT0 (A9h), A  ; CS0_UBR = 07h

        # Copy F92 interrupt vector table to 00F600h (96 bytes)
        0x21, 0x85, 0x8A, 0x00, # LD HL, 008A85h
        0x11, 0x00, 0xF6, 0x00, # LD DE, 00F600h
        0x01, 96,   0x00, 0x00, # LD BC, 96
        0xED, 0xB0,             # LDIR

        # Set I register to F6h
        0x3E, 0xF6,             # LD A, F6h
        0xED, 0x47,             # LD I, A

        # Patch STRAY.handler at 00F786h with "EI; RETI" (FB ED 4D) so stray IRQs return cleanly
        0x21, 0x86, 0xF7, 0x00, # LD HL, 00F786h
        0x36, 0xFB,             # LD (HL), FBh (EI)
        0x23,                   # INC HL
        0x36, 0xED,             # LD (HL), EDh
        0x23,                   # INC HL
        0x36, 0x4D,             # LD (HL), 4Dh (RETI)

        # Set UART0_SPR to 3 (identifies platform as AGON to TRS-OS)
        0x3E, 0x03,             # LD A, 03h
        0xED, 0x39, 0xC7,       # OUT0 (C7h), A  ; UART0_SPR = 03h

        # Reset Mixed-Memory Mode: RSMIX sets MADL = 0 (essential for Z80 interrupt operation)
        0xED, 0x7E,             # RSMIX

        # Clear registers BC, DE, HL
        0xAF,                   # XOR A
        0x01, 0x00, 0x00, 0x00, # LD BC, 0
        0x11, 0x00, 0x00, 0x00, # LD DE, 0
        0x21, 0x00, 0x00, 0x00, # LD HL, 0

        # Switch CPU to ADL=0 (Z80 mode) and jump to 000000h
        0x40, 0xC3, 0x00, 0x00  # JP.SIS 0000h
    ])

    # Launcher header executed at 0x0B8000 by MOS JMP &B8000 (ADL=1)
    stub_addr = 0x0B8000 + 32
    launcher = bytearray([
        0x3E, 0x80,             # LD A, 80h
        0xED, 0x39, 0xB4,       # OUT0 (B4h), A ; Enable internal RAM
        0x3E, 0xFF,             # LD A, FFh
        0xED, 0x39, 0xB5,       # OUT0 (B5h), A ; Set RAM address upper segment to FFh
        0x31, 0xFF, 0xFF, 0xFF, # LD SP, 0FFFFFFh (stack into on-chip RAM)
        0x21, stub_addr & 0xFF, (stub_addr >> 8) & 0xFF, (stub_addr >> 16) & 0xFF, # LD HL, stub_addr
        0x11, 0x00, 0xE0, 0xFF, # LD DE, 0FFE000h
        0x01, len(stub), 0x00, 0x00, # LD BC, len(stub)
        0xED, 0xB0,             # LDIR
        0xC3, 0x00, 0xE0, 0xFF  # JP 0FFE000h
    ])
    assert len(launcher) == 32, f"Header length must be 32, got {len(launcher)}"
    launcher += stub
    return launcher

if __name__ == "__main__":
    out_dir = Path(__file__).resolve().parent
    out_file = out_dir / "boottrs.bin"
    binary = build_loader()
    out_file.write_bytes(binary)
    print(f"Built {out_file} ({len(binary)} bytes)")

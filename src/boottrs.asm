; ==============================================================================
; boottrs.asm - Agon MOS Launcher & Hardware Handover for TRS-OS
;
; Target Platform : Olimex Agon Light 2 / Agon Family (Zilog eZ80F92)
; Target Address  : &0B8000 (loaded into external SRAM by Quark MOS)
; Initial CPU Mode: ADL=1 (24-bit linear addressing)
; Final CPU Mode  : ADL=0, MADL=0 (16-bit Z80 compatibility mode at 0x000000)
; Output Binary   : boottrs.bin (154 bytes)
;
; Assembles using ez80asm (the canonical Agon assembler):
;   ez80asm src/boottrs.asm boottrs.bin
;
; How It Works:
; 1. Quark MOS loads trsos.dat (480 KB) to external SRAM at &40000.
; 2. Quark MOS loads boottrs.bin to external SRAM at &B8000.
; 3. User (or autoexec.txt) executes: JMP &B8000
; 4. The launcher header enables on-chip 8 KB SRAM at 0xFFE000 and sets SP to 0xFFFFFF.
; 5. It copies the relocation stub to on-chip SRAM (0xFFE000) and jumps to it.
; 6. Running safely from on-chip SRAM (so external SRAM can be unmapped):
;    a. Disables eZ80 timers (TMR0-TMR5) and clears pending interrupt flags.
;    b. Disables UART0 and UART1 interrupts.
;    c. Disables GPIO interrupts (PB_ALT1, PC_ALT1, PD_ALT1).
;    d. Remaps on-chip Flash to 0x200000 (FLASH_ADDR_U = 0x20).
;    e. Remaps external 512 KB SRAM CS0 to 0x000000-0x07FFFF (CS0_LBR=0x00, CS0_UBR=0x07).
;       This shifts trsos.dat from 0x040000 down to 0x000000!
;    f. Relocates eZ80F92 interrupt vector table to 0x00F600 (96 bytes).
;    g. Sets CPU I register = 0xF6.
;    h. Patches STRAY.handler at 0x00F786 with "EI; RETI" (FB ED 4D) so stray IRQs return cleanly.
;    i. Sets UART0_SPR = 0x03 (identifies platform as AGON to TRS-OS).
;    j. Resets Mixed-Memory Mode: RSMIX (MADL = 0).
;    k. Clears registers BC, DE, HL to 0.
;    l. Executes JP.SIS 0000h to switch to ADL=0 (Z80 mode) and jump to 0x000000.
; ==============================================================================

    ORG $0B8000
    ASSUME ADL = 1

    INCLUDE "ez80f92.inc"

; ------------------------------------------------------------------------------
; Application Memory Layout & Equates
; ------------------------------------------------------------------------------
INTERNAL_SRAM:  EQU $FFE000         ; Base address of 8 KB on-chip SRAM (segment 0xFF)
STACK_TOP:      EQU $FFFFFF         ; Top of on-chip SRAM for temporary stack

IVT_SRC:        EQU $008A85         ; Source of F92 IVT inside loaded TRS-OS image
IVT_DEST:       EQU $00F600         ; Target destination of F92 IVT in low memory
IVT_SIZE:       EQU 96              ; Size of eZ80 interrupt vector table (bytes)
IVT_VEC_I:      EQU $F6             ; Vector base value for CPU I register
STRAY_HANDLER:  EQU $00F786         ; Address of STRAY.handler in TRS-OS

; ==============================================================================
; Stage 1: Launcher Header
; Executed from external SRAM at 0x0B8000 by MOS command: JMP &B8000
; ==============================================================================

launcher:
    ; Enable eZ80 on-chip 8 KB SRAM at 0xFFE000
    LD A, $80
    OUT0 (RAM_CTL), A
    LD A, $FF
    OUT0 (RAM_ADDR_U), A

    ; Switch stack pointer to top of on-chip SRAM
    LD SP, STACK_TOP

    ; Copy relocation stub to on-chip SRAM
    LD HL, stub
    LD DE, INTERNAL_SRAM
    LD BC, stub_len
    LDIR

    ; Jump into relocation stub running in on-chip SRAM
    JP INTERNAL_SRAM

; ==============================================================================
; Stage 2: Relocation Stub
; Copied to and executed from on-chip SRAM (0xFFE000)
; ==============================================================================

stub:
    ; Disable maskable interrupts during hardware reconfiguration
    DI

    ; Disable all hardware timers (TMR0 to TMR5)
    XOR A, A
    OUT0 (TMR0_CTL), A
    OUT0 (TMR1_CTL), A
    OUT0 (TMR2_CTL), A
    OUT0 (TMR3_CTL), A
    OUT0 (TMR4_CTL), A
    OUT0 (TMR5_CTL), A

    ; Disable UART0 and UART1 interrupt generation
    OUT0 (UART0_IER), A
    OUT0 (UART1_IER), A

    ; Disable GPIO pin interrupts
    OUT0 (PB_ALT1), A
    OUT0 (PC_ALT1), A
    OUT0 (PD_ALT1), A

    ; Read timer control registers to clear any pending interrupt flags
    IN0 A, (TMR0_CTL)
    IN0 A, (TMR1_CTL)
    IN0 A, (TMR2_CTL)
    IN0 A, (TMR3_CTL)
    IN0 A, (TMR4_CTL)
    IN0 A, (TMR5_CTL)

    ; Relocate internal Flash out of the way to 0x200000 - 0x21FFFF
    LD A, $20
    OUT0 (FLASH_ADDR_U), A

    ; Remap 512 KB external SRAM CS0 to 0x000000 - 0x07FFFF
    ; This places the trsos.dat image (loaded at 0x040000) directly at address 0x000000
    LD A, $00
    OUT0 (CS0_LBR), A
    LD A, $07
    OUT0 (CS0_UBR), A

    ; Copy eZ80F92 interrupt vector table to 0x00F600 (96 bytes)
    LD HL, IVT_SRC
    LD DE, IVT_DEST
    LD BC, IVT_SIZE
    LDIR

    ; Set CPU I register to F6h
    LD A, IVT_VEC_I
    LD I, A

    ; Patch STRAY.handler at 0x00F786 with "EI; RETI" ($FB $ED $4D)
    ; This ensures any spurious or unhandled interrupts return safely
    LD HL, STRAY_HANDLER
    LD (HL), $FB                    ; EI
    INC HL
    LD (HL), $ED                    ; RETI prefix
    INC HL
    LD (HL), $4D                    ; RETI opcode

    ; Set UART0_SPR to 3: identifies platform as AGON to TRS-OS
    LD A, $03
    OUT0 (UART0_SPR), A

    ; Reset Mixed-Memory Mode: RSMIX sets MADL = 0 (critical for Z80 interrupt stack handling)
    RSMIX

    ; Clear Z80 registers
    XOR A, A
    LD BC, 0
    LD DE, 0
    LD HL, 0

    ; Switch CPU from ADL=1 to ADL=0 (Z80 compatibility mode) and jump to 0x000000
    JP.SIS 0

stub_end:
stub_len: EQU stub_end - stub

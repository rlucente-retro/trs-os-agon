> [!NOTE]
> Use [Sijnstra's OSboot utility](https://github.com/sijnstra/agon-projects/tree/main/OSboot#) instead. This repository is deprecated.
 
This repository provides the bare-metal loader source code and build automation for running Daniel Paul Martin's **TRS-OS** (a port of **TRSDOS 6.3.1 / LS-DOS 6.3** to the Zilog eZ80) on the **Olimex Agon Light 2** and compatible Agon family hardware, as well as on software emulators such as `fab-agon-emulator`.

---

## Architecture & How It Works

### Does it run on top of MOS and VDP?
- **MOS (No)**: TRS-OS is a bare-metal operating system that replaces Quark MOS. MOS is used only as a transient first-stage loader to read files into external SRAM. Once execution transitions to TRS-OS, MOS is completely replaced in memory.
- **VDP (Yes, as an ANSI Terminal)**: TRS-OS communicates with the ESP32 VDP over UART0 at **115,200 baud (8-N-1)**, using standard ASCII and VT100/ANSI terminal escape sequences for display and keyboard input.

### Memory Architecture & SRAM Partitioning

The Olimex Agon Light 2 features **512 KB** of external parallel SRAM (`AS6C4008`) and **8 KB** of on-chip eZ80 SRAM. During the hardware handover, the external SRAM is remapped via Chip Select 0 (`CS0`) from MOS's default range (`0x040000`–`0x0BFFFF`) down to base address zero (`0x000000`–`0x07FFFF`):

| Physical Address Range | Size | Allocation / Role |
|---|---|---|
| `0x000000`–`0x00FFFF` | 64 KB | **Z80 Execution Space (`ADL=0`)**: TRSDOS 6.3.1 operating system kernel, system tables, driver jump vectors, IVT (`0x00F600`), and user workspace. |
| `0x010000`–`0x077FFF` | 416 KB | **RAM Disk Area (Volume 0)**: Extended SRAM slices containing the boot RAM disk formatted as a **360 KB** Misosys DiskDISK volume (40 tracks × 2 sides × 18 sectors × 256 bytes = 368,640 bytes). Slices 0 & 1 metadata reside at `0x010000`–`0x0101FF`, DiskDISK header at `0x010200`, and sector data at `0x010300`–`0x06A2FF`. |
| `0x078000`–`0x07FFFF` | 32 KB | **Unallocated External SRAM**: Top of 512 KB physical SRAM (where `boottrs.bin` was loaded under MOS at `&B8000` prior to remapping). |
| `0xFFE000`–`0xFFFFFF` | 8 KB | **eZ80F92 On-Chip SRAM**: Transient trampoline buffer and stack used during hardware handover to safely reconfigure CS0 and peripheral registers. |

### Hardware Handover Sequence
1. **Load Phase**: MOS loads `trsos.dat` (480 KB) into external SRAM at `&40000` (`0x040000`–`0x0B7FFF`), then loads `boottrs.bin` (154 bytes) at `&B8000`.
2. **Launch**: Execution begins at `&B8000` in 24-bit linear addressing mode (`ADL=1`).
3. **SRAM Trampoline**: The launcher enables the eZ80F92 on-chip 8 KB SRAM (`RAM_CTL = 0x80`, `RAM_ADDR_U = 0xFF`), sets the stack pointer to `0xFFFFFF`, copies the relocation stub to on-chip SRAM (`0xFFE000`), and jumps to it.
4. **Hardware Handover**: Running safely from on-chip SRAM:
   - Disables all eZ80 timers (`TMR0`–`TMR5`), UART interrupts, and GPIO interrupt modes (`PB_ALT1`, `PC_ALT1`, `PD_ALT1`).
   - Relocates on-chip Flash to `0x200000` (`FLASH_ADDR_U = 0x20`).
   - Remaps external 512 KB SRAM CS0 to base address 0 (`CS0_LBR = 0x00`, `CS0_UBR = 0x07`), shifting `trsos.dat` directly to `0x000000`.
   - Relocates the eZ80F92 interrupt vector table to `0x00F600` and sets CPU `I = 0xF6`.
   - Patches `STRAY.handler` at `0x00F786` with `EI; RETI` so unhandled interrupts return cleanly.
   - Sets `UART0_SPR = 0x03` to identify the hardware platform as `AGON` to TRS-OS.
   - Resets Mixed-Memory Mode via `RSMIX` (`MADL = 0`) to prevent stack corruption on 16-bit interrupts.
   - Clears registers and executes `JP.SIS 0000h` to transition the CPU to 16-bit Z80 compatibility mode (`ADL=0`) and boot TRS-OS.

---

## Repository Structure

This repository contains **only source code and build automation**:

```text
├── Makefile             # Build automation (ez80asm integration, download, deploy)
├── LICENSE              # MIT License and third-party attribution
├── README.md            # Reference documentation
└── src/
    ├── boottrs.asm      # eZ80 assembly source for the MOS handover loader
    ├── ez80f92.inc      # Canonical eZ80F92 peripheral register equates
    ├── autoexec.txt     # Optional MOS auto-boot batch script
    └── TRS80M4pG.F10    # Authentic 8x10 TRS-80 Model 4 font bitmap
```

---

## Building and Deploying

### 1. Requirements & Tools
- **Assembler**: The canonical [AgonPlatform/agon-ez80asm](https://github.com/AgonPlatform/agon-ez80asm) assembler. Install `ez80asm` into your system `PATH`, or provide its path when invoking `make`.
- **Operating System Image**: `trsos.dat` (480 KB), distributed as `TRS-OS_Squirrel.bin` inside `TRS-NET.zip` on [Daniel Paul Martin's Downloads Page](https://danielpaulmartin.com/home/my-downloads/).

### 2. Building with Make
```bash
# Build boottrs.bin (uses ez80asm in PATH, or specify path with EZ80ASM=)
make [EZ80ASM=/path/to/ez80asm]

# Fetch the upstream 480 KB trsos.dat image from Daniel Paul Martin's release
make fetch

# Deploy all boot files directly to a microSD card or emulator directory
make install SDCARD=/path/to/sdcard

# Clean build artifacts
make clean
```

### 3. Assembling Natively on Agon Light
You can also assemble directly on Agon hardware under MOS using `ez80asm.bin`:
```text
*cd src
*ez80asm boottrs.asm ../boottrs.bin
```

---

## Running TRS-OS

### MicroSD Card Setup
Place the following files in the root directory of a FAT32-formatted microSD card (or your emulator's `sdcard` folder):
- `boottrs.bin` — Generated loader binary (197 bytes)
- `trsos.dat` — TRS-OS system and RAM disk image (480 KB)
- `autoexec.txt` — MOS auto-boot script
- `TRS80M4pG.F10` — Authentic 8×10 TRS-80 font bitmap (loaded into VDP font slot 1)

### Booting
- **Automatic**: Power on the Agon Light with the microSD card inserted. MOS automatically executes `autoexec.txt`.
- **Manual**: At the MOS prompt (`*`), enter:
  ```text
  *LOAD trsos.dat &40000
  *LOAD boottrs.bin &B8000
  *JMP &B8000
  ```

### Operating TRS-OS
1. At the **Main IPL Menu**, press `0` for **Default IPL**.
2. TRS-OS initializes the RAM disk (Volume 0), tests UART1 for TRS-NET, and presents the prompt:
   ```text
   TRSDOS Ready
   ```
3. Standard TRSDOS commands are available: `DIR`, `FREE`, `HELP`, `MEMDIR`, `DEVICE`, `PURGE`.

### Storage & Persistence (SD Card vs. RAM Disk vs. TRS-NET)
- **No MicroSD Card Access at Runtime**: TRS-OS runs bare-metal in 16-bit Z80 compatibility mode (`ADL=0`), completely replacing Quark MOS in memory. Because MOS and its SPI FatFS drivers are unloaded, TRS-OS cannot read from or write to the microSD card once running.
- **Volatile RAM Disk (Drive `:0`)**: Volume 0 is loaded into external SRAM during boot. Files can be created, modified, and executed on Drive `:0` while running, but **all changes are volatile and will be lost when the Agon Light is powered off or reset**.
- **Persistent Storage via TRS-NET (Drive `:6`)**: To save files persistently across sessions, connect the eZ80 UART1 serial pins to a host computer running `TRS-NET.py`. Mounting Drive `:6` allows transferring files (`BACKUP`, `COPY`) to and from virtual floppy disk images (`.dsk`, such as 720 KB images) stored on the host PC.

---

## Technical Credits & Attribution

- **TRS-OS Port for eZ80**: Ported and adapted for the Zilog eZ80 by Daniel Paul Martin ([danielpaulmartin.com](https://danielpaulmartin.com/how%20do%20i%20get/)). Full upstream source code is available in [TRSDOS_7.zip](https://danielpaulmartin.com/sitepad-data/uploads/TRSDOS_7.zip) and system images in [TRS-NET.zip](https://danielpaulmartin.com/sitepad-data/uploads/TRS-NET.zip) on his [Downloads page](https://danielpaulmartin.com/home/my-downloads/).
- **TRSDOS 6.3.1 / LS-DOS 6.3**: Originally developed by Logical Systems, Inc. and Misosys (Roy Soltoff, Dick Miller). See [Tim Mann's Misosys & LS-DOS Archive](https://www.tim-mann.org/misosys.html) and [Wikipedia: TRSDOS](https://en.wikipedia.org/wiki/TRSDOS).
- **Agon Light Hardware Platform**: Designed by Bernardo Kastrup ([The Byte Attic](https://www.thebyteattic.com/p/agon.html)) and manufactured as the AgonLight2 by Olimex ([OLIMEX AgonLight2 GitHub](https://github.com/OLIMEX/AgonLight2)).
- **Quark MOS & VDP**: Developed by Dean Belfield and the [Agon Platform](https://github.com/AgonPlatform) community ([Agon Platform Documentation](https://agonplatform.github.io/agon-docs/), [agon-mos](https://github.com/AgonPlatform/agon-mos), and [agon-vdp](https://github.com/AgonPlatform/agon-vdp)).
- **ez80asm Assembler**: Canonical Agon assembler developed by Jeroen Venema and the [Agon Platform](https://github.com/AgonPlatform/agon-ez80asm) community.
- **fab-agon-emulator**: Developed by Tom Nairn ([fab-agon-emulator GitHub](https://github.com/tomm/fab-agon-emulator)).

# TRS-OS on the Olimex Agon Light 2

This repository provides the loader, binaries, and instructions for running Daniel Paul Martin's **TRS-OS** (a port of **TRSDOS 6.3.1 / LS-DOS 6.3** to the Zilog eZ80) on the **Olimex Agon Light 2** (and compatible Agon Light boards), as well as on the **fab-agon-emulator**.

---

## Table of Contents
1. [Overview & Architecture](#overview--architecture)
2. [Does it run on top of MOS and VDP?](#does-it-run-on-top-of-mos-and-vdp)
3. [The Technical Challenge & The Loader Solution](#the-technical-challenge--the-loader-solution)
4. [Quick Start: Running in the Emulator](#quick-start-running-in-the-emulator)
5. [Running on Real Hardware (Olimex Agon Light 2)](#running-on-real-hardware-olimex-agon-light-2)
6. [Navigating TRS-OS / TRSDOS 6.3.1](#navigating-trs-os--trsdos-631)
7. [Repository Contents](#repository-contents)

---

## Overview & Architecture

**TRS-OS** is an operating system developed by Daniel Paul Martin based on **TRSDOS 6.3.1 / LS-DOS 6.3** (originally created by Logical Systems / Misosys for the TRS-80 Model 4).

### Key Architectural Characteristics:
- **CPU Architecture**: Runs on the Zilog eZ80 processor (specifically eZ80F92 on Agon Light 2, and eZ80F91 on some other platforms).
- **CPU Mode**: Runs in **ADL=0 (Z80 Compatibility Mode)** with **MADL=0 (Mixed-Memory Mode disabled)**. In this mode, addresses and registers behave as standard 16-bit Z80 registers (`PC`, `SP`, `HL`, `DE`, `BC`, etc.), addressing a 64 KB logical space mapped from `0x000000` to `0x00FFFF`.
- **Operating System Size**: The core operating system (`SYS0` through `SYS13`) occupies the base 64 KB of RAM (`0x000000`–`0x00FFFF`).
- **RAM Disk Storage (Volume 0)**: The Agon Light 2 has 512 KB of fast external SRAM. TRS-OS formats the remaining 448 KB of SRAM (`0x010000`–`0x07FFFF`) as an internal high-speed Misosys DiskDisk RAM drive (Volume 0). This volume contains all standard TRSDOS/LS-DOS utilities, library files, and system overlays (`BACKUP`, `BUILD`, `COMM`, `CONV`, `DIR`, `FORMAT`, `HELP`, `KILL`, `LIB`, `LINK`, `LIST`, `LOAD`, `MEMDIR`, `PURGE`, `RENAME`, `SYSGEN`, `TAPE`, etc.).
- **Serial Networking (TRS-NET)**: Includes built-in networking over eZ80 UART1 (`@ping`, `@pong`, virtual remote disk mounting via the companion Python server `TRS-NET.py`).

---

## Does it run on top of MOS and VDP?

### Does it run on top of MOS?
**NO.**
- TRS-OS is a **bare-metal operating system**.
- Standard Agon programs execute within MOS at address `&040000` in 24-bit ADL mode and use `RST 08h` / `RST 10h` to request MOS kernel services.
- TRS-OS, by contrast, takes over low memory (`0x000000`–`0x00FFFF`), defines its own interrupt vector table (IVT), installs its own restart vectors (`RST 00h` through `RST 38h`), and manages its own supervisor calls (`RST 28h` / SVCs).
- MOS is used **solely as a first-stage bootloader** to copy `trsos.dat` (480 KB) and `boottrs.bin` (154 bytes) from the microSD card into external SRAM before control is permanently transferred to TRS-OS. Once booted, MOS is completely deactivated and superseded.

### Does it run on top of VDP?
**INDIRECTLY, as an ANSI/VT100 Serial Console via UART0.**
- On the Agon Light hardware, the eZ80's primary serial port (**UART0**) is wired directly to the ESP32 coprocessor running the VDP firmware.
- TRS-OS contains an internal driver (`driver-uart0.s`) that binds to `$CL` (the console device).
- It transmits plain ASCII and VT100/ANSI escape sequences across UART0 at **115,200 baud** (8 data bits, 1 stop bit, no parity).
- The Agon VDP firmware receives these serial characters over UART0 and renders them to the VGA display. Keystrokes typed on the PS/2 or USB keyboard attached to the VDP are transmitted back as serial bytes over UART0 to the eZ80, which TRS-OS reads using its `getchar` routine.
- TRS-OS does **not** send VDP VDU graphics command packets; it interacts with VDP purely as an 80-column terminal.

---

## The Technical Challenge & The Loader Solution

Booting TRS-OS from MOS requires overcoming several hardware constraints of the eZ80F92 and Agon Light architecture:

1. **Memory Map Conflict**:
   - At power-on, MOS maps the eZ80F92 internal Flash (128 KB) at `0x000000`–`0x01FFFF`, and maps the external 512 KB SRAM to `0x040000`–`0x0BFFFF` using Chip Select 0 (`CS0`).
   - TRS-OS requires external SRAM to be located at `0x000000`–`0x07FFFF` so that its 64 KB OS and 416 KB RAM disk start at address 0.
   - If MOS loads `trsos.dat` at `0x040000`, the memory must be remapped to base 0. But modifying `CS0` while executing code located in external SRAM causes the CPU to crash immediately because the executing code disappears from beneath the program counter.

2. **The Execution Trampoline Solution**:
   - The eZ80F92 contains 8 KB of internal on-chip SRAM.
   - `boottrs.bin` enables this internal SRAM and maps it to `0xFFE000` (`RAM_CTL = 0x80`, `RAM_ADDR_U = 0xFF`).
   - It sets the stack pointer to `0xFFFFFF` (inside internal SRAM), copies a 122-byte relocation stub to `0xFFE000`, and jumps into it.
   - Running safely inside internal SRAM, the stub:
     1. Remaps the on-chip Flash out of the way to `0x200000` (`FLASH_ADDR_U = 0x20`).
     2. Sets `CS0_LBR = 0x00` and `CS0_UBR = 0x07`. This shifts the 512 KB external SRAM down to `0x000000`–`0x07FFFF`. The image previously loaded at `0x040000` now sits perfectly at `0x000000`!

3. **Platform Identification**:
   - TRS-OS checks the lower nibble of `UART0_SPR` (UART0 Scratchpad Register).
   - If `(UART0_SPR & 0x0F) == 3`, it identifies the board as `AGON` and skips destructive chip-select re-initialization (`NOINITIAL` path).
   - The stub writes `0x03` to `UART0_SPR` (`0xC7`).

4. **Interrupt Safety (`RSMIX` and Peripheral Cleanup)**:
   - MOS configures Timer 0 (for its 1ms system tick) and GPIO interrupt pins before handing over control.
   - In `driver-uart0.s`, TRS-OS issues an `EI` (Enable Interrupts) instruction on console I/O (`U0BGN`).
   - TRS-OS test builds only handle Timer 1 (60 Hz clock); all other vectors in `f92INTERRUPT.table` point to `STRAY.handler` which loops indefinitely (`jr $`).
   - Furthermore, on eZ80 reset, **MADL** (Mixed-Memory Mode ADL bit) is 1. If an interrupt fires with `MADL=1`, the CPU forces `ADL=1`, which desynchronizes TRS-OS's 16-bit Z80 interrupt handlers and causes a memory violation.
   - `boottrs.bin` resolves this by:
     - Disabling all hardware timers (`TMR0`–`TMR5`) and reading their status registers to clear pending interrupts.
     - Disabling UART0/UART1 and GPIO interrupt registers (`PB_ALT1`, `PC_ALT1`, `PD_ALT1`).
     - Copying the F92 interrupt vector table to `0x00F600` and setting `I = 0xF6`.
     - Patching `STRAY.handler` at `0x00F786` with `EI; RETI` (`FB ED 4D`) so any spurious interrupts return cleanly.
     - Executing **`RSMIX` (`ED 7E`)** to ensure `MADL = 0`.
     - Executing **`JP.SIS 000000h` (`40 C3 00 00`)** to switch to `ADL = 0` and jump to `0x000000`.

---

## Quick Start: Running in the Emulator

You can test TRS-OS immediately on macOS using the included `fab-agon-emulator`:

1. Ensure the required files are present on the emulator's virtual SD card:
   - `sdcard/trsos.dat` (480 KB OS + RAM disk image)
   - `sdcard/boottrs.bin` (154-byte loader)
   - `sdcard/autoexec.txt` (MOS auto-execution script)

2. Start the emulator:
   ```bash
   cd ../fab-agon-emulator-v1.2.4-macos-arm64
   ./fab-agon-emulator --sdcard ./sdcard
   ```

3. The emulator will open an SDL2 graphical window showing the Agon Light boot screen:
   - MOS automatically executes `autoexec.txt`:
     ```text
     LOAD trsos.dat &40000
     LOAD boottrs.bin &B8000
     JMP &B8000
     ```
   - TRS-OS boots, tests hardware, tests network (timing out harmlessly on `@ping`), and displays the **Main IPL Menu**.

---

## Running on Real Hardware (Olimex Agon Light 2)

### 1. Preparing the microSD Card
1. Format a microSD card (FAT32, standard MBR partition scheme).
2. Copy the following three files to the root directory of the microSD card:
   - `trsos.dat`
   - `boottrs.bin`
   - `autoexec.txt`

> **Note**: If you already use your microSD card for Agon MOS programs and do not want TRS-OS to boot automatically, do not include `autoexec.txt`. You can boot manually from the MOS prompt.

### 2. Booting Automatically
Insert the microSD card into the Olimex Agon Light 2, connect your VGA monitor and PS/2 (or USB) keyboard, and power on the board. MOS will read `autoexec.txt` and launch TRS-OS.

### 3. Booting Manually from the MOS Prompt
If booting manually, turn on the Agon Light 2 to the MOS prompt (`*`), then enter:
```text
*LOAD trsos.dat &40000
*LOAD boottrs.bin &B8000
*JMP &B8000
```

---

## Navigating TRS-OS / TRSDOS 6.3.1

When TRS-OS boots, it presents the **Main IPL Menu**:

```text
               TRS-OS / TRSDOS 6.3.1 (eZ80)
   Copyright 2024 Daniel Paul Martin, All rights reserved.

   Platform: AGON (eZ80F92 @ 18.432 MHz)
   Memory  : 64 KB System RAM + 416 KB RAM Disk (Vol 0)

   0 - Default IPL (Boot TRSDOS System)
   1 - System Diagnostics
   2 - Safe Mode IPL (Prompt before loading SYSGEN/AUTO)
   3 - BIT Diagnostic Initialization
   4 - Real-Time Clock (RTC) Setup
   5 - Edit / Inspect Memory Slices
   D - Select Default Terminal Type
   N - Network Utilities (TRS-NET)
```

- **Press `0`** to proceed with **Default IPL**:
  - The system checks disk volumes, synchronizes the heartbeat clock, and displays the familiar TRSDOS prompt:
    ```text
    TRSDOS Ready
    ```
- From the TRSDOS prompt, you can use all standard TRSDOS/LS-DOS 6.x commands:
  - `DIR` - List files on RAM disk Volume 0.
  - `FREE` - Show free disk space.
  - `HELP` - Display interactive documentation.
  - `MEMDIR` - Show resident drivers and system memory allocation.
  - `BASIC` - Launch Model 4 Disk BASIC (if installed on volume).

---

## Repository Contents

| File | Description |
|---|---|
| `build_loader.py` | Python script to assemble and generate `boottrs.bin`. |
| `boottrs.bin` | The compiled 154-byte Agon MOS loader binary. |
| `autoexec.txt` | MOS auto-boot script (`LOAD ... &40000`, `LOAD ... &B8000`, `JMP &B8000`). |
| `trsos.dat` | Complete 480 KB TRS-OS system binary and pre-formatted Volume 0 RAM disk. |
| `README.md` | This documentation file. |

---

## Technical Credits & Citations
- **TRSDOS 6.3.1 / LS-DOS 6.3**: Originally created by Logical Systems, Inc. / Misosys (Roy Soltoff, Dick Miller).
- **TRS-OS for eZ80**: Ported and adapted for eZ80 by Daniel Paul Martin ([danielpaulmartin.com](https://danielpaulmartin.com/how%20do%20i%20get/)).
- **Agon Light Platform**: Created by Bernardo Kastrup (The Byte Attic) and Olimex.
- **MOS & VDP**: Developed by Dean Belfield and the Agon Platform community.
- **fab-agon-emulator**: Developed by Tom Nairn.

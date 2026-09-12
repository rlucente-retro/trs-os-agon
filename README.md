# TRS-OS on the Agon Light Platform

This repository provides the loader, system image, and instructions for running Daniel Paul Martin's **TRS-OS** (a port of **TRSDOS 6.3.1 / LS-DOS 6.3** to the Zilog eZ80) on the **Olimex Agon Light 2** and compatible Agon family hardware (e.g., Agon Light, Agon Electron, Agon Console8), as well as on software emulators such as `fab-agon-emulator`.

---

## Table of Contents
1. [Overview & Architecture](#overview--architecture)
2. [Does it Run on Top of MOS and VDP?](#does-it-run-on-top-of-mos-and-vdp)
3. [The Technical Challenge & The Loader Solution](#the-technical-challenge--the-loader-solution)
4. [Prerequisites & Repository Contents](#prerequisites--repository-contents)
5. [Running on Real Hardware (MicroSD Setup)](#running-on-real-hardware-microsd-setup)
6. [Running in an Agon Emulator](#running-in-an-agon-emulator)
7. [Building the Loader (`boottrs.bin`)](#building-the-loader-boottrsbin)
8. [Navigating TRS-OS / TRSDOS 6.3.1](#navigating-trs-os--trsdos-631)
9. [Networking (TRS-NET)](#networking-trs-net)
10. [Technical Credits & References](#technical-credits--references)

---

## Overview & Architecture

**TRS-OS** is an operating system developed by Daniel Paul Martin based on **TRSDOS 6.3.1 / LS-DOS 6.3** (originally authored by Logical Systems / Misosys for the Tandy TRS-80 Model 4).

### Key Architectural Characteristics:
- **Target CPU**: Zilog eZ80 series running at 18.432 MHz (eZ80F92 on the Olimex Agon Light 2; eZ80F91 on some other development systems).
- **Execution Mode**: Runs in **ADL=0 (Z80 Compatibility Mode)** with **MADL=0 (Mixed-Memory Mode disabled)**. In this mode, registers and addresses behave as standard 16-bit Z80 registers (`PC`, `SP`, `HL`, `DE`, `BC`, `IX`, `IY`), addressing a 64 KB logical memory address space from `0x000000` to `0x00FFFF`.
- **System Memory**: The core operating system (`SYS0` through `SYS13`) resides entirely within the lower 64 KB of RAM (`0x000000`–`0x00FFFF`).
- **RAM Disk Storage (Volume 0)**: The Agon Light provides 512 KB of fast external SRAM. TRS-OS formats the remaining 448 KB of SRAM (`0x010000`–`0x07FFFF`) as an internal high-speed Misosys DiskDisk RAM drive (designated as **Volume 0** / `:0`). This volume contains all standard TRSDOS/LS-DOS commands, utilities, and system overlays (`BACKUP`, `BUILD`, `COMM`, `CONV`, `DIR`, `FORMAT`, `HELP`, `KILL`, `LIB`, `LINK`, `LIST`, `LOAD`, `MEMDIR`, `PURGE`, `RENAME`, `SYSGEN`, `TAPE`, etc.).
- **Serial Networking (TRS-NET)**: Includes built-in networking routines via eZ80 UART1 (`@ping`, `@pong`, virtual remote disk mounting via a host Python server).

---

## Does it Run on Top of MOS and VDP?

### Does it run on top of MOS?
**NO.**
- TRS-OS is a **bare-metal operating system**.
- Conventional Agon software runs under Quark MOS at memory address `&040000` in 24-bit ADL mode, relying on MOS kernel calls (`RST 08h` / `RST 10h`).
- TRS-OS completely replaces MOS. It overwrites low memory (`0x000000`–`0x00FFFF`), configures its own interrupt vector table (IVT), installs its own restart vectors (`RST 00h` through `RST 38h`), and manages its own supervisor calls (`RST 28h` / SVCs).
- MOS is utilized **only as a transient first-stage bootloader** to copy `trsos.dat` (480 KB) and `boottrs.bin` (154 bytes) from the microSD card into SRAM before execution is transferred to TRS-OS. Once booted, MOS is superseded and no longer present in memory.

### Does it run on top of VDP?
**INDIRECTLY, as an ANSI/VT100 Serial Console via UART0.**
- On the Agon Light hardware architecture, the eZ80 primary serial port (**UART0**) is directly connected to the ESP32 coprocessor running the VDP (Visual Display Processor) firmware.
- TRS-OS provides an internal console driver (`driver-uart0.s`) linked to `$CL` (the console device).
- Console output is transmitted as standard ASCII characters and VT100/ANSI terminal control sequences across UART0 at **115,200 baud, 8-N-1**.
- The Agon VDP firmware receives these serial bytes over UART0 and renders them to the VGA screen. Keystrokes entered on the PS/2 or USB keyboard attached to the VDP are sent back as serial bytes over UART0 to the eZ80, which TRS-OS reads using its `getchar` routine.
- TRS-OS does not invoke VDP VDU graphics command packets; it interacts with VDP strictly as an 80-column terminal.

---

## The Technical Challenge & The Loader Solution

Booting TRS-OS from MOS requires addressing several hardware constraints of the eZ80F92 memory controller and Agon Light board design:

1. **Memory Map Conflict**:
   - At power-on, MOS maps the eZ80F92 internal Flash (128 KB) at `0x000000`–`0x01FFFF`, and maps the external 512 KB SRAM starting at `0x040000` (`CS0_LBR = 0x04`, `CS0_UBR = 0x0B`).
   - TRS-OS requires external SRAM to be located at `0x000000`–`0x07FFFF` so that low memory and the RAM disk start at base 0.
   - If MOS loads `trsos.dat` at `0x040000`, the memory must be remapped to base 0. However, modifying `CS0` while executing from external SRAM causes the CPU to crash immediately, as the code being executed vanishes from beneath the program counter.

2. **The Execution Trampoline Solution**:
   - The eZ80F92 contains 8 KB of internal on-chip SRAM.
   - The `boottrs.bin` loader enables this internal SRAM and maps it to `0xFFE000` (`RAM_CTL = 0x80`, `RAM_ADDR_U = 0xFF`).
   - It sets the stack pointer to `0xFFFFFF` (inside internal SRAM), copies a 122-byte relocation stub to `0xFFE000`, and jumps into it.
   - Running safely inside internal SRAM, the stub:
     1. Relocates on-chip Flash out of the way to `0x200000` (`FLASH_ADDR_U = 0x20`).
     2. Configures `CS0_LBR = 0x00` and `CS0_UBR = 0x07`. This shifts the 512 KB external SRAM down to `0x000000`–`0x07FFFF`. The image previously loaded at `0x040000` now sits at `0x000000`.

3. **Platform Identification**:
   - TRS-OS inspects the lower nibble of `UART0_SPR` (UART0 Scratchpad Register).
   - If `(UART0_SPR & 0x0F) == 3`, it identifies the board as `AGON` and skips destructive chip-select re-initialization (`NOINITIAL` branch).
   - The stub sets `UART0_SPR = 0x03`.

4. **Interrupt Safety (`RSMIX` and Peripheral Cleanup)**:
   - MOS initializes Timer 0 (for its 1ms system tick) and GPIO interrupt pins prior to executing user code.
   - In `driver-uart0.s`, TRS-OS executes an `EI` (Enable Interrupts) instruction during serial operations (`U0BGN`).
   - In TRS-OS test releases, only Timer 1 (60 Hz system clock) is serviced; all other vectors in `f92INTERRUPT.table` point to `STRAY.handler`, which halts the CPU in a loop (`jr $`).
   - Additionally, upon eZ80 reset, **MADL** (Mixed-Memory Mode ADL bit) defaults to 1. If an interrupt occurs with `MADL=1`, the CPU switches to `ADL=1`, corrupting the return stack and crashing 16-bit Z80 interrupt routines.
   - `boottrs.bin` resolves this by:
     - Disabling all hardware timers (`TMR0`–`TMR5`) and clearing pending interrupt flags.
     - Disabling UART0/UART1 and GPIO interrupt registers (`PB_ALT1`, `PC_ALT1`, `PD_ALT1`).
     - Relocating the F92 interrupt vector table to `0x00F600` and initializing `I = 0xF6`.
     - Patching `STRAY.handler` at `0x00F786` with `EI; RETI` (`FB ED 4D`) so any spurious interrupts return cleanly.
     - Executing **`RSMIX` (`ED 7E`)** to ensure `MADL = 0`.
     - Executing **`JP.SIS 000000h` (`40 C3 00 00`)** to switch the eZ80 to `ADL = 0` and jump to `0x000000`.

---

## Prerequisites & Repository Contents

### Requirements
- **Hardware**: An Agon Light computer (e.g., Olimex Agon Light 2, original Agon Light, Agon Electron, Console8) with a microSD card, a VGA monitor, and a PS/2 (or USB) keyboard.
- **Software/Emulation**: Any standard Agon emulator (e.g., `fab-agon-emulator`, `Agon-Light-Emulator`) running on macOS, Linux, or Windows.

### Repository Files
| File | Description |
|---|---|
| `trsos.dat` | Complete 480 KB binary (64 KB TRS-OS system + 416 KB pre-formatted Volume 0 RAM disk). |
| `boottrs.bin` | Compiled 154-byte Agon MOS loader and memory remap stub. |
| `autoexec.txt` | MOS auto-execution script to load and launch TRS-OS on power-up. |
| `build_loader.py` | Standalone Python 3 script to assemble and regenerate `boottrs.bin`. |
| `README.md` | This reference documentation. |

---

## Running on Real Hardware (MicroSD Setup)

### Step 1: Format the MicroSD Card
Format a microSD card using **FAT32** with a standard MBR (Master Boot Record) partition table. (Cards up to 32 GB formatted with standard SD card formatting tools are recommended.)

### Step 2: Copy Required Files
Copy the following files to the **root directory** of the microSD card:
- `trsos.dat`
- `boottrs.bin`
- `autoexec.txt`

> **Tip**: If you use your microSD card for other MOS software and do not want TRS-OS to boot automatically, omit `autoexec.txt` and use the manual boot instructions below.

### Step 3: Booting Automatically
1. Insert the microSD card into the Agon Light card slot.
2. Connect a VGA monitor and a compatible keyboard.
3. Power on the system.
4. Quark MOS will automatically execute `autoexec.txt`, load `trsos.dat` to `&40000`, load `boottrs.bin` to `&B8000`, and start TRS-OS.

### Step 4: Booting Manually from the MOS Prompt
If booting manually, turn on the Agon Light to the MOS command prompt (`*`), then enter:
```text
*LOAD trsos.dat &40000
*LOAD boottrs.bin &B8000
*JMP &B8000
```

---

## Running in an Agon Emulator

You can run TRS-OS in any Agon Light software emulator (such as `fab-agon-emulator` by Tom Nairn):

1. **Locate your emulator's virtual SD card directory**:
   Most Agon emulators use a directory on the host filesystem (often named `sdcard`) to simulate the microSD card.

2. **Copy files into the emulator's SD card directory**:
   ```bash
   cp trsos.dat <path_to_emulator>/sdcard/
   cp boottrs.bin <path_to_emulator>/sdcard/
   cp autoexec.txt <path_to_emulator>/sdcard/
   ```

3. **Launch the emulator**:
   ```bash
   cd <path_to_emulator>
   ./fab-agon-emulator --sdcard ./sdcard
   ```
   MOS will execute `autoexec.txt` and launch into the TRS-OS boot menu.

---

## Building the Loader (`boottrs.bin`)

The repository includes a pre-built `boottrs.bin`. If you modify the stub or wish to rebuild it from source, run `build_loader.py` with Python 3:

```bash
python3 build_loader.py
```

The script assembles the machine code bytes for the on-chip SRAM trampoline, interrupt setup, and ADL mode switch, outputting a 154-byte binary. No external assembler or dependencies are required.

---

## Navigating TRS-OS / TRSDOS 6.3.1

When TRS-OS starts, it displays the **Main IPL Menu**:

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

### Booting into TRSDOS
- **Press `0`** to select **Default IPL**:
  - The system tests the disk volume, starts the 60 Hz timer heartbeat, and displays the command prompt:
    ```text
    TRSDOS Ready
    ```

### Common Commands to Explore
Once at the `TRSDOS Ready` prompt, standard TRSDOS/LS-DOS 6.x commands are available:
- `DIR` — Lists files on Volume 0 (the RAM disk).
- `FREE` — Displays available space on mounted disk volumes.
- `HELP` — Opens the built-in interactive help utility.
- `MEMDIR` — Displays memory layout, resident drivers, and system areas.
- `DEVICE` — Shows active device routing (`*KI`, `*DO`, `*PR`, etc.).
- `PURGE` — Removes temporary or unneeded files from the volume.

---

## Networking (TRS-NET)

TRS-OS includes network client capabilities via eZ80 **UART1** to interface with a host computer running `TRS-NET.py`:
- During boot, TRS-OS issues an `@ping` packet over UART1. If no response is received, it times out gracefully and continues local operation.
- Connecting UART1 to a host serial adapter (or an ESP32 secondary channel) with `TRS-NET.py` running allows mounting remote host disk images and transferring files over the serial connection.

---

## Technical Credits & References

- **TRSDOS 6.3.1 / LS-DOS 6.3**: Originally developed by Logical Systems, Inc. and Misosys (Roy Soltoff, Dick Miller). See [Tim Mann's Misosys & LS-DOS Archive](https://www.tim-mann.org/misosys.html) and [Wikipedia: TRSDOS](https://en.wikipedia.org/wiki/TRSDOS).
- **TRS-OS Port for eZ80**: Ported and adapted for eZ80 by Daniel Paul Martin ([danielpaulmartin.com](https://danielpaulmartin.com/how%20do%20i%20get/)).
- **Agon Light Hardware Platform**: Designed by Bernardo Kastrup ([The Byte Attic](https://www.thebyteattic.com/p/agon.html)) and manufactured as the AgonLight2 by Olimex ([OLIMEX AgonLight2 GitHub](https://github.com/OLIMEX/AgonLight2)).
- **Quark MOS & VDP**: Developed by Dean Belfield and the Agon Platform community ([Agon Platform Documentation](https://agonplatform.github.io/agon-docs/), [agon-mos](https://github.com/breakintoprogram/agon-mos), and [agon-vdp](https://github.com/breakintoprogram/agon-vdp)).
- **fab-agon-emulator**: Developed by Tom Nairn ([fab-agon-emulator GitHub](https://github.com/tomm/fab-agon-emulator)).

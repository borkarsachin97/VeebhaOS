# OBTEL B10 Hardware Bringup & Flashing Guide

This document details the hardware architecture, memory layout, toolchain configuration, and flashing instructions for the **OBTEL B10** feature phone.

---

## 1. Hardware Specifications

The **OBTEL B10** is a classic form-factor feature phone powered by the RDA Microelectronics (Unisoc) RDA8809 platform.

| Subsystem | Specification |
| :--- | :--- |
| **System-on-Chip (SoC)** | RDA8809 (MIPS32r1 Architecture) |
| **Clock Frequency** | 312 MHz (Dynamic PLL scaling down to 32 kHz idle) |
| **Internal RAM** | 8 MB PSRAM (Pseudo-Static RAM) |
| **Internal Flash** | 4 MB SPI NOR Flash |
| **Display Panel** | 2.0-inch 176×220 Portrait TFT LCD (ILI9225G controller, RGB565) |
| **Graphics Accelerator** | RDA GOUDA 2D Hardware DMA Blitter (zero-copy memory-to-LCD) |
| **Keypad** | 4×5 Hardware Matrix Keypad with D-Pad and Softkeys |
| **Audio** | Integrated RDA ABB (Analog Baseband) DAC + 8-bit Polyphonic Synthesizer |
| **Wireless** | 2G Quad-band GSM/GPRS + Bluetooth 2.1+EDR (OBEX, BNEP PAN) |
| **Storage Expansion** | MicroSD slot (up to 32 GB, SDSC/SDHC via SPI/SDMMC) |
| **Interface Port** | Micro-USB (CDC ACM Serial, Mass Storage, Flashing) |

---

## 2. Memory Map

The RDA8809 memory map partitions physical memory across RAM and Flash:

```text
+-----------------------+ 0x8200_0000 (8 MB PSRAM End)
|  FreeRTOS Dynamic Pool| (App allocations & VAPP sandbox)
+-----------------------+ 0x81C0_0000
|  LVGL Display Pool    | (2.0 MB fenced graphics buffer)
+-----------------------+ 0x81A0_0000
|  GOUDA DMA Buffers    | (Ping-Pong LCD scanline buffers)
+-----------------------+ 0x8180_0000
|  Kernel BSS & Data    | (OS kernel & driver states)
+-----------------------+ 0x8100_0000 (RAM Base / Cached KSEG0)
==============================================================
+-----------------------+ 0x8040_0000 (4 MB Flash End)
|  User NVRAM Storage   | (Persistent calibration & preferences)
+-----------------------+ 0x8038_0000
|  VAPP Storage Area    | (Preloaded .vapp packages)
+-----------------------+ 0x8008_0000
|  VeebhaOS Code (.text)| (Compiled OS firmware binary)
+-----------------------+ 0x8000_0000 (Flash Base / KSEG0)
```

---

## 3. Toolchain Setup

Building for the OBTEL B10 requires a MIPS baremetal cross-compiler (`mips-elf-gcc` or `mips-mti-elf-gcc`) supporting the MIPS32 Release 1 instruction set.

### Recommended Toolchain
CoolTools GCC for RDA MIPS:
* Compiler: `mips-elf-gcc` (GCC 4.4.2 or modern `mips-mti-elf-gcc`)
* Architecture Flags: `-march=mips32r1 -mabi=32 -EL -mno-shared -mno-abicalls`
* Optimization Flags: `-O2 -ffunction-sections -fdata-sections -G0`

Verify your compiler is present in your PATH:
```bash
mips-elf-gcc --version
```

---

## 4. Building Firmware

To compile the baremetal firmware for OBTEL B10 from the repository root:

```bash
make clean BOARD=obtel_b10
make BOARD=obtel_b10
```

The build system executes the board Makefile in [`boards/obtel_b10/Makefile`](file:///home/vixxkigoli/pm/VeebhaOS/boards/obtel_b10/Makefile) and produces:
* `build/obtel_b10/veebha_os.elf`: Executable and Linkable Format binary with debug symbols.
* `build/obtel_b10/veebha_os.bin`: Raw flat binary image ready for flash programming.
* `build/obtel_b10/disassembly.txt`: Complete assembly disassembly for verification.

---

## 5. Flashing the OBTEL B10

### Putting the Device into USB Boot Mode
1. Power off the OBTEL B10 and remove the battery.
2. Re-insert the battery (do not press the Power button).
3. Hold the **Center OK** button (or **Center + * key** on some board revisions).
4. Connect the Micro-USB cable to your PC while holding the key.
5. Release the key after 2 seconds. The phone will enumerate on USB as an RDA USB Download Device (`1782:4d00`).

### Flashing via CoolWatcher / CoolTools
```bash
# Using CoolTools USB Flash Loader CLI:
coolhost -cmd "load_bin build/obtel_b10/veebha_os.bin 0x80000000"
coolhost -cmd "reboot"
```

Once rebooted, the ILI9225G display will initialize and boot directly into the VeebhaOS Standby / Home screen.

---

## 6. Live Hardware Gallery

The following captures demonstrate VeebhaOS executing on physical OBTEL B10 hardware (RDA8809 @ 312 MHz, 176×220 ILI9225G LCD, hardware matrix keypad):

<div align="center">

| **Idle Screen & Live Pill** | **Main Menu Launcher** | **Quick Settings Drawer** |
| :---: | :---: | :---: |
| <img src="../screenshots/obtel_b10/idle_screen.jpg" width="200" alt="Idle Screen" /> | <img src="../screenshots/obtel_b10/main_menu.jpg" width="200" alt="Main Menu" /> | <img src="../screenshots/obtel_b10/qs_settings.jpg" width="200" alt="Quick Settings" /> |

| **Task Switcher (Card Stack)** | **Fun Zone Store** | **Brick Breaker on LCD** |
| :---: | :---: | :---: |
| <img src="../screenshots/obtel_b10/task_switcher.jpg" width="200" alt="Task Switcher" /> | <img src="../screenshots/obtel_b10/veebhaos_funzone.jpg" width="200" alt="Fun Zone" /> | <img src="../screenshots/obtel_b10/brick_game1.jpg" width="200" alt="Brick Breaker" /> |

| **Tetris Retro Action** | **Space Shooter Gameplay** | **Hardware Calibration & Specs** |
| :---: | :---: | :---: |
| <img src="../screenshots/obtel_b10/tetris_1.jpg" width="200" alt="Tetris" /> | <img src="../screenshots/obtel_b10/space_shooter_game.jpg" width="200" alt="Space Shooter" /> | <img src="../screenshots/obtel_b10/about_phone1.jpg" width="200" alt="Hardware Specs" /> |

</div>


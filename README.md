# VeebhaOS

<div align="center">

**A modern, ultra-lightweight embedded operating system engineered for feature phones and low-power microcontrollers.**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)](#quick-start)
[![Target: OBTEL B10](https://img.shields.io/badge/Hardware-OBTEL%20B10%20(RDA8809)-orange.svg)](docs/getting-started/obtel-b10-hardware.md)
[![Simulator: SDL2](https://img.shields.io/badge/Simulator-Linux%20%7C%20macOS%20%7C%20WSL-blue.svg)](docs/getting-started/quickstart-simulator.md)
[![Math: Zero-FPU](https://img.shields.io/badge/FPU-Strict%20Zero--Float%20(Integer%20Only)-red.svg)](docs/architecture/no-fpu-policy.md)
[![Kernel: RTOS & Linux](https://img.shields.io/badge/Kernel-FreeRTOS%20%7C%20Linux%20Kernel-informational.svg)](docs/porting-guide.md)
[![Manifesto: Author Note](https://img.shields.io/badge/Manifesto-Release%20Note-purple.svg)](AUTHOR_NOTE.md)

<br/>

### Core System Shell & Multitasking

| **Idle Screen (Live Pill)** | **3×3 Main Menu** | **Quick Settings (Media)** | **Task Switcher (3 Running)** |
| :---: | :---: | :---: | :---: |
| <img src="docs/screenshots/idle_screen_live_pill.png" width="165" alt="Idle Screen with Live Pill" /> | <img src="docs/screenshots/main_menu.png" width="165" alt="3x3 Main Menu Launcher" /> | <img src="docs/screenshots/qs_panel_settings_music_playing.png" width="165" alt="Quick Settings Panel with Media" /> | <img src="docs/screenshots/task_switcher_having3_tskrunning.png" width="165" alt="Multitasking Task Switcher" /> |

### Distinct System Themes

| **Dark Cyan (Default)** | **OLED Black** | **Clean Light** | **High-Contrast B&W** |
| :---: | :---: | :---: | :---: |
| <img src="docs/screenshots/main_menu_cyan_theme.png" width="165" alt="Dark Cyan Theme" /> | <img src="docs/screenshots/main_menu_oled_theme.png" width="165" alt="OLED Black Theme" /> | <img src="docs/screenshots/main_menu_light_theme.png" width="165" alt="Clean Light Theme" /> | <img src="docs/screenshots/main_menu_bw_theme.png" width="165" alt="High Contrast B&W Theme" /> |

### Fun Zone & Arcade Gaming (.vapp Dynamic Packages)

| **Fun Zone Store** | **Brick Breaker (60 FPS)** | **Tetris Retro** | **Space Shooter** |
| :---: | :---: | :---: | :---: |
| <img src="docs/screenshots/vebhaos_fun_zone.png" width="165" alt="Fun Zone App Store" /> | <img src="docs/screenshots/brickgame_in_funzone.png" width="165" alt="Brick Breaker Game" /> | <img src="docs/screenshots/tetris_in_funzone.png" width="165" alt="Tetris Retro Game" /> | <img src="docs/screenshots/space_shooter_gamein_funzone.png" width="165" alt="Space Shooter Game" /> |

### Media, Internet Lite & Tools

| **Walkman Music Player** | **FM Radio Tuner** | **Tiny Web Browser** | **File Manager** |
| :---: | :---: | :---: | :---: |
| <img src="docs/screenshots/walkman_music_play.png" width="165" alt="Walkman Music Player" /> | <img src="docs/screenshots/walkman_fm_radio.png" width="165" alt="FM Radio Tuner" /> | <img src="docs/screenshots/browser.png" width="165" alt="Tiny Web Browser" /> | <img src="docs/screenshots/file_manager.png" width="165" alt="File Manager" /> |

### Notifications & Preferences

| **Notifications Deck** | **Settings Menu** | **Theme Selector** | **About Phone (Hardware)** |
| :---: | :---: | :---: | :---: |
| <img src="docs/screenshots/qs_notification.png" width="165" alt="Notifications Deck" /> | <img src="docs/screenshots/settings.png" width="165" alt="System Settings" /> | <img src="docs/screenshots/settings_theme_list.png" width="165" alt="Theme Selector" /> | <img src="docs/screenshots/settings_about_phone.png" width="165" alt="About Phone Hardware Details" /> |

</div>

---

## Hardware in Action (OBTEL B10 Live Photos)

The photos below capture **VeebhaOS running live on physical hardware** — an OBTEL B10 feature phone powered by the **RDA8809 MIPS32r1 CPU**, 176×220 portrait LCD, and hardware matrix keypad:

### Core Shell & Navigation
<div align="center">

| **Idle Screen & Live Pill** | **3×3 Main Launcher** | **Task Switcher (Card Stack)** |
| :---: | :---: | :---: |
| <img src="docs/screenshots/obtel_b10/idle_screen.jpg" width="220" alt="OBTEL B10 Idle Screen" /> | <img src="docs/screenshots/obtel_b10/main_menu.jpg" width="220" alt="OBTEL B10 Main Menu" /> | <img src="docs/screenshots/obtel_b10/task_switcher.jpg" width="220" alt="OBTEL B10 Task Switcher" /> |

| **Quick Settings Drawer** | **Notification Deck** | **System Settings** |
| :---: | :---: | :---: |
| <img src="docs/screenshots/obtel_b10/qs_settings.jpg" width="220" alt="OBTEL B10 Quick Settings" /> | <img src="docs/screenshots/obtel_b10/notification.jpg" width="220" alt="OBTEL B10 Notifications" /> | <img src="docs/screenshots/obtel_b10/settings.jpg" width="220" alt="OBTEL B10 Settings" /> |

</div>

### Fun Zone & Dynamic Games on Silicon
<div align="center">

| **Fun Zone App Store** | **Brick Breaker (Stage 1)** | **Brick Breaker (Action)** |
| :---: | :---: | :---: |
| <img src="docs/screenshots/obtel_b10/veebhaos_funzone.jpg" width="220" alt="Fun Zone Store" /> | <img src="docs/screenshots/obtel_b10/brick_game1.jpg" width="220" alt="Brick Breaker on LCD" /> | <img src="docs/screenshots/obtel_b10/brick_game2.jpg" width="220" alt="Brick Breaker Action" /> |

| **Tetris Retro (Matrix)** | **Tetris Retro (Gameplay)** | **Space Shooter Combat** |
| :---: | :---: | :---: |
| <img src="docs/screenshots/obtel_b10/tetris_1.jpg" width="220" alt="Tetris on OBTEL B10" /> | <img src="docs/screenshots/obtel_b10/tetris_2.jpg" width="220" alt="Tetris Stacking Action" /> | <img src="docs/screenshots/obtel_b10/space_shooter_game.jpg" width="220" alt="Space Shooter on LCD" /> |

</div>

### Hardware Diagnostics & RDA8809 Specs
<div align="center">

| **About Phone (System Specs)** | **About Phone (Hardware Memory & Info)** |
| :---: | :---: |
| <img src="docs/screenshots/obtel_b10/about_phone1.jpg" width="220" alt="About Phone Hardware Specs" /> | <img src="docs/screenshots/obtel_b10/about_phone2.jpg" width="220" alt="About Phone Memory Info" /> |

</div>

---

## Overview

**VeebhaOS** is a freestanding C99 operating system designed for hardware-constrained feature phones and embedded devices with 176×220 portrait displays. It balances rich, modern graphical UI capabilities with deterministic real-time performance on resource-constrained embedded hardware.

### Working Boards & Reference Targets
1. **OBTEL B10**: Primary bare-metal hardware target powered by the **RDA8809 MIPS32r1 @ 312 MHz** processor with 8 MB RAM, 4 MB SPI Flash, ILI9225G LCD, RDA GOUDA 2D Hardware DMA blitter, and FreeRTOS v10.
2. **Desktop Simulator**: Full native SDL2 simulator running the exact same FreeRTOS scheduler, driver interfaces, display HAL, and window manager on Linux, macOS, and Windows (WSL).

### Universal Portability (RTOS & Linux Kernel)
While developed with the OBTEL B10 and desktop simulator as reference targets, **VeebhaOS is fundamentally hardware- and OS-agnostic**. The shell, window manager, input engine, and application runtime are cleanly decoupled from the underlying hardware via OSAL (OS Abstraction Layer) and HAL (Hardware Abstraction Layer).

**VeebhaOS can be ported to ANY device running:**
- **An RTOS**: FreeRTOS, RT-Thread, Zephyr, or vendor baseband RTOS on ARM Cortex-M, MIPS, or RISC-V.
- **A Linux Kernel**: Embedded Linux / SBCs using the Linux Framebuffer (`/dev/fb0`) or DRM/KMS, and Linux `evdev` (`/dev/input/event*`).

📖 *See the complete [Porting Guide (FreeRTOS & Linux Kernel)](docs/porting-guide.md) for step-by-step instructions.*

---

## Key Features

* **Strict Zero Floating-Point Arithmetic**: The entire operating system, window manager, widgets, and game engines strictly use 100% integer and fixed-point math (`CONFIG_STRICT_NO_FLOAT`), eliminating FPU emulation overhead on MIPS and Cortex-M cores.
* **Universal Quick Settings & Notification Drawer**: Hold the **CALL** key anywhere in the OS or during full-screen games to pull down notifications, toggles (Bluetooth, Torch, Brightness, Sound), and now-playing media controls. Short press **CALL** to toggle between notifications and settings.
* **Preemptive Multitasking & Task Switcher**: Hold the **`*`** key for 600ms to open the visual card Task Manager, pause/minimize apps, or kill background tasks.
* **Dynamic Idle Screen Live Pill**: Contextual interactive badge on the home screen showing ongoing calls with live durations, Walkman track info, FM Radio frequency, or countdown timers with one-click jump-to-app.
* **4 Distinct Built-in Themes**: Instant system-wide switching across **Dark Cyan**, **OLED Black**, **Clean Light**, and **High-Contrast B&W** (direct sunlight readability).
* **VAPP Dynamic Package Engine (`.vapp`)**:
  * Self-contained binary application format with CCITT CRC16 verification, heap budgeting, and metadata.
  * Built-in **VeebhaOS Fun Zone** app store with 60 FPS arcade games (*Space Shooter*, *Brick Breaker*, *Tetris Retro*) and utility apps (*Chip Synth*, *Morse Flasher*, *Unit Converter*, *System Monitor*).
* **Pre-installed Core Applications**:
  * **Telephony & Messaging**: Phone Dialer, Contacts, SMS Messaging with T9 text editor, and Call Logs.
  * **Media & Utilities**: Walkman Audio Player, Image Gallery, Voice Recorder, File Manager, Tools Suite, Calculator, Stopwatch, Alarm Clock, Flashlight, and Text Reader.
  * **Connectivity**: Tiny Web Browser (WAP/HTTP), Bluetooth OBEX file transfer, and Bluetooth PAN / USB network tethering.
* **USB CDC-ACM Logging & Console**: 16 KiB non-blocking ring-buffered virtual serial console (`/dev/ttyACM0`) for kernel logs and interactive shell commands.
* **CRC16-Protected Virtual NVRAM**: Hardware-agnostic persistent preference storage with automatic corruption detection and fallback to factory defaults.
* **Multilingual Typography & i18n**: Multi-script font rendering supporting English (Latin), Russian (Cyrillic), and initial Indic script shaping for Hindi (Devanagari).

📖 *For a complete breakdown, see the [Author Note & Release Manifesto](AUTHOR_NOTE.md), [Full Features Showcase](docs/features.md), and [Release Notes](RELEASE_NOTES_v1.0.md).*

---

## System Architecture

```text
+-------------------------------------------------------------------------+
|                              APPLICATIONS                               |
|   Dialer | Contacts | Messages | Browser | Music | Gallery | Fun Zone   |
|   VAPPs: Space Shooter | Brick Breaker | Tetris Retro | Chip Synth      |
+-------------------------------------------------------------------------+
|                             WINDOW MANAGER                              |
|   Navigation Stack | Softkeys (LSK/RSK) | Task Pool | Fullscreen Engine |
+-------------------------------------------------------------------------+
|                            GRAPHICS & UI LAYER                          |
|   LVGL 9 UI Framework | GOUDA DMA Blitter | Declarative Screen Templates|
+-------------------------------------------------------------------------+
|                        OSAL & EMBEDDED KERNEL                           |
|   FreeRTOS / Linux Kernel | Event Queue | Virtual File System | NVRAM   |
+-------------------------------------------------------------------------+
|                    HARDWARE ABSTRACTION LAYER (HAL)                     |
|      OBTEL B10 (RDA8809 MIPS32)      |       Desktop SDL2 Simulator     |
|   ILI9225G LCD | GOUDA Blitter       |   SDL2 Display Framebuffer       |
|   Hardware Matrix Keypad             |   Keyboard Input Mapping         |
|   RDA ABB Audio DAC / Synth          |   SDL2 Audio Callback            |
+-------------------------------------------------------------------------+
```

---

## Quick Start

### 1. Run Desktop Simulator

#### Prerequisites (Debian / Ubuntu / Raspberry Pi OS)
```bash
sudo apt update
sudo apt install -y build-essential cmake libsdl2-dev
```

#### Build & Run
```bash
# Clone the repository
git clone https://github.com/vixxkigoli/VeebhaOS.git
cd VeebhaOS

# Launch the desktop simulator
make sim
```

### 2. Run Automated Test Verification
VeebhaOS features a comprehensive 60-phase automated test suite verifying typography, i18n, NVRAM persistence, window navigation, and 60 FPS arcade games across 1096 frames:
```bash
# Standard automated verification (Phase 1 to 60)
make test

# Memory safety & AddressSanitizer verification (0 leaks, 0 buffer overflows)
make asan
```

### 3. Build for Real Hardware (OBTEL B10)
```bash
# Build freestanding MIPS32 binary for RDA8809
make BOARD=obtel_b10
```
This produces `build/obtel_b10/veebha_os.bin` ready to flash to the OBTEL B10 phone using CoolTools/CoolWatcher over USB.

---

## Desktop Simulator Keypad Controls

| Feature Phone Key | PC Keyboard Key | Functionality |
| :--- | :--- | :--- |
| **D-Pad Up / Down / Left / Right** | Arrow Keys | Directional navigation / focus movement |
| **OK / Enter Key** | `Enter` / `Space` | Select / confirm action / primary action |
| **Left Softkey (LSK)** | `Left Alt` / `F1` | Left softkey action displayed on bottom bar |
| **Right Softkey (RSK)** | `Right Alt` / `F2` / `Escape` | Right softkey action (Back / Exit / Cancel) |
| **Keypad 0–9, \*, #** | Numpad or Top Row `0`–`9`, `*`, `#` | Numeric entry, T9 text input, game controls |
| **Call Key (Green)** | `C` | Short press: Dial / Long press (600ms): Quick Settings |
| **End / Power Key (Red)**| `E` | Return to Home / Exit app |
| **Star Key (\*)** | `*` | Long press (600ms): Multitasking Task Switcher |
| **Pound Key (#)** | `#` | Toggle T9 text entry mode (`Abc` -> `ABC` -> `123` -> `abc`) |

---

## Documentation

Comprehensive guides are available in the [`docs/`](docs/) directory:

* **Getting Started & Porting**:
  * [Porting Guide (FreeRTOS & Linux Kernel)](docs/porting-guide.md)
  * [Desktop Simulator Quickstart Guide](docs/getting-started/quickstart-simulator.md)
  * [OBTEL B10 Hardware Bringup & Flashing Guide](docs/getting-started/obtel-b10-hardware.md)
* **Architecture**:
  * [System Architecture Overview](docs/architecture/overview.md)
  * [Telephony & Baseband Modem Architecture](docs/architecture/modem-telephony.md)
  * [Strict Zero-FPU Integer Arithmetic Policy](docs/architecture/no-fpu-policy.md)
  * [Out-of-Tree BSP & Clean-Room Licensing](docs/architecture/bsp-architecture.md)
  * [Window Manager & Navigation Model](docs/architecture/window-manager.md)
* **Hardware & Diagnostics**:
  * [USB CDC-ACM Logging & Debug Console](docs/hardware/usb-log-console.md)
* **VAPP Developer SDK & Evolution**:
  * [VAPP Package Specification](docs/sdk/vapp-specification.md)
  * [Dynamic VAPP Evolution & OS Integration](docs/specs/dynamic-vapp-evolution.md)
  * [Universal Multi-Screen Layout Guidelines](docs/sdk/universal-screens.md)
  * [Creating Your First 2D Arcade Game (Tutorial)](docs/sdk/creating-your-first-game.md)
  * [Core SDK API Reference](docs/sdk/api-reference.md)
* **Features & Releases**:
  * [Full Features Showcase](docs/features.md)
  * [Release Notes v1.0](RELEASE_NOTES_v1.0.md)
* **Contributing**:
  * [Coding Standards & Verification](docs/contributing/coding-standards.md)

---

## License

VeebhaOS is open-source software licensed under the **MIT License**. See [LICENSE](LICENSE) for details.

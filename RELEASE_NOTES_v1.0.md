# VeebhaOS v1.0 — Feature Showcase & System Overview

> **VeebhaOS** is a lightweight, zero-coordinate, embedded RTOS-based operating system designed for feature phones and resource-constrained microcontrollers. Engineered with a strict **Zero-Floating-Point (Integer-only)** policy, VeebhaOS delivers a modern feature phone user experience on both bare-metal RTOS (FreeRTOS / RDA8809) and the Linux kernel.
>
> 📜 *Read the [Release Manifesto & Author Note](AUTHOR_NOTE.md) by Sachin Arunrao Borkar.*

---

## 1. System Shell & Core User Experience

### Universal Quick Settings & Notification Drawer
* **Accessible Anywhere**: Press and hold the **CALL** key in any app or game to instantly pull down the dual-page drawer.
* **Instant Toggle**: A short press of the **CALL** key switches seamlessly between incoming Notifications and the Quick Settings 3×2 grid (Bluetooth, Backlight, Sound Profiles, Torch, Battery, Settings shortcut).
* **Now-Playing Media Card**: Play, pause, and track media progress without leaving your active game.

<div align="center">

| **Quick Settings with Media Control** | **One-Line Notification Deck** |
| :---: | :---: |
| <img src="docs/screenshots/qs_panel_settings_music_playing.png" width="176" alt="Quick Settings Panel with Media" /> | <img src="docs/screenshots/qs_notification.png" width="176" alt="One-Line Notification Deck" /> |

</div>

### Preemptive Multitasking & Task Switcher
* **Hardware Long-Press Switcher**: Hold the **`*`** key anywhere in the OS to open the visual card Task Manager.
* **Background & Resume**: Switch between active running apps, pause background tasks, kill memory hogs, or jump instantly to the Idle Home Screen.

<div align="center">

| **Multitasking Card Switcher (3 Tasks Running)** |
| :---: |
| <img src="docs/screenshots/task_switcher_having3_tskrunning.png" width="176" alt="Task Switcher" /> |

</div>

### Dynamic Idle Screen Live Pill
* **Priority Status Bar Pill**: Contextual interactive badge on the Idle screen displaying real-time activity:
  - 📞 Ongoing phone call with live duration
  - 🎵 Music track title & artist (Walkman)
  - 📻 Active FM Radio frequency
  - ⏱ Countdown timer ticks
* **One-Click Jump**: Press **OK** on the idle pill to jump straight into the running background application.

<div align="center">

| **Idle Screen with Live Pill & Wallpaper** |
| :---: |
| <img src="docs/screenshots/idle_screen_live_pill.png" width="176" alt="Idle Screen Live Pill" /> |

</div>

---

## 2. Display, Themes & Typography

### 4 Distinct Built-in Themes
Instant system-wide color palette switching persisted across reboots:
1. **Dark Cyan** (Default): Slate dark `#121212` background with vibrant cyan accents.
2. **OLED Black**: Pure `#000000` pixel-off contrast for maximum battery efficiency.
3. **Clean Light**: Crisp chalk `#F1F5F9` background with deep slate-blue highlights.
4. **High-Contrast B&W**: Stark monochrome styling designed for ultra-low vision and direct sunlight readability.

<div align="center">

| **Dark Cyan (Default)** | **OLED Black** | **Clean Light** | **High-Contrast B&W** |
| :---: | :---: | :---: | :---: |
| <img src="docs/screenshots/main_menu_cyan_theme.png" width="165" alt="Dark Cyan Theme" /> | <img src="docs/screenshots/main_menu_oled_theme.png" width="165" alt="OLED Black Theme" /> | <img src="docs/screenshots/main_menu_light_theme.png" width="165" alt="Clean Light Theme" /> | <img src="docs/screenshots/main_menu_bw_theme.png" width="165" alt="High Contrast B&W Theme" /> |

</div>

### Multilingual Typography & Font Shaper
* Native UTF-8 string support with fallback glyph rendering.
* Full support for **English** (Latin) and **Russian** (Cyrillic).
* Embedded Indic script infrastructure for **Hindi** (Devanagari).

<div align="center">

| **Language Selection** | **High-Contrast Monochrome Applied** |
| :---: | :---: |
| <img src="docs/screenshots/settings_languages.png" width="176" alt="Language Selection" /> | <img src="docs/screenshots/high_bw_theme_applied.png" width="176" alt="High Contrast Monochrome Theme" /> |

</div>

---

## 3. Storage, Configuration & Hardware Testing

### CRC16-Protected Virtual NVRAM
* Hardware-agnostic persistent settings engine backed by flash or local storage.
* Stores sound profile, language, theme palette, wallpaper, Bluetooth state, and alarm timers.
* Automatically verifies data integrity with a CCITT CRC16 checksum on every boot, cleanly reverting to defaults if corrupted.

<div align="center">

| **Settings Menu** | **Theme Selector** | **Wallpaper Selection** | **About Phone (Hardware)** |
| :---: | :---: | :---: | :---: |
| <img src="docs/screenshots/settings.png" width="165" alt="Settings Menu" /> | <img src="docs/screenshots/settings_theme_list.png" width="165" alt="Theme Selector" /> | <img src="docs/screenshots/wallpaper_selection_settings.png" width="165" alt="Wallpaper Selection" /> | <img src="docs/screenshots/settings_about_phone.png" width="165" alt="About Phone Hardware Details" /> |

</div>

### Real Hardware Tested Subsystems
Verified on real silicon (RDA8809 / OBTEL B10) and desktop simulator:
* **Display**: 176×220 16-bit RGB565 LCD (ILI9225G) driven by **RDA GOUDA 2D Hardware DMA** blitter.
* **Keypad**: 4×5 hardware matrix with full 5-way D-Pad, dual Softkeys (`LSK`/`RSK`), and Call/End keys.
* **Power Management**: Live ADC battery voltage fuel-gauge and USB charger detect.
* **PWM Backlight & Torch**: Multi-level brightness scaling and hardware LED flashlight.
* **USB Debugging**: 16 KiB non-blocking ring-buffered USB CDC-ACM virtual serial console (`/dev/ttyACM0`) for kernel logs and live diagnostics.

---

## 4. Connectivity & Internet Lite

### Tiny Web Browser
* Built-in lightweight HTML/WAP browser engine.
* Parses web links, forms, and formatted paragraphs over Bluetooth PANU tethering.
* Supports URL history, Back navigation, and web search shortcuts.

<div align="center">

| **Tiny Web Browser (WAP/HTTP)** | **Bluetooth Settings** | **Connectivity / Tethering** |
| :---: | :---: | :---: |
| <img src="docs/screenshots/browser.png" width="176" alt="Tiny Web Browser" /> | <img src="docs/screenshots/settings_bluetooth.png" width="176" alt="Bluetooth Settings" /> | <img src="docs/screenshots/settings_connectivity.png" width="176" alt="Connectivity Settings" /> |

</div>

### Bluetooth 2.1+EDR Stack
* **OBEX File Transfer**: Send and receive `.vapp` binaries and images wirelessly.
* **BNEP / PAN**: Mobile Internet tethering.

---

## 5. Built-in Apps & Fun Zone Store

### VeebhaOS Fun Zone (`.vapp` Dynamic Loader)
Single-file dynamic binary app store with zero compilation overhead:
* **Brick Breaker**: 60 FPS retro arcade breakout game.
* **Tetris Retro**: Complete 10×16 matrix block stacker with rotation physics.
* **Space Shooter**: Full-screen scrolling galactic space combat.
* **Tools & Utilities**: Chip Synth, Morse Flasher, Unit Converter, and System Resource Monitor.

<div align="center">

| **Fun Zone App Store** | **Brick Breaker (60 FPS)** | **Tetris Retro** | **Space Shooter** |
| :---: | :---: | :---: | :---: |
| <img src="docs/screenshots/vebhaos_fun_zone.png" width="165" alt="Fun Zone Store" /> | <img src="docs/screenshots/brickgame_in_funzone.png" width="165" alt="Brick Breaker" /> | <img src="docs/screenshots/tetris_in_funzone.png" width="165" alt="Tetris Retro" /> | <img src="docs/screenshots/space_shooter_gamein_funzone.png" width="165" alt="Space Shooter" /> |

</div>

### Essential Feature Phone Suite
* **Telephony Suite**: Dialer with quick-match, SMS inbox with T9 text editor, Contacts directory, and In-Call HUD.
* **Tools Suite**: Stopwatch & Countdown Timer with mode toggling, Alarm Clock with high-contrast field picking, Voice/Audio Recorder, Calculator, and File Manager.
* **Media**: Walkman Music Player with MP3/WAV playback, FM Radio tuner, and Photo Gallery.

<div align="center">

| **Walkman Music Player** | **FM Radio Tuner** | **Tools Suite** | **File Manager** |
| :---: | :---: | :---: | :---: |
| <img src="docs/screenshots/walkman_music_play.png" width="165" alt="Walkman Player" /> | <img src="docs/screenshots/walkman_fm_radio.png" width="165" alt="FM Radio" /> | <img src="docs/screenshots/tools.png" width="165" alt="Tools Suite" /> | <img src="docs/screenshots/file_manager.png" width="165" alt="File Manager" /> |

</div>

---

## 6. Architecture & Portability

* **Zero-FPU Guarantee**: 100% fixed-point integer math — zero floating-point registers used.
* **Dual-Target Portability**: Runs natively on bare-metal RTOS (FreeRTOS) and Linux kernel (fbdev/evdev).
* **Decoupled Out-of-Tree BSP**: Clean-room MIT core decoupled from proprietary vendor baseband SDKs.

---

## 7. Live Hardware Execution (OBTEL B10 Silicon)

The photographs below verify **VeebhaOS v1.0 running on physical silicon** — an OBTEL B10 feature phone powered by the RDA8809 MIPS32r1 CPU with 8 MB RAM, 176×220 LCD, and hardware matrix keypad:

<div align="center">

| **Idle Screen Live Pill** | **App Launcher Grid** | **Task Switcher** |
| :---: | :---: | :---: |
| <img src="docs/screenshots/obtel_b10/idle_screen.jpg" width="200" alt="Idle Screen Live Pill" /> | <img src="docs/screenshots/obtel_b10/main_menu.jpg" width="200" alt="App Launcher Grid" /> | <img src="docs/screenshots/obtel_b10/task_switcher.jpg" width="200" alt="Task Switcher" /> |

| **Brick Breaker on LCD** | **Tetris Retro Action** | **About Phone Specs** |
| :---: | :---: | :---: |
| <img src="docs/screenshots/obtel_b10/brick_game1.jpg" width="200" alt="Brick Breaker" /> | <img src="docs/screenshots/obtel_b10/tetris_1.jpg" width="200" alt="Tetris Retro" /> | <img src="docs/screenshots/obtel_b10/about_phone1.jpg" width="200" alt="About Phone Specs" /> |

</div>


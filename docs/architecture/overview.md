# VeebhaOS Architecture Overview

This document describes the architectural layers and subsystem interactions comprising VeebhaOS.

---

## 1. Five-Layer Architecture

VeebhaOS is organized into five strict vertical layers to ensure modularity, portability, and clean separation between hardware-specific drivers and portable application logic:

```text
+------------------------------------------------------------------+
| Layer 5: Applications & VAPPs                                    |
|   Core Apps (Dialer, Messages, Contacts, Browser, Music)        |
|   VAPPs (Space Shooter, Brick Breaker, Tetris Retro, Synth)     |
+------------------------------------------------------------------+
| Layer 4: Window Manager & Shell                                  |
|   Screen Navigation Stack | Softkey Routing | Task Pool         |
|   Fullscreen Viewport Engine | Notification & Settings Drawers   |
+------------------------------------------------------------------+
| Layer 3: Graphics & Typography Pipeline                          |
|   LVGL 9 Embedded Graphics | Declarative Screen Templates       |
|   Multi-Script Unicode Font Shaper (Devanagari, Cyrillic, Latin) |
+------------------------------------------------------------------+
| Layer 2: Operating System Abstraction Layer (OSAL) & Kernel      |
|   FreeRTOS Kernel | Event Queue | Virtual File System (VFS)      |
|   NVRAM Storage (CRC16 Protected) | Power Manager                |
+------------------------------------------------------------------+
| Layer 1: Hardware Abstraction Layer (HAL)                        |
|   Display Controller (ILI9225G / SDL2) | GOUDA 2D DMA Blitter   |
|   Matrix Keypad Driver | Audio DAC Synthesizer | Timer / IRQ     |
+------------------------------------------------------------------+
```

---

## 2. Kernel & Multitasking Model

VeebhaOS runs on **FreeRTOS v10** configured for preemptive multitasking with a strict tick rate of 1000 Hz:

* **Task Prioritization**:
  * `Priority 4 (Highest)`: Display Refresh & Hardware IRQ Task (handles DMA blit completion and audio streaming).
  * `Priority 3`: Core GUI & Window Manager Task (processes LVGL rendering and user input events).
  * `Priority 2`: Telephony & Connectivity Background Task (cellular mock, Bluetooth OBEX/BNEP).
  * `Priority 1 (Lowest)`: System Idle & Power Management Task (puts CPU into sleep mode when event queues are empty).

* **Zero Dynamic Allocation Rule in Drivers**:
  * Drivers and core kernel structures are statically allocated or pool-managed.
  * System memory allocations undergo bounded verification with no heap fragmentation.

---

## 3. Graphics Pipeline & Hardware Blitter

* **Display Resolution**: 176 × 220 pixels portrait, 16-bit RGB565 color format (2 bytes per pixel).
* **Direct DMA Blitting**:
  * On baremetal hardware (OBTEL B10), the **RDA GOUDA DMA engine** pushes pixels directly from RAM to the ILI9225G LCD without blocking the CPU.
  * In the simulator, the frame is transferred to an SDL2 streaming texture.
* **Declarative Template System**:
  Standard applications construct screens using five reusable declarative templates:
  1. `tpl_list`: Scrollable vertical menu with icons, titles, and subtexts.
  2. `tpl_grid`: Multi-column application launcher grid with icon navigation.
  3. `tpl_dialog`: Modal dialog overlays for confirmation, alerts, and notifications.
  4. `tpl_editor`: Multi-line text composer with T9 predictive input.
  5. `tpl_media`: Media playback screen with progress bars and album art.

---

## 4. NVRAM & Persistent Storage

User settings, network configurations, paired Bluetooth devices, and call history are stored in a dedicated NVRAM partition protected by CCITT CRC16 checksums:

```c
typedef struct {
    uint16_t version;
    uint16_t crc16;
    uint8_t  active_language_id;
    uint8_t  sound_profile;
    uint8_t  display_brightness;
    uint8_t  wallpaper_mode;
    char     wallpaper_path[64];
    bool     bluetooth_enabled;
    ...
} veebha_nvram_store_t;
```

If the CRC16 fails on boot (e.g. following corruption or initial factory flash), the system safely falls back to factory defaults without crashing.

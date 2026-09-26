# Out-of-Tree BSP & Clean-Room Licensing Architecture

This document describes the architectural decoupling of hardware Board Support Packages (BSPs) from the core VeebhaOS operating system repository.

---

## 1. Clean-Room & Licensing Rationale

VeebhaOS is licensed under the **MIT License** and adheres to a strict clean-room open-source standard.

Silicon vendor baseband SDKs (such as RDA/Coolsand, MediaTek, and Unisoc) often contain:
1. **Proprietary Vendor Code & Blobs**: Precompiled binary objects (e.g. `mgr_ecc.o`), non-redistributable header files, and NDA-restricted baseband registers.
2. **Reverse-Engineered Drivers**: Register-level drivers derived from reverse engineering or vendor leaks that cannot legally reside in a clean-room MIT repository.

To protect the core operating system and third-party contributors, **vendor-proprietary drivers and hardware-specific blobs are never hosted directly in the main VeebhaOS repository**. Instead, VeebhaOS uses an **Out-of-Tree BSP Architecture**.

---

## 2. Repository Separation

```text
+--------------------------------------------------------------------------+
|                  MAIN REPOSITORY (vixxkigoli/VeebhaOS)                    |
|                      100% Clean-Room Open-Source (MIT)                   |
|                                                                          |
|  * Core Kernel & OSAL (FreeRTOS v10, Event Queue, VFS, NVRAM)            |
|  * Window Manager & Declarative Templates (tpl_list, tpl_grid, dialogs)  |
|  * Typography, Font Shaper & Multi-language i18n                         |
|  * Built-in Apps (Dialer, Messages, Contacts, Browser, Music Player)     |
|  * VAPP Sandbox & Fun Zone Arcade Suite (Space Shooter, Tetris, Bricks)  |
|  * Reference Hardware Target: Desktop Simulator (boards/simulator/)      |
|  * Generic FreeRTOS Port & Linker Templates (boards/port_generic/)       |
|  * Hardware Abstraction Layer (HAL) Interface Contracts (drivers/)       |
+--------------------------------------------------------------------------+
                                    ▲
                                    │ Clean HAL Contract
                                    │ (hal_display.h, hal_keypad.h, hal_audio.h)
                                    ▼
+--------------------------------------------------------------------------+
|                 EXTERNAL BSP REPOSITORY (e.g. veebha-bsp-obtel_b10)      |
|                      Target-Specific Hardware Drivers                    |
|                                                                          |
|  * RDA8809 Silicon Drivers (hal_gouda.c, hal_pmd.c, hal_sys_ctrl.c)      |
|  * Vendor Register Definitions & Baseband Headers (include/)             |
|  * Precompiled Hardware Blobs (mgr_ecc.o) & SRAM Assembly (isram_stub.S) |
|  * ILI9225G LCD & Matrix Keypad Hardware Initializers (lcd_ili9225g.c)   |
|  * Hardware Entrypoint & Board Initialization (main_obtel_b10.c)         |
+--------------------------------------------------------------------------+
```

---

## 3. What Stays in the Main VeebhaOS Repository

The main repository contains everything required to build, test, and run VeebhaOS on the desktop simulator, plus the standard porting interfaces:

1. **Hardware Abstraction Layer (HAL) Headers (`drivers/`)**:
   - `hal_display.h`: Display resolution, frame flushing, blit hooks.
   - `hal_input.h`: Matrix keypad scanning and event injection.
   - `hal_audio.h`: Tone generation, DAC control, and polyphonic buzzer.
   - `hal_power.h`: Battery ADC voltage measurement and charging status.
   - `hal_storage.h`: SD card and NVRAM flash drivers.
2. **Board Configuration Switchboard (`boards/board_config.h`)**:
   - Central capability flags (`CONFIG_BOARD_SUPPORT_BLUETOOTH`, `CONFIG_STATUS_BAR_HEIGHT`).
3. **Generic FreeRTOS Port Files (`kernel/freertos/portable/`)**:
   - Standard open-source FreeRTOS MIPS32 / ARM Cortex-M ports (`port.c`, `portmacro.h`, `heap_4.c`).
4. **Reference Target (`boards/simulator/`)**:
   - Full native SDL2 desktop simulator for Linux, macOS, and WSL.

---

## 4. What Lives in the External BSP Repository

All vendor-specific and reverse-engineered hardware code is maintained in an external board repository (e.g. `boards/obtel_b10` or a separate git repo):

* **Vendor Driver Implementations**: `hal_gouda.c`, `hal_pmd.c`, `hal_sys_ctrl.c`, `hal_bt.c`, `hal_usb_cdc.c`, `hal_sdmmc.c`, `lcd_ili9225g.c`.
* **Vendor Register Headers**: `include/sys_ctrl.h`, `include/globals.h`, `include/cs_types.h`, `include/hal_pmd.h`, etc.
* **Proprietary Blobs & ASM**: `mgr_ecc.o`, `isram_stub.S`, `os_vector_table.c`.
* **Target Makefile & Linker**: Board-specific GCC flags, memory addresses, and flash offsets.

---

## 5. Build System Integration

The root `Makefile` automatically detects whether the target BSP is located locally in `boards/<board_name>` or in an external directory via `BSP_DIR`:

```bash
# Option 1: BSP cloned directly into boards/obtel_b10
make BOARD=obtel_b10

# Option 2: BSP located in an external directory
make BOARD=obtel_b10 BSP_DIR=/path/to/veebha-bsp-obtel_b10
```

If the BSP is not present, the build system gracefully informs the user with instructions on where to obtain the hardware drivers without breaking the core repository.

# VeebhaOS Porting Guide (FreeRTOS & Linux Kernel)

VeebhaOS is a lightweight, zero-coordinate, embedded RTOS-based feature phone operating system. While natively tailored for resource-constrained microcontrollers and baseband processors (e.g. RDA8809 with FreeRTOS), its core UI shell, window manager, and application runtime are cleanly decoupled from the underlying hardware via OSAL (OS Abstraction Layer) and HAL (Hardware Abstraction Layer).

Consequently, VeebhaOS can run across two primary runtime environments:
1. **Bare-metal RTOS** (FreeRTOS, RT-Thread, Zephyr, or vendor RTOS).
2. **Linux Kernel** (Embedded Linux using `/dev/fb0` or DRM/KMS and `/dev/input/event*` evdev).

---

## 1. System Architecture & Porting Boundaries

```text
+--------------------------------------------------------------------------+
|                 VeebhaOS Shell & Application Stack                       |
|   Window Manager | Declarative Templates (List, Grid, Dialog, Editor)    |
|   Built-in Phone Apps (Dialer, SMS, Contacts) | VAPP Sandbox (.vapp)    |
+--------------------------------------------------------------------------+
                                    ▲
                                    │ Clean HAL / OSAL APIs
                                    ▼
+-----------------------------------+--------------------------------------+
|       FreeRTOS / RTOS Port        |         Linux Kernel Port            |
+-----------------------------------+--------------------------------------+
| * Display: Direct SPI / DMA / LCD | * Display: /dev/fb0 or DRM/KMS       |
| * Input: Matrix Keypad Scan / IRQ | * Input: Linux evdev /dev/input/event|
| * Ticks: FreeRTOS Tick Hook       | * Ticks: clock_gettime(CLOCK_MONO)   |
| * Memory: Static Heap (heap_4)    | * Memory: Standard glibc malloc      |
| * Storage: Raw Flash / FatFS      | * Storage: POSIX filesystem mounts   |
| * Audio: Hardware DAC / PWM       | * Audio: ALSA / PulseAudio / /dev/dsp|
+-----------------------------------+--------------------------------------+
```

### Hardware Baseline Requirements
* **Display**: Standard 176 × 220 px portrait (configurable), 16-bit RGB565.
* **RAM**: Minimum 2 MB (recommended 4 MB for dynamic VAPP loader).
* **FPU**: **Zero Floating-Point Policy** — all coordinate math and drivers must use fixed-point / integer arithmetic.

---

## 2. Porting to FreeRTOS (Embedded & Baseband SoCs)

Use this path when porting to bare-metal SoCs, cellular baseband chips, or MCUs (e.g. RDA8809, ESP32, STM32, Cortex-M, MIPS32).

### Step 2.1: FreeRTOS Task & Tick Configuration
In your board entrypoint (`main.c`):
1. **Tick Hook**: Call `lv_tick_inc(1)` inside `vApplicationTickHook()` or from a hardware timer interrupt configured for 1000 Hz.
2. **GUI Task**: Create a dedicated UI task with at least 8 KB stack:
```c
void gui_task(void *pvParameters)
{
    hal_display_init();
    hal_input_init();
    veebha_init(); /* Initializes window manager and home screen */

    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5)); /* Run UI event loop at ~100-200 Hz */
    }
}
```

### Step 2.2: Display Driver (`hal_display.c`)
Implement the display initialization and flush callback:
```c
static void disp_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint16_t w = area->x2 - area->x1 + 1;
    uint16_t h = area->y2 - area->y1 + 1;

    /* Push pixels via SPI / 8080 parallel bus or hardware DMA */
    lcd_draw_rect(area->x1, area->y1, w, h, (const uint16_t *)px_map);

    /* Signal completion back to LVGL */
    lv_display_flush_ready(disp);
}

bool hal_display_init(void)
{
    lcd_hardware_init();
    lv_display_t *disp = lv_display_create(176, 220);
    static uint16_t buf[176 * 20]; /* 20-line partial draw buffer */
    lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, disp_flush_cb);
    return true;
}
```

### Step 2.3: Keypad Driver (`hal_input.c`)
Periodically scan the hardware keypad matrix or listen to key change interrupts. Map physical keys to `veebha_key_t` and inject them:
```c
#include "drivers/hal_input.h"

void keypad_on_key_change(uint8_t row, uint8_t col, bool pressed)
{
    veebha_key_t key = matrix_lookup(row, col);
    veebha_key_state_t state = pressed ? VEEBHA_KEY_STATE_PRESSED : VEEBHA_KEY_STATE_RELEASED;

    hal_input_push_event(key, state);
}
```

### Step 2.4: Storage & NVRAM
* **NVRAM**: Implement `os_nvram_read()` and `os_nvram_write()` backed by a dedicated 4 KB or 8 KB flash sector.
* **VFS / SD Card**: Mount FatFS or LittleFS under `/sdcard` and `/vapps`.

---

## 3. Porting to Linux Kernel (Embedded Linux / Single Board Computers)

Use this path when porting to Linux-based feature phones, smart devices, or Single Board Computers (Allwinner, Rockchip, Broadcom, NXP i.MX). VeebhaOS executes as a standard Linux userspace process or `/sbin/init` replacement.

### Step 3.1: Display via Framebuffer (`/dev/fb0`) or DRM/KMS
Use LVGL's Linux framebuffer integration or map `/dev/fb0`:
```c
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

static int fb_fd = -1;
static uint16_t *fb_ptr = NULL;
static struct fb_var_screeninfo vinfo;

bool hal_display_init(void)
{
    fb_fd = open("/dev/fb0", O_RDWR);
    ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);
    fb_ptr = (uint16_t *)mmap(NULL, vinfo.yres * vinfo.xres * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);

    lv_display_t *disp = lv_display_create(vinfo.xres, vinfo.yres);
    lv_display_set_buffers(disp, fb_ptr, NULL, vinfo.yres * vinfo.xres * 2, LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(disp, linux_fb_flush_cb);
    return true;
}
```

### Step 3.2: Keypad via Linux evdev (`/dev/input/event*`)
Spawn a background thread reading Linux input event structures:
```c
#include <linux/input.h>

void *input_thread(void *arg)
{
    int ev_fd = open("/dev/input/event0", O_RDONLY);
    struct input_event ev;

    while (read(ev_fd, &ev, sizeof(ev)) > 0) {
        if (ev.type != EV_KEY) continue;

        veebha_key_t key = VEEBHA_KEY_NONE;
        switch (ev.code) {
            case KEY_ENTER:     key = VEEBHA_KEY_OK; break;
            case KEY_BACKSPACE: key = VEEBHA_KEY_RSK; break;
            case KEY_COMPOSE:   key = VEEBHA_KEY_LSK; break;
            case KEY_UP:        key = VEEBHA_KEY_UP; break;
            case KEY_DOWN:      key = VEEBHA_KEY_DOWN; break;
            case KEY_LEFT:      key = VEEBHA_KEY_LEFT; break;
            case KEY_RIGHT:     key = VEEBHA_KEY_RIGHT; break;
            case KEY_0 ... KEY_9: key = VEEBHA_KEY_NUM_0 + (ev.code - KEY_0); break;
            default: break;
        }

        if (key != VEEBHA_KEY_NONE) {
            hal_input_push_event(key, ev.value ? VEEBHA_KEY_STATE_PRESSED : VEEBHA_KEY_STATE_RELEASED);
        }
    }
    return NULL;
}
```

### Step 3.3: Filesystem & Persistence
* Mount standard Linux directories:
  * `/sdcard` -> `/media/sdcard` or `./storage/sdcard`
  * `/vapps` -> `/usr/share/veebha/vapps` or `./vapps`
* NVRAM file backend: persist `veebha_nvram_store_t` to a binary file (`/etc/veebha/nvram.dat`).

---

## 4. Board Profile Registration

Register your target in [`boards/board_config.h`](file:///home/vixxkigoli/pm/VeebhaOS/boards/board_config.h):
```c
#if defined(CONFIG_BOARD_MYDEVICE) && CONFIG_BOARD_MYDEVICE
    #define CONFIG_DISP_HOR_RES         176
    #define CONFIG_DISP_VER_RES         220
    #define CONFIG_STATUS_BAR_HEIGHT     18
    #define CONFIG_SOFTKEY_BAR_HEIGHT    18
    #define CONFIG_BOARD_SUPPORT_BLUETOOTH 1
#endif
```

Compile with:
```bash
make BOARD=mydevice
```

---

## 5. Porting Verification Checklist

- [ ] **Frame Timing**: `lv_timer_handler()` invoked at 5-10ms intervals without starvation.
- [ ] **Keypad Navigation**: Softkeys (`LSK`, `RSK`), 5-way D-Pad, and numeric keys cycle smoothly through list templates.
- [ ] **Modal Alerts**: Dialogs pop up and dismiss via `RSK` / `OK` without leaking on `lv_layer_top()`.
- [ ] **Zero-FPU**: Code compiled with `-msoft-float` or verified with zero hardware floating-point instructions.
- [ ] **NVRAM CRC16**: Settings persist across reboot/power-cycle without corruption.

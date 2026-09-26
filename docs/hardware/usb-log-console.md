# USB CDC-ACM Serial Logging & Debug Console

VeebhaOS features a built-in, bare-metal USB Communication Device Class (CDC-ACM) virtual serial port driver. This interface enables real-time kernel and user-space logging, diagnostic tracing, and interactive debug shell console access when connected to a host computer over a standard Micro-USB or USB-C cable.

---

## 1. Architecture Overview

```text
+--------------------------------------------------------------------------+
|                      VeebhaOS Application & Kernel                       |
|   OS_LOGI / OS_LOGW / OS_LOGE  |  os_log_printf()  |  Crash Dumper       |
+--------------------------------------------------------------------------+
                                     │
                                     ▼
+--------------------------------------------------------------------------+
|                  16 KiB Non-Blocking RAM Ring Buffer                     |
|      (Buffers log records even when USB is disconnected or suspended)    |
+--------------------------------------------------------------------------+
                                     │
                                     ▼
+--------------------------------------------------------------------------+
|                    Hardware USB CDC-ACM Driver                           |
|       EP0 (Control)  |  EP1 IN (Interrupt)  |  EP2 IN / EP3 OUT (Bulk)   |
+--------------------------------------------------------------------------+
                                     │ USB D+/D- Cable
                                     ▼
+--------------------------------------------------------------------------+
|                     Host PC Terminal (Linux / macOS / Windows)           |
|      /dev/ttyACM0 (Linux)  |  cu.usbmodem* (macOS)  |  COMx (Windows)    |
+--------------------------------------------------------------------------+
```

### Key Technical Characteristics
* **Standard Class**: USB 2.0 Full-Speed (12 Mbps), CDC-ACM (Class `0x02`, Subclass `0x02`).
* **Endpoints**:
  * `EP0` (IN/OUT, 64-byte): Control transfers & standard USB descriptor negotiation.
  * `EP1` (IN, 10-byte): Interrupt notification endpoint (serial state notification).
  * `EP2` (IN, 64-byte): Bulk IN data transmission (device-to-host log output).
  * `EP3` (OUT, 64-byte): Bulk OUT command reception (host-to-device console input).
* **RAM Buffering**: 16 KiB dedicated static circular ring buffer (`LOG_RING_BUFFER_SIZE = 16384`). Logs generated during boot, sleep, or before the host opens the port are preserved and flushed once the USB host binds.
* **Zero Overhead During Suspend**: If the USB port is suspended or disconnected, log calls drop older overflow bytes without blocking FreeRTOS tasks or stalling the graphics pipeline.

---

## 2. Connecting to the Debug Console

### Linux
When connecting the device to a Linux host, the kernel registers a CDC-ACM device automatically:

```bash
# Verify device detection
dmesg | tail -n 20
# Output:
# cdc_acm 1-2:1.0: ttyACM0: USB ACM device

# Connect using picocom or minicom (115200 baud, 8-N-1):
picocom -b 115200 /dev/ttyACM0

# Or using screen:
screen /dev/ttyACM0 115200
```

> [!TIP]
> If you get `Permission denied` when accessing `/dev/ttyACM0`, add your Linux user to the `dialout` group:
> ```bash
> sudo usermod -aG dialout $USER
> ```

### macOS
On macOS, the port enumerates under `/dev/cu.usbmodem*`:
```bash
ls /dev/cu.usbmodem*
screen /dev/cu.usbmodem14101 115200
```

### Windows
On Windows 10/11, the native `usbser.sys` driver automatically binds to CDC-ACM devices without requiring INF files:
1. Open **Device Manager** and check **Ports (COM & LPT)** -> `USB Serial Device (COMx)`.
2. Connect using **PuTTY**, **Tera Term**, or Windows Terminal at `115200` baud.

---

## 3. Log Output Format & API Usage

VeebhaOS uses standard structured log levels:

```c
#include "sdk/include/veebha_log.h"

#define TAG "MY_APP"

void sample_function(void)
{
    OS_LOGI(TAG, "Application initialized successfully");
    OS_LOGW(TAG, "Battery level below threshold: %u%%", 15);
    OS_LOGE(TAG, "Failed to mount SD card partition (err=%d)", -1);
}
```

### Terminal Output
```text
[00:00:01.120] [INFO]  [APP_ALARM]: Alarm Clock module initialized (07:00 AM, enabled=0)
[00:00:01.240] [INFO]  [VAPP_LOADER]: Initializing VAPP package manager (Mount: /vapps)
[00:00:01.245] [INFO]  [VAPP_LOADER]: Scanned /vapps: found 8 valid package(s)
[00:00:01.310] [INFO]  [WIN_MGR]: Pushed screen 'Idle' 0x81045A10 (New Depth: 1)
```

---

## 4. USB Console Commands

When the terminal session is active, typing commands into the serial terminal interacts directly with the VeebhaOS diagnostic shell:

| Command | Description | Output |
| :--- | :--- | :--- |
| `help` | Lists all available console commands | List of built-in debug commands |
| `tasks` | Displays FreeRTOS task status & stack usage | Task Name, State, Priority, Stack High Watermark |
| `mem` | Reports RAM, PSRAM, and LVGL heap statistics | Total RAM, Free Heap, Peak Allocation |
| `vapps` | Lists installed `.vapp` binaries on storage | Package IDs, Types, Heap requirements |
| `nvram` | Dumps NVRAM calibration parameters & CRC16 | Theme, Language, Profile, CRC status |
| `reboot` | Performs clean software reboot | Initiates hardware watchdog reboot |

---

## 5. Troubleshooting

1. **Device not detected on host**:
   - Ensure the cable is a **4-wire data cable**, not a charge-only 2-wire cable.
   - Check if the device is stuck in USB Download / Bootloader mode (`1782:4d00`) instead of normal runtime mode.
2. **Missing early boot logs**:
   - The initial 16 KiB of boot logs are stored in RAM. Once your terminal connects and asserts DTR, the circular buffer flushes all buffered messages immediately.
3. **Log buffer overflow under heavy debug**:
   - If log volume exceeds USB transmission rates, oldest messages are dropped cleanly with a `[LOG OVERFLOW]` tag without crashing the system.

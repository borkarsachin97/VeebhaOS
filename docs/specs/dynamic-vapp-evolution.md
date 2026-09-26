# Dynamic VAPP Evolution & OS Integration Specification

This specification outlines the architectural roadmap to evolve the `.vapp` (*Veebha Application Package*) format from a simple relocatable binary into a **dynamic, reliable, and deeply integrated application runtime** for VeebhaOS.

---

## 1. Evolution Objectives

1. **Dynamic Symbol Resolution**: Eliminate hardcoded absolute RAM link addresses by introducing a dynamic relocation table and OS syscall import table.
2. **Deep OS Integration**: Enable VAPPs to post system notifications, play polyphonic audio/haptics, persist high scores/data, and hook into the Idle Screen Live Pill.
3. **Fault-Tolerant Sandboxing**: Protect the OS from rogue or buggy VAPPs via bounded heap quotas, memory leak tracking, and crash-recovery traps.

---

## 2. VAPP ABI v2 Container Structure

```text
+--------------------------------------------------------------------------+
|  VAPP Header v2 (256 bytes)                                              |
|  - Magic ('VAPP' / 0x50504156), ABI Version = 2                          |
|  - App Metadata (Name, Author, Version, Permissions, Heap Budget)        |
|  - Section Offsets: Code, Data, Import Table, Asset Bundle               |
|  - Checksums: Header CRC16 + Full Package SHA-256 / CRC32                |
+--------------------------------------------------------------------------+
|  OS Syscall Import Table                                                 |
|  - Array of requested OS Function IDs (Window Mgr, LVGL, Audio, Storage) |
+--------------------------------------------------------------------------+
|  Executable Code Section (.text)                                         |
|  - Position-Independent Code (PIC) or Relocation Table Entries           |
+--------------------------------------------------------------------------+
|  Initialized Data & String Tables (.rodata, .data)                       |
+--------------------------------------------------------------------------+
|  Embedded Asset Archive (.assets)                                        |
|  - Sound tables, sprites, icons, and level maps                          |
+--------------------------------------------------------------------------+
```

---

## 3. Dynamic OS Services & API Interface (`vapp_api_t`)

When a VAPP launches, the OS Loader passes an API dispatch table (`vapp_api_t`) to the application entrypoint. The VAPP calls OS services safely through this contract:

```c
typedef struct {
    uint32_t api_version;

    /* 1. Window & UI Subsystem */
    lv_obj_t * (*create_screen)(void);
    void       (*set_softkeys)(const char *lsk, void (*lsk_cb)(void), const char *rsk, void (*rsk_cb)(void));
    void       (*show_dialog)(const char *title, const char *msg);

    /* 2. System Notifications & Shell */
    void       (*post_notification)(const char *title, const char *msg);
    void       (*set_live_pill)(const char *text, uint8_t priority);
    void       (*clear_live_pill)(void);

    /* 3. Audio & Haptics Engine */
    void       (*play_tone)(uint16_t freq_hz, uint16_t duration_ms);
    void       (*trigger_vibration)(uint16_t duration_ms);

    /* 4. Sandboxed Storage & Persistence */
    int        (*save_data)(const void *buf, size_t len);
    int        (*load_data)(void *buf, size_t max_len);

    /* 5. Memory Management (Tracked & Budgeted) */
    void *     (*alloc)(size_t bytes);
    void       (*free)(void *ptr);

    /* 6. Lifecycle & Termination */
    void       (*exit_app)(int exit_code);
} vapp_api_t;
```

---

## 4. Sandboxing & Fault Isolation

To ensure system stability, the OS enforces strict isolation on every executing VAPP:

### A. Memory Fencing & Quotas
- Every VAPP declares `req_heap_bytes` (e.g. 16 KB or 32 KB).
- The loader allocates a dedicated heap pool (`heap_4` sub-allocator or isolated memory arena).
- If the VAPP exceeds its declared heap ceiling, `api->alloc()` safely returns `NULL` rather than starving the OS or corrupting kernel memory.

### B. Hardware Watchdog & Exception Recovery
- On FreeRTOS/MIPS32: The CPU exception vector traps any NULL-pointer dereference or alignment fault originating from within the VAPP's code address boundaries.
- On Linux: Traps `SIGSEGV` and `SIGBUS` via `sigaction`.
- **Recovery Action**: Instead of rebooting the entire phone, the OS forcibly frees the VAPP's screen, deallocates its heap arena, and smoothly pops back to the **Fun Zone** menu with an alert: *"Application exited unexpectedly"*.

### C. Automatic Resource De-allocation
When a VAPP exits (either normally via RSK or due to a fault), the OS clean-up routine:
1. Deletes all LVGL widgets created by the VAPP.
2. Stops any background timers (`lv_timer_delete`).
3. Mutes any active audio tones or ongoing vibrations.
4. Frees the entire memory arena back to the system pool.

---

## 5. Sandboxed Persistent Storage

VAPPs cannot directly access arbitrary raw flash sectors or system NVRAM. Instead, each application is assigned an isolated data silo:

* **Storage Path**: `/sdcard/appdata/<app_id>.dat` or internal flash offset `0x8038_0000 + (app_id * 4KB)`.
* **Integrity Protection**: Every data block saved through `api->save_data()` automatically prepends a 16-bit CRC checksum. Corrupted reads are detected and rejected.

---

## 6. Implementation Roadmap

1. **Phase 1 (vapp_api_t Dispatch Table)**: Formalize `vapp_api_t` and refactor existing Fun Zone apps (Brick Breaker, Space Shooter, Tetris) to utilize standard API calls for audio and persistence.
2. **Phase 2 (Sandboxed Memory Arena)**: Implement tracking allocator in `vapp_loader.c` to enforce strict per-app heap budgets.
3. **Phase 3 (Exception Trapping)**: Add CPU exception wrappers around VAPP execution frames to enable crash-safe fallback to the shell.

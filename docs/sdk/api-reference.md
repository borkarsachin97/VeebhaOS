# VeebhaOS Core SDK API Reference

This document provides a concise reference for developers building applications and games on VeebhaOS.

---

## 1. Window Manager (`veebha_win_mgr.h`)

### `win_mgr_push`
Pushes a new screen onto the navigation stack.
```c
bool win_mgr_push(lv_obj_t *screen,
                  const char *lsk, softkey_callback_t lsk_cb,
                  const char *rsk, softkey_callback_t rsk_cb);
```
* **Parameters**:
  * `screen`: Root LVGL object of the screen to push.
  * `lsk`: Label for Left Softkey (e.g. `"Select"`, `"Restart"`).
  * `lsk_cb`: Callback triggered when Left Softkey is pressed.
  * `rsk`: Label for Right Softkey (e.g. `"Back"`, `"Exit"`).
  * `rsk_cb`: Callback triggered when Right Softkey is pressed.
* **Returns**: `true` on success, `false` on failure.

### `win_mgr_pop`
Pops the topmost screen from the stack and restores the underlying screen.
```c
bool win_mgr_pop(void);
```

### `win_mgr_get_group`
Returns the global keypad navigation group.
```c
lv_group_t * win_mgr_get_group(void);
```

### `win_mgr_set_fullscreen_mode`
Applies a fullscreen viewport mode to a screen.
```c
void win_mgr_set_fullscreen_mode(lv_obj_t *screen, os_fullscreen_mode_t mode, bool show_battery_hud);
```
* **Modes**:
  * `OS_FULLSCREEN_NONE`: Standard 18px top bar + 184px viewport + 20px softkeys.
  * `OS_FULLSCREEN_PARTIAL`: Hide top bar (200px viewport + 20px softkeys).
  * `OS_FULLSCREEN_FULL`: Hide both top bar and softkeys (Full 176×220 viewport).

---

## 2. Softkeys Subsystem (`veebha_softkeys.h`)

### `softkey_set_actions`
Updates softkey labels and callbacks for the active screen.
```c
void softkey_set_actions(const char *lsk_label, softkey_callback_t lsk_cb,
                         const char *rsk_label, softkey_callback_t rsk_cb);
```

### `softkey_bar_create`
Instantiates a fixed 20px softkey bar widget at the bottom of a container.
```c
lv_obj_t * softkey_bar_create(lv_obj_t *parent, const char *lsk_label, const char *rsk_label);
```

---

## 3. Declarative Templates (`veebha_templates.h`)

### `tpl_list_create`
Creates a standard scrollable menu screen.
```c
lv_obj_t * tpl_list_create(const tpl_list_view_t *desc);
```

### `tpl_dialog_show`
Displays a modal alert or confirmation dialog.
```c
void tpl_dialog_show(const tpl_dialog_desc_t *desc);
```

### `tpl_grid_create`
Creates an app launcher grid with D-pad navigation.
```c
lv_obj_t * tpl_grid_create(const tpl_grid_view_t *desc);
```

---

## 4. Audio Subsystem (`hal_audio.h`)

### `hal_audio_play_tone`
Generates a square-wave musical tone on the hardware audio DAC / speaker.
```c
void hal_audio_play_tone(uint16_t freq_hz, uint16_t duration_ms);
```
* **Parameters**:
  * `freq_hz`: Tone frequency in Hertz (e.g. `440` for A4).
  * `duration_ms`: Playback duration in milliseconds.

### `hal_audio_stop_tone`
Immediately stops tone playback.
```c
void hal_audio_stop_tone(void);
```

---

## 5. Storage & NVRAM (`os_nvram.h`)

### `os_nvram_read`
Reads persistent settings from flash storage.
```c
bool os_nvram_read(void *buffer, size_t size);
```

### `os_nvram_write`
Writes persistent settings with automatic CCITT CRC16 verification.
```c
bool os_nvram_write(const void *buffer, size_t size);
```

---

## 6. Virtual File System (`os_vfs.h`)

### `vfs_format_size`
Formats a byte count into a human-readable string using pure integer math (e.g. `"14.2 KB"`, `"4.8 MB"`).
```c
void vfs_format_size(uint32_t bytes, char *out_buf, size_t buf_size);
```

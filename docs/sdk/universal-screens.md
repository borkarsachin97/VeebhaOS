# Universal Multi-Screen SDK & Layout Guidelines

This document outlines the architectural strategy for transitioning VeebhaOS and its SDK from a fixed 176×220 portrait resolution into a **Universal Feature Phone SDK** supporting diverse display panels (128×160, 176×220, 240×320 QVGA, and 320×480 HVGA).

---

## 1. Supported Screen Profiles

Feature phones span several standardized screen sizes. The Universal SDK categorizes them into standard profiles:

| Profile | Dimensions | Aspect | Typical Device Class | Columns in Grid | Row Height |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Micro (QQVGA)** | 128 × 160 | 4:5 | Ultra-budget 1.44"–1.77" devices | 2 cols | 18 px |
| **Standard (QCIF+)** | 176 × 220 | 4:5 | Classic 2.0"–2.2" phones (OBTEL B10) | 3 cols | 22 px |
| **QVGA (Standard)** | 240 × 320 | 3:4 | Premium 2.4"–2.8" 4G VoLTE feature phones | 3 or 4 cols | 28 px |
| **HVGA (Large)** | 320 × 480 | 2:3 | Touch-and-type or large 3.2" feature devices | 4 cols | 36 px |

---

## 2. Three-Zone Screen Geometry

VeebhaOS screens are partitioned into three vertical layout zones. To ensure universal compatibility, **hardcoded pixel heights are replaced by dynamic layout tokens**:

```text
+-------------------------------------------------------+
| Zone A: Status Bar   (height: CONFIG_STATUS_BAR_H)    |  <-- 14px (Micro), 18px (Standard), 24px (QVGA)
+-------------------------------------------------------+
|                                                       |
| Zone B: Viewport      (lv_pct(100) x flex-grow: 1)   |  <-- Elastic content container
|                                                       |
+-------------------------------------------------------+
| Zone C: Softkey Bar  (height: CONFIG_SOFTKEY_BAR_H)   |  <-- 14px (Micro), 18px (Standard), 24px (QVGA)
+-------------------------------------------------------+
```

### Layout Tokens (`boards/board_config.h`)
```c
#define CONFIG_DISP_HOR_RES          240
#define CONFIG_DISP_VER_RES          320
#define CONFIG_STATUS_BAR_HEIGHT      24
#define CONFIG_SOFTKEY_BAR_HEIGHT     24
#define CONFIG_LIST_ROW_HEIGHT        28
#define CONFIG_GRID_COLS               3
```

---

## 3. Best Practices for Responsive Screen Development

### Rule 1: Never Hardcode Resolution Constants
❌ **Bad (Broken on QVGA or Micro screens)**:
```c
lv_obj_set_size(my_box, 176, 220);
lv_obj_set_pos(my_label, 10, 180);
```

✔ **Good (Adapts dynamically to any screen)**:
```c
lv_obj_set_size(my_box, lv_pct(100), lv_pct(100));
lv_obj_align(my_label, LV_ALIGN_BOTTOM_MID, 0, -4);
```

---

### Rule 2: Use Elastic Viewports for App Content
All screens created via `win_mgr_push()` or custom builders must use `lv_pct(100)` width and `flex-grow: 1`:

```c
lv_obj_t *content = lv_obj_create(screen);
lv_obj_set_size(content, lv_pct(100), 0);
lv_obj_set_flex_grow(content, 1);
```

---

### Rule 3: Responsive Declarative Templates

1. **`tpl_list`**: Row heights adapt automatically:
   - Micro (128x160): `height = 18 px`, font Montserrat 10.
   - Standard (176x220): `height = 22 px`, default font.
   - QVGA (240x320): `height = 28 px`, font Montserrat 14.

2. **`tpl_grid` (App Launcher)**:
   - Grid columns dynamically adjust based on horizontal resolution:
   ```c
   uint8_t cols = (CONFIG_DISP_HOR_RES >= 300) ? 4 : ((CONFIG_DISP_HOR_RES >= 170) ? 3 : 2);
   ```

3. **`tpl_dialog` (Modal Overlays)**:
   - Card width uses relative percentage padding rather than fixed 150px:
   ```c
   lv_obj_set_width(card, CONFIG_DISP_HOR_RES - 20);
   ```

---

## 4. Typography & Font Scaling

VeebhaOS provides resolution-scaled font aliases through `veebha_font_get_default()`:

| Font Role | Micro (128×160) | Standard (176×220) | QVGA (240×320) |
| :--- | :--- | :--- | :--- |
| **Small / Captions** | 8 px | 10 px | 12 px |
| **Regular / List Items** | 10 px | 12 px | 14 px |
| **Large / Titles** | 12 px | 14 px | 16 px |
| **Clock Numbers** | 14 px | 18 px | 24 px |

---

## 5. Migration Roadmap for Full Universal SDK

1. **Phase 1 (Layout Tokens)**: Replace all remaining `176` and `220` numeric literals in `apps/` with `CONFIG_DISP_HOR_RES` and `CONFIG_DISP_VER_RES`.
2. **Phase 2 (Dynamic Grid & Template Scaling)**: Update `tpl_grid.c` and `tpl_list.c` to compute dimensions from active display properties.
3. **Phase 3 (Simulator Profile Switcher)**: Allow running the desktop simulator with runtime flags (e.g. `./veebha_sim --res 240x320` or `./veebha_sim --res 128x160`).

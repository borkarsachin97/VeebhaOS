# Window Manager & Navigation Model

This document details the VeebhaOS Window Manager ([`sdk/core/win_mgr.c`](file:///home/vixxkigoli/pm/VeebhaOS/sdk/core/win_mgr.c)), navigation stack, softkey bindings, multitasking task switcher, and fullscreen viewport modes.

---

## 1. Navigation Stack Architecture

VeebhaOS uses a hierarchical navigation stack model optimized for physical keypad navigation.

* **Root Screen (Depth 1)**: The Standby / Home screen is the permanent root of the navigation stack. It is never popped.
* **Push Operation (`win_mgr_push`)**:
  Pushes a new screen onto the stack. The previous screen is preserved and hidden:
  ```c
  win_mgr_push(new_screen, "Select", on_select_cb, "Back", on_back_cb);
  ```
* **Pop Operation (`win_mgr_pop`)**:
  Pops the topmost screen from the stack, automatically deletes its LVGL objects (unless flagged with `keep_alive`), restores focus to the previously focused item, and re-activates the underlying screen.

---

## 2. Softkey Routing Model

Feature phones feature two dedicated physical softkeys positioned directly underneath the LCD:
* **Left Softkey (LSK)**: Typically mapped to primary contextual actions (e.g. *Select*, *Options*, *Call*, *Restart*, *Save*).
* **Right Softkey (RSK)**: Typically mapped to secondary or cancellation actions (e.g. *Back*, *Exit*, *Clear*, *Dismiss*).

When screens are pushed or popped, the window manager automatically updates the labels on the persistent bottom softkey bar and routes key events to the registered callback functions:
```c
softkey_set_actions("Options", on_options_click, "Back", on_back_click);
```

---

## 3. Fullscreen Modes

VeebhaOS supports three fullscreen viewport modes ([`sdk/include/veebha_win_mgr.h`](file:///home/vixxkigoli/pm/VeebhaOS/sdk/include/veebha_win_mgr.h)):

```text
+-----------------------+   +-----------------------+   +-----------------------+
|  18px Top Status Bar  |   |                       |   |                       |
+-----------------------+   |                       |   |                       |
|                       |   |                       |   |                       |
|                       |   |     200px Viewport    |   |     Full 176x220      |
|     184px Viewport    |   |                       |   |    Game Playfield     |
|                       |   |                       |   |                       |
|                       |   |                       |   |                       |
+-----------------------+   +-----------------------+   |                       |
|  20px Softkey Bar     |   |  20px Softkey Bar     |   |                       |
+-----------------------+   +-----------------------+   +-----------------------+
  OS_FULLSCREEN_NONE          OS_FULLSCREEN_PARTIAL        OS_FULLSCREEN_FULL
```

| Mode | Top Status Bar (18px) | Content Viewport | Bottom Softkeys (20px) | Typical Usage |
| :--- | :--- | :--- | :--- | :--- |
| `OS_FULLSCREEN_NONE` | **Visible** | 184 px | **Visible** | Standard menus, Settings, Contacts, Messages |
| `OS_FULLSCREEN_PARTIAL` | **Hidden** | 200 px | **Visible** | Web browser, Camera viewfinder, Gallery |
| `OS_FULLSCREEN_FULL` | **Hidden** | **220 px (Full)** | **Hidden** | Games (Space Shooter, Tetris, Brick Breaker) |

### Setting Fullscreen Mode
In the screen header (`win_mgr_screen_hdr_t`):
```c
win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)calloc(1, sizeof(win_mgr_screen_hdr_t));
if (hdr) {
    hdr->fullscreen_mode = OS_FULLSCREEN_FULL;
    hdr->show_battery_hud = false;
    lv_obj_set_user_data(screen, hdr);
}
```

---

## 4. Multitasking & Task Switcher

VeebhaOS supports true multitasking across multiple independent application contexts.

* **Task Pool**: Up to 8 concurrent backgrounded tasks can reside in memory.
* **Invoking Task Switcher**:
  Holding the **`*` (Star)** key for 600ms triggers the visual Task Switcher overlay.
* **Task Switching**:
  The user cycles through active apps using D-Pad Left/Right and presses **Center OK** to bring the selected application to the foreground. The window manager restores the application's exact screen stack and focused widget.

---

## 5. Notification & Quick Settings Drawer

Holding the **Green Call** key for 600ms opens the persistent Notification Drawer overlay:
* **Tab 1: Notifications Deck**: Displays unread SMS messages, missed calls, and system status alerts.
* **Tab 2: Quick Settings**: Fast toggles for Silent profile, Bluetooth, Flashlight/Torch, and Display Brightness.

/*
 * VeebhaOS - QEMU RDA8809 Keypad Driver & Input Abstraction
 * Maps hardware matrix scanner registers to VeebhaOS input events and LVGL indev
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "hal_keypad_qemu.h"
#include "sdk/hal/hal_keypad.h"
#include "drivers/hal_input.h"
#include "boards/qemu_xcpu/include/keypad.h"
#include "boards/qemu_xcpu/include/timer.h"
#include "boards/board_config.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_templates.h"
#include "sdk/include/veebha_overlays.h"
#include "sdk/include/veebha_t9.h"
#include "sdk/include/veebha_event.h"
#include "sdk/include/veebha_live_pill.h"
#include "apps/home/app_launcher.h"
#include "apps/home/app_idle.h"
#include "apps/dialer/app_dialer.h"
#include "apps/music/app_music.h"
#include "apps/tools/app_calc.h"
#include "apps/tools/app_stopwatch.h"
#include "apps/overlays/screen_saver.h"
#include "apps/overlays/usb_select.h"
#include "apps/settings/app_settings.h"
#include "apps/telephony/app_incall.h"
#include "sdk/core/os_kernel.h"
#include "sdk/include/veebha_hardware.h"
#include "third_party/lvgl/lvgl.h"
#include "third_party/lvgl/src/indev/lv_indev_private.h"
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

TaskHandle_t g_keypad_task_handle = NULL;
static lv_indev_t *s_lv_indev = NULL;
static veebha_key_callback_t s_user_cb = NULL;
static INT32 s_prev_hw_key = KEY_CODE_NONE;
static uint32_t s_current_held_lv_key = 0;
static veebha_key_t s_held_hw_key = VEEBHA_KEY_NONE;

/* Key state tracking for long-press calculation */
static struct {
    veebha_key_t key;
    uint32_t press_start_ms;
    bool is_pressed;
    bool long_press_fired;
} s_key_state[VEEBHA_KEY_MAX];

/* FIFO event queue so rapid presses and releases are not dropped */
typedef struct {
    uint32_t key;
    lv_indev_state_t state;
} indev_event_t;

#define INDEV_QUEUE_SIZE 64
static indev_event_t s_indev_queue[INDEV_QUEUE_SIZE];
static uint16_t s_q_head = 0;
static uint16_t s_q_tail = 0;

static void indev_queue_push(uint32_t key, lv_indev_state_t state)
{
    uint16_t next = (s_q_head + 1) % INDEV_QUEUE_SIZE;
    if (next != s_q_tail) {
        s_indev_queue[s_q_head].key = key;
        s_indev_queue[s_q_head].state = state;
        s_q_head = next;
    }
}

static bool indev_queue_pop(indev_event_t *ev)
{
    if (s_q_head == s_q_tail) return false;
    *ev = s_indev_queue[s_q_tail];
    s_q_tail = (s_q_tail + 1) % INDEV_QUEUE_SIZE;
    return true;
}

static bool indev_queue_has_items(void)
{
    return s_q_head != s_q_tail;
}

static veebha_key_t hw_code_to_veebha_key(INT32 hw_code)
{
    switch (hw_code) {
    case KEY_CODE_LS:     return VEEBHA_KEY_LSK;
    case KEY_CODE_CALL:   return VEEBHA_KEY_CALL;
    case KEY_CODE_RS:     return VEEBHA_KEY_RSK;
    case KEY_CODE_RIGHT:  return VEEBHA_KEY_RIGHT;
    case KEY_CODE_STAR:   return VEEBHA_KEY_STAR;
    case KEY_CODE_0:      return VEEBHA_KEY_NUM_0;
    case KEY_CODE_HASH:   return VEEBHA_KEY_HASH;
    case KEY_CODE_LEFT:   return VEEBHA_KEY_LEFT;
    case KEY_CODE_7:      return VEEBHA_KEY_NUM_7;
    case KEY_CODE_8:      return VEEBHA_KEY_NUM_8;
    case KEY_CODE_9:      return VEEBHA_KEY_NUM_9;
    case KEY_CODE_DOWN:   return VEEBHA_KEY_DOWN;
    case KEY_CODE_4:      return VEEBHA_KEY_NUM_4;
    case KEY_CODE_5:      return VEEBHA_KEY_NUM_5;
    case KEY_CODE_6:      return VEEBHA_KEY_NUM_6;
    case KEY_CODE_OK:     return VEEBHA_KEY_OK;
    case KEY_CODE_1:      return VEEBHA_KEY_NUM_1;
    case KEY_CODE_2:      return VEEBHA_KEY_NUM_2;
    case KEY_CODE_3:      return VEEBHA_KEY_NUM_3;
    case KEY_CODE_UP:     return VEEBHA_KEY_UP;
    case KEY_CODE_POWER:  return VEEBHA_KEY_END;
    default:              return VEEBHA_KEY_NONE;
    }
}

static uint32_t veebha_key_to_lv_key(veebha_key_t key)
{
    /* Prioritize active overlays on lv_layer_top */
    if (task_mgr_is_active()) {
        switch (key) {
        case VEEBHA_KEY_UP:
        case VEEBHA_KEY_LEFT:  return LV_KEY_PREV;
        case VEEBHA_KEY_DOWN:
        case VEEBHA_KEY_RIGHT: return LV_KEY_NEXT;
        case VEEBHA_KEY_OK:    return LV_KEY_ENTER;
        default: return 0;
        }
    }

    if (usb_select_is_active()) {
        switch (key) {
        case VEEBHA_KEY_UP:
        case VEEBHA_KEY_LEFT:  return LV_KEY_PREV;
        case VEEBHA_KEY_DOWN:
        case VEEBHA_KEY_RIGHT: return LV_KEY_NEXT;
        case VEEBHA_KEY_OK:    return LV_KEY_ENTER;
        default: return 0;
        }
    }

    if (notif_panel_is_active() || tpl_dialog_is_active()) {
        return 0;
    }

    if (app_stopwatch_is_active()) {
        switch (key) {
        case VEEBHA_KEY_UP:
        case VEEBHA_KEY_DOWN:
        case VEEBHA_KEY_LEFT:
        case VEEBHA_KEY_RIGHT:
        case VEEBHA_KEY_OK:
        case VEEBHA_KEY_HASH:
        case VEEBHA_KEY_STAR:
            return 0;
        default:
            if (key >= VEEBHA_KEY_NUM_0 && key <= VEEBHA_KEY_NUM_9) return 0;
            break;
        }
    }

    if (app_incall_is_foreground()) {
        switch (key) {
        case VEEBHA_KEY_UP:
        case VEEBHA_KEY_DOWN:
        case VEEBHA_KEY_LEFT:
        case VEEBHA_KEY_RIGHT:
        case VEEBHA_KEY_OK:
        case VEEBHA_KEY_HASH:
        case VEEBHA_KEY_STAR:
            return 0;
        default:
            if (key >= VEEBHA_KEY_NUM_0 && key <= VEEBHA_KEY_NUM_9) return 0;
            break;
        }
    }

    veebha_view_type_t vt = win_mgr_get_active_view_type();
    lv_group_t *cur_grp = win_mgr_get_group();
    bool is_editing = cur_grp ? lv_group_get_editing(cur_grp) : false;

    switch (key) {
    case VEEBHA_KEY_UP:
        if (vt == VEEBHA_VIEW_TYPE_DIALER || vt == VEEBHA_VIEW_TYPE_MEDIA || vt == VEEBHA_VIEW_TYPE_CALC) return 0;
        if (is_editing) return LV_KEY_UP;
        return (vt == VEEBHA_VIEW_TYPE_GRID || vt == VEEBHA_VIEW_TYPE_CALENDAR) ? LV_KEY_UP : LV_KEY_PREV;
    case VEEBHA_KEY_DOWN:
        if (vt == VEEBHA_VIEW_TYPE_DIALER || vt == VEEBHA_VIEW_TYPE_MEDIA || vt == VEEBHA_VIEW_TYPE_CALC) return 0;
        if (is_editing) return LV_KEY_DOWN;
        return (vt == VEEBHA_VIEW_TYPE_GRID || vt == VEEBHA_VIEW_TYPE_CALENDAR) ? LV_KEY_DOWN : LV_KEY_NEXT;
    case VEEBHA_KEY_LEFT:
        if (vt == VEEBHA_VIEW_TYPE_MEDIA || vt == VEEBHA_VIEW_TYPE_DIALER || vt == VEEBHA_VIEW_TYPE_CALC) return 0;
        if (is_editing) return LV_KEY_LEFT;
        return (vt == VEEBHA_VIEW_TYPE_LIST) ? LV_KEY_PREV : LV_KEY_LEFT;
    case VEEBHA_KEY_RIGHT:
        if (vt == VEEBHA_VIEW_TYPE_MEDIA || vt == VEEBHA_VIEW_TYPE_DIALER || vt == VEEBHA_VIEW_TYPE_CALC) return 0;
        if (is_editing) return LV_KEY_RIGHT;
        return (vt == VEEBHA_VIEW_TYPE_LIST) ? LV_KEY_NEXT : LV_KEY_RIGHT;
    case VEEBHA_KEY_OK:
        if (vt == VEEBHA_VIEW_TYPE_MEDIA || vt == VEEBHA_VIEW_TYPE_DIALER) return 0;
        return LV_KEY_ENTER;
    case VEEBHA_KEY_RSK:
    case VEEBHA_KEY_LSK:
    case VEEBHA_KEY_CALL:
    case VEEBHA_KEY_END:
        return 0;
    case VEEBHA_KEY_NUM_0:
        return '0';
    case VEEBHA_KEY_NUM_1:
        return '1';
    case VEEBHA_KEY_NUM_2:
        return '2';
    case VEEBHA_KEY_NUM_3:
        return '3';
    case VEEBHA_KEY_NUM_4:
        return '4';
    case VEEBHA_KEY_NUM_5:
        return '5';
    case VEEBHA_KEY_NUM_6:
        return '6';
    case VEEBHA_KEY_NUM_7:
        return '7';
    case VEEBHA_KEY_NUM_8:
        return '8';
    case VEEBHA_KEY_NUM_9:
        return '9';
    case VEEBHA_KEY_STAR:
        return '*';
    case VEEBHA_KEY_HASH:
        return '#';
    default:
        return 0;
    }
}

static void dispatch_key_event(veebha_key_t key, veebha_key_state_t state, veebha_press_type_t press_type, uint32_t ts)
{
    veebha_key_event_t ev = {
        .key = key,
        .state = state,
        .press_type = press_type,
        .timestamp_ms = ts
    };

    if (s_user_cb) {
        s_user_cb(&ev);
    }
}

void hal_input_push_event(veebha_key_t key, veebha_key_state_t state)
{
    if (key <= VEEBHA_KEY_NONE || key >= VEEBHA_KEY_MAX) return;

    os_lvgl_lock();
    uint32_t now = timer_get_ms();
    uint32_t lv_key = veebha_key_to_lv_key(key);
    veebha_view_type_t vt = win_mgr_get_active_view_type();
    os_log_printf("[INPUT_PUSH] key=%d state=%d vt=%d lv_key=%u\n", (int)key, (int)state, (int)vt, lv_key);

    if (state == VEEBHA_KEY_STATE_PRESSED) {
        if (screen_saver_is_active()) {
            screen_saver_hide();
            os_lvgl_unlock();
            return;
        }
        screen_saver_reset_idle();

        if (!s_key_state[key].is_pressed) {
            s_key_state[key].is_pressed = true;
            s_key_state[key].press_start_ms = now;
            s_key_state[key].long_press_fired = false;
            s_held_hw_key = key;
            s_current_held_lv_key = lv_key;
            if (notif_panel_is_active()) {
                if (key == VEEBHA_KEY_CALL || key == VEEBHA_KEY_STAR || key == VEEBHA_KEY_HASH ||
                    key == VEEBHA_KEY_UP || key == VEEBHA_KEY_DOWN ||
                    key == VEEBHA_KEY_LEFT || key == VEEBHA_KEY_RIGHT ||
                    key == VEEBHA_KEY_OK || key == VEEBHA_KEY_NUM_0) {
                    notif_panel_handle_key(key);
                } else if (key == VEEBHA_KEY_LSK) {
                    softkey_trigger_lsk();
                } else if (key == VEEBHA_KEY_RSK) {
                    softkey_trigger_rsk();
                }
            } else if (task_mgr_is_active()) {
                if (key == VEEBHA_KEY_UP || key == VEEBHA_KEY_LEFT ||
                    key == VEEBHA_KEY_DOWN || key == VEEBHA_KEY_RIGHT) {
                    if (lv_key != 0) {
                        indev_queue_push(lv_key, LV_INDEV_STATE_PRESSED);
                    }
                } else if (key == VEEBHA_KEY_OK || key == VEEBHA_KEY_LSK) {
                    softkey_trigger_lsk();
                } else if (key == VEEBHA_KEY_RSK) {
                    softkey_trigger_rsk();
                } else if (key == VEEBHA_KEY_END || key == VEEBHA_KEY_STAR) {
                    task_mgr_close();
                }
            } else if (usb_select_is_active()) {
                if (key == VEEBHA_KEY_UP || key == VEEBHA_KEY_LEFT ||
                    key == VEEBHA_KEY_DOWN || key == VEEBHA_KEY_RIGHT) {
                    if (lv_key != 0) {
                        indev_queue_push(lv_key, LV_INDEV_STATE_PRESSED);
                    }
                } else if (key == VEEBHA_KEY_OK || key == VEEBHA_KEY_LSK) {
                    softkey_trigger_lsk();
                } else if (key == VEEBHA_KEY_RSK || key == VEEBHA_KEY_END) {
                    softkey_trigger_rsk();
                }
            } else if (tpl_dialog_is_active()) {
                if (key == VEEBHA_KEY_OK || key == VEEBHA_KEY_LSK) {
                    softkey_trigger_lsk();
                } else if (key == VEEBHA_KEY_RSK || key == VEEBHA_KEY_END) {
                    softkey_trigger_rsk();
                }
            } else if (app_incall_is_foreground() &&
                ((key >= VEEBHA_KEY_NUM_0 && key <= VEEBHA_KEY_NUM_9) ||
                 key == VEEBHA_KEY_HASH || key == VEEBHA_KEY_STAR ||
                 key == VEEBHA_KEY_OK)) {
                app_incall_handle_key(key);
            } else if (vt == VEEBHA_VIEW_TYPE_EDITOR &&
                ((key >= VEEBHA_KEY_NUM_0 && key <= VEEBHA_KEY_NUM_9) ||
                 key == VEEBHA_KEY_HASH || key == VEEBHA_KEY_STAR)) {
                t9_engine_handle_key(lv_key);
            } else if (vt == VEEBHA_VIEW_TYPE_DIALER &&
                       ((key >= VEEBHA_KEY_NUM_0 && key <= VEEBHA_KEY_NUM_9) ||
                        key == VEEBHA_KEY_HASH || key == VEEBHA_KEY_STAR)) {
                char d = (key >= VEEBHA_KEY_NUM_0 && key <= VEEBHA_KEY_NUM_9) ? ('0' + (key - VEEBHA_KEY_NUM_0)) :
                         (key == VEEBHA_KEY_STAR ? '*' : '#');
                app_dialer_handle_digit(d);
            } else if (vt == VEEBHA_VIEW_TYPE_CALC &&
                       ((key >= VEEBHA_KEY_NUM_0 && key <= VEEBHA_KEY_NUM_9) ||
                        key == VEEBHA_KEY_HASH || key == VEEBHA_KEY_STAR ||
                        key == VEEBHA_KEY_UP || key == VEEBHA_KEY_DOWN ||
                        key == VEEBHA_KEY_LEFT || key == VEEBHA_KEY_RIGHT ||
                        key == VEEBHA_KEY_OK)) {
                app_calc_handle_key(key);
            } else if (app_stopwatch_is_active() &&
                       ((key >= VEEBHA_KEY_NUM_0 && key <= VEEBHA_KEY_NUM_9) ||
                        key == VEEBHA_KEY_HASH || key == VEEBHA_KEY_STAR ||
                        key == VEEBHA_KEY_UP || key == VEEBHA_KEY_DOWN ||
                        key == VEEBHA_KEY_LEFT || key == VEEBHA_KEY_RIGHT ||
                        key == VEEBHA_KEY_OK)) {
                app_stopwatch_handle_key(key);
            } else if (vt == VEEBHA_VIEW_TYPE_IDLE &&
                       live_pill_is_active() && key == VEEBHA_KEY_OK) {
                live_pill_trigger_click();
            } else if (vt == VEEBHA_VIEW_TYPE_IDLE &&
                       app_incall_is_active() && (key == VEEBHA_KEY_OK || key == VEEBHA_KEY_CALL)) {
                app_incall_show();
            } else if (vt == VEEBHA_VIEW_TYPE_IDLE && key == VEEBHA_KEY_OK) {
                app_launcher_open();
            } else if ((vt == VEEBHA_VIEW_TYPE_GRID || vt == VEEBHA_VIEW_TYPE_IDLE) &&
                       (key >= VEEBHA_KEY_NUM_0 && key <= VEEBHA_KEY_NUM_9)) {
                char str[2] = { (char)('0' + (key - VEEBHA_KEY_NUM_0)), '\0' };
                app_dialer_open(str);
            } else if (vt == VEEBHA_VIEW_TYPE_IDLE &&
                       key == VEEBHA_KEY_HASH) {
                app_settings_toggle_silent();
                bool is_sil = app_settings_is_silent();
                static char s_prof_msg[64];
                snprintf(s_prof_msg, sizeof(s_prof_msg), "%s Mode Activated", is_sil ? "Silent" : "General");
                notif_panel_post_alert("Sound Profile", s_prof_msg);
            } else if (lv_key != 0) {
                indev_queue_push(lv_key, LV_INDEV_STATE_PRESSED);
            }

            dispatch_key_event(key, VEEBHA_KEY_STATE_PRESSED, VEEBHA_PRESS_SHORT, now);

            if (!notif_panel_is_active() && !task_mgr_is_active() && !usb_select_is_active() && !tpl_dialog_is_active()) {
                if (vt == VEEBHA_VIEW_TYPE_MEDIA && !app_incall_is_foreground()) {
                    if (key == VEEBHA_KEY_HASH) {
                        app_music_toggle_mode();
                    } else if (app_music_get_mode() == MUSIC_MODE_FM_RADIO) {
                        if (key == VEEBHA_KEY_LEFT) {
                            app_music_fm_seek(-1);
                        } else if (key == VEEBHA_KEY_RIGHT) {
                            app_music_fm_seek(+1);
                        } else if (key == VEEBHA_KEY_UP) {
                            app_music_fm_next_preset();
                        } else if (key == VEEBHA_KEY_DOWN) {
                            app_music_fm_prev_preset();
                        } else if (key == VEEBHA_KEY_OK) {
                            app_music_fm_toggle_mute();
                        }
                    } else {
                        if (key == VEEBHA_KEY_LEFT) {
                            tpl_media_handle_seek(-5);
                        } else if (key == VEEBHA_KEY_RIGHT) {
                            tpl_media_handle_seek(+5);
                        } else if (key == VEEBHA_KEY_OK) {
                            tpl_media_handle_toggle();
                        } else if (key == VEEBHA_KEY_UP) {
                            app_music_adjust_volume(+1);
                        } else if (key == VEEBHA_KEY_DOWN) {
                            app_music_adjust_volume(-1);
                        }
                    }
                } else if (vt == VEEBHA_VIEW_TYPE_DIALER) {
                    if (key == VEEBHA_KEY_OK) {
                        app_dialer_start_call();
                    }
                }

                if (key == VEEBHA_KEY_LSK) {
                    softkey_trigger_lsk();
                } else if (key == VEEBHA_KEY_RSK) {
                    if (softkey_get_rsk_long_action() == NULL) {
                        softkey_trigger_rsk();
                    }
                } else if (key == VEEBHA_KEY_END) {
                    win_mgr_show_home();
                } else if (key == VEEBHA_KEY_CALL) {
                    if (app_incall_is_active() && !app_incall_is_foreground()) {
                        app_incall_show();
                    } else if (vt == VEEBHA_VIEW_TYPE_DIALER) {
                        app_dialer_start_call();
                    } else if (vt == VEEBHA_VIEW_TYPE_GRID || vt == VEEBHA_VIEW_TYPE_IDLE) {
                        app_dialer_open(NULL);
                    }
                }
            }
        }
    } else {
        if (s_key_state[key].is_pressed) {
            uint32_t duration = now - s_key_state[key].press_start_ms;
            veebha_press_type_t ptype = (duration >= VEEBHA_LONG_PRESS_MS || s_key_state[key].long_press_fired)
                                        ? VEEBHA_PRESS_LONG
                                        : VEEBHA_PRESS_SHORT;

            s_key_state[key].is_pressed = false;
            if (s_held_hw_key == key) {
                s_held_hw_key = VEEBHA_KEY_NONE;
                s_current_held_lv_key = 0;
            }

            veebha_view_type_t vt = win_mgr_get_active_view_type();
            if ((vt == VEEBHA_VIEW_TYPE_EDITOR || vt == VEEBHA_VIEW_TYPE_CALC || app_stopwatch_is_active() ||
                 app_incall_is_foreground() || notif_panel_is_active() || task_mgr_is_active() ||
                 usb_select_is_active() || tpl_dialog_is_active() || (vt == VEEBHA_VIEW_TYPE_IDLE && key == VEEBHA_KEY_OK)) &&
                ((key >= VEEBHA_KEY_NUM_0 && key <= VEEBHA_KEY_NUM_9) ||
                 key == VEEBHA_KEY_HASH || key == VEEBHA_KEY_STAR ||
                 key == VEEBHA_KEY_UP || key == VEEBHA_KEY_DOWN ||
                 key == VEEBHA_KEY_LEFT || key == VEEBHA_KEY_RIGHT ||
                 key == VEEBHA_KEY_OK || key == VEEBHA_KEY_LSK || key == VEEBHA_KEY_RSK || key == VEEBHA_KEY_END || key == VEEBHA_KEY_CALL)) {
                /* Handled on press, do not push release to indev */
            } else if (lv_key != 0) {
                indev_queue_push(lv_key, LV_INDEV_STATE_RELEASED);
            }

            dispatch_key_event(key, VEEBHA_KEY_STATE_RELEASED, ptype, now);

            if (key == VEEBHA_KEY_RSK) {
                if (softkey_get_rsk_long_action() != NULL) {
                    if (!s_key_state[key].long_press_fired && duration < VEEBHA_LONG_PRESS_MS) {
                        softkey_trigger_rsk();
                    }
                }
            }
        }
    }
    os_lvgl_unlock();
}

static void keypad_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    indev_event_t ev;
    if (indev_queue_pop(&ev)) {
        data->key = ev.key;
        data->state = ev.state;
        data->continue_reading = indev_queue_has_items();
    } else {
        if (s_current_held_lv_key != 0) {
            data->key = s_current_held_lv_key;
            data->state = LV_INDEV_STATE_PRESSED;
        } else {
            data->state = LV_INDEV_STATE_RELEASED;
        }
        data->continue_reading = false;
    }
}

void hal_keypad_task(void *pvParameters)
{
    (void)pvParameters;
    INT32 prev_hw_key = KEY_CODE_NONE;

    while (1) {
        uint32_t now = timer_get_ms();

        /* 1. Long press expiration check */
        for (int i = 1; i < VEEBHA_KEY_MAX; i++) {
            if (s_key_state[i].is_pressed && !s_key_state[i].long_press_fired) {
                if ((now - s_key_state[i].press_start_ms) >= VEEBHA_LONG_PRESS_MS) {
                    s_key_state[i].long_press_fired = true;
                    os_lvgl_lock();
                    dispatch_key_event((veebha_key_t)i, VEEBHA_KEY_STATE_PRESSED, VEEBHA_PRESS_LONG, now);
                    if (i == VEEBHA_KEY_RSK) {
                        softkey_trigger_rsk_long();
                    } else if (i == VEEBHA_KEY_CALL) {
                        notif_panel_toggle();
                    } else if (i == VEEBHA_KEY_STAR) {
                        task_mgr_toggle();
                    }
                    os_lvgl_unlock();
                }
            }
        }

        /* 2. Hardware Matrix Register Polling & IRQ latch reading */
        INT32 active = keypad_get_active_key();

        if (active != prev_hw_key) {
            if (active != KEY_CODE_NONE) {
                bool was_sleeping = veebha_hw_lps_is_sleeping();
                veebha_hw_lps_reset_activity();
                if (was_sleeping) {
                    os_log_printf("[KEYPAD_TASK] Screen woken from sleep by key %d\n", active);
                    prev_hw_key = active;
                    vTaskDelay(pdMS_TO_TICKS(50));
                    continue;
                }
            }

            if (prev_hw_key != KEY_CODE_NONE) {
                veebha_key_t old_vk = hw_code_to_veebha_key(prev_hw_key);
                if (old_vk != VEEBHA_KEY_NONE) {
                    os_event_t evt;
                    memset(&evt, 0, sizeof(evt));
                    evt.type = OS_EVT_KEY_UP;
                    evt.timestamp = now;
                    evt.payload.key.key = old_vk;
                    os_event_post(&evt);

                    hal_input_push_event(old_vk, VEEBHA_KEY_STATE_RELEASED);
                }
            }

            if (active != KEY_CODE_NONE) {
                veebha_key_t new_vk = hw_code_to_veebha_key(active);
                if (new_vk != VEEBHA_KEY_NONE) {
                    os_event_t evt;
                    memset(&evt, 0, sizeof(evt));
                    evt.type = OS_EVT_KEY_DOWN;
                    evt.timestamp = now;
                    evt.payload.key.key = new_vk;
                    os_event_post(&evt);

                    hal_input_push_event(new_vk, VEEBHA_KEY_STATE_PRESSED);
                }
            }

            os_log_printf("[KEYPAD_TASK] Key change: prev=%d -> active=%d\n", prev_hw_key, active);
            prev_hw_key = active;
        }

        bool any_pressed = false;
        for (int i = 1; i < VEEBHA_KEY_MAX; i++) {
            if (s_key_state[i].is_pressed) {
                any_pressed = true;
                break;
            }
        }

        if (!any_pressed && active == KEY_CODE_NONE) {
            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(50));
        } else {
            vTaskDelay(pdMS_TO_TICKS(15));
        }
    }
}

void hal_keypad_qemu_init(void)
{
    memset(s_key_state, 0, sizeof(s_key_state));
    s_prev_hw_key = KEY_CODE_NONE;
    s_q_head = 0;
    s_q_tail = 0;

    keypad_init();
    keypad_set_irq_mode(TRUE); /* Enable IRQ latch + Polling hybrid */

    s_lv_indev = lv_indev_create();
    lv_indev_set_type(s_lv_indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(s_lv_indev, keypad_read_cb);
    s_lv_indev->long_press_time = 200;
    s_lv_indev->long_press_repeat_time = 60;

    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_indev_set_group(s_lv_indev, g);
    }
}

lv_indev_t * hal_keypad_qemu_get_indev(void)
{
    return s_lv_indev;
}

lv_indev_t * hal_input_get_lv_indev(void)
{
    return s_lv_indev;
}

void hal_input_set_callback(veebha_key_callback_t cb)
{
    s_user_cb = cb;
}

bool hal_input_init(void)
{
    hal_keypad_qemu_init();
    return true;
}

void hal_input_deinit(void)
{
    s_lv_indev = NULL;
    s_user_cb = NULL;
}

bool hal_input_poll(void)
{
    return true;
}

void hal_keypad_init(void)
{
    hal_keypad_qemu_init();
}

void hal_keypad_deinit(void)
{
    hal_input_deinit();
}

bool hal_keypad_poll(uint32_t *out_key, bool *out_pressed)
{
    indev_event_t ev;
    if (indev_queue_pop(&ev)) {
        if (out_key) *out_key = ev.key;
        if (out_pressed) *out_pressed = (ev.state == LV_INDEV_STATE_PRESSED);
        return true;
    }
    return false;
}

void hal_keypad_poll_and_feed(void)
{
    /* Handled automatically inside keypad_read_cb by LVGL timer handler */
}

/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * OBTEL B10 Hardware Abstraction Layer: Keypad Driver (5x5 Matrix)
 *
 * SPDX-License-Identifier: MIT
 */

#include "sdk/hal/hal_keypad.h"
#include "boards/obtel_b10/include/global_macros.h"
#include "boards/obtel_b10/include/sys_irq.h"
#include "lvgl.h"
#include <stdbool.h>
#include <string.h>

#define RDA_KEYPAD_DATA         REG32(RDA_BASE_KEYPAD + 0x00)
#define RDA_KEYPAD_STATUS       REG32(RDA_BASE_KEYPAD + 0x04)
#define RDA_KEYPAD_CTRL         REG32(RDA_BASE_KEYPAD + 0x08)

static lv_indev_t *s_indev = NULL;
static uint32_t s_last_key = 0;
static bool s_is_pressed = false;

/* 5x5 Matrix to LVGL Key Map */
static const uint32_t s_key_matrix[5][5] = {
    { LV_KEY_UP,       LV_KEY_ENTER,  LV_KEY_DOWN,   LV_KEY_LEFT,   LV_KEY_RIGHT },
    { '1',             '2',           '3',           LV_KEY_PREV,   LV_KEY_NEXT  }, /* LSK, RSK */
    { '4',             '5',           '6',           'c',           'e'          }, /* CALL, END */
    { '7',             '8',           '9',           '*',           '#'          },
    { '*',             '0',           '#',           0,             0            }
};

static void keypad_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    uint32_t status = RDA_KEYPAD_STATUS;
    if (status & 0x01) {
        uint32_t raw = RDA_KEYPAD_DATA;
        uint8_t row = (raw >> 4) & 0x07;
        uint8_t col = raw & 0x07;
        if (row < 5 && col < 5) {
            s_last_key = s_key_matrix[row][col];
            s_is_pressed = (raw & 0x80) ? true : false;
        }
    }

    data->key = s_last_key;
    data->state = s_is_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void hal_keypad_init(void)
{
    RDA_KEYPAD_CTRL = 0x01; /* Enable HW keypad scanner with debouncing */

    s_indev = lv_indev_create();
    if (s_indev) {
        lv_indev_set_type(s_indev, LV_INDEV_TYPE_KEYPAD);
        lv_indev_set_read_cb(s_indev, keypad_read_cb);
    }
}

void hal_keypad_deinit(void)
{
    RDA_KEYPAD_CTRL = 0x00;
    s_indev = NULL;
}

bool hal_keypad_poll(uint32_t *out_key, bool *out_pressed)
{
    if (out_key) *out_key = s_last_key;
    if (out_pressed) *out_pressed = s_is_pressed;
    return s_is_pressed;
}

void hal_keypad_poll_and_feed(void)
{
    /* Driven by LVGL indev read callback */
}

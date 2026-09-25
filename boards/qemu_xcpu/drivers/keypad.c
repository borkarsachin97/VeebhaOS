/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Keypad Driver Implementation for RDA8809 / RDA8955 SoC
 */

#include "cs_types.h"
#include "global_macros.h"
#include "sys_ctrl.h"
#include "keypad.h"
#include "timer.h"
#include "os_vector_table.h"
#include "FreeRTOS.h"
#include "task.h"

void keypad_init(void)
{
    // Clear keypad module reset in sysCtrl
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_UNLOCK;
    hwp_sysCtrl->Sys_Rst_Clr = SYS_CTRL_CLR_RST_KEYPAD;
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_LOCK;

    // Reset keypad control state machine
    hwp_keypad->KP_CTRL = 0;
    
    // Wait for keypad hardware reset
    timer_delay_ms(5);

    // Register Keypad IRQ handler in OS vector table
    os_register_irq_handler(IRQ_INDEX_KEYPAD, keypad_irq_handler);

    // Configure Keypad Control Register:
    // Enable Keypad, set fast Debounce Time & Interval Time,
    // and enable all 8 input and 8 output matrix lines.
    hwp_keypad->KP_CTRL = KEYPAD_KP_EN |
                          KEYPAD_KP_DBN_TIME(2) |
                          KEYPAD_KP_ITV_TIME(2) |
                          KEYPAD_KP_IN_MASK(0xFF) |
                          KEYPAD_KP_OUT_MASK(0xFF);

    // Enable Keypad hardware event mask for Key Down (EVT0) and Key Up (EVT1)
    hwp_keypad->KP_IRQ_MASK = KEYPAD_KP_EVT0_IRQ_MASK |
                              KEYPAD_KP_EVT1_IRQ_MASK;

    // Clear IRQ status
    hwp_keypad->KP_IRQ_CLR = 0xFFFFFFFF;
}

static BOOL g_keypad_irq_mode = TRUE;
static volatile UINT32 g_keypad_irq_count   = 0;
static volatile UINT32 g_keypad_press_count = 0;
static volatile INT32  g_active_key_code    = KEY_CODE_NONE;
static volatile INT32  g_last_key_code      = KEY_CODE_NONE;
static volatile BOOL   g_key_is_pressed     = FALSE;
static const char*     g_last_key_name_str  = "NONE";

void keypad_set_irq_mode(BOOL enabled)
{
    g_keypad_irq_mode = enabled;
}

const char* keypad_get_key_name(INT32 key_code)
{
    if (key_code == KEY_CODE_POWER) return "POWER";
    if (key_code == KEY_CODE_NONE)  return "NONE";

    switch (key_code) {
        case KEY_CODE_LS:    return "LS";
        case KEY_CODE_CALL:  return "CALL";
        case KEY_CODE_RS:    return "RS";
        case KEY_CODE_RIGHT: return "RIGHT";
        case KEY_CODE_STAR:  return "*";
        case KEY_CODE_0:     return "0";
        case KEY_CODE_HASH:  return "#";
        case KEY_CODE_LEFT:  return "LEFT";
        case KEY_CODE_7:     return "7";
        case KEY_CODE_8:     return "8";
        case KEY_CODE_9:     return "9";
        case KEY_CODE_DOWN:  return "DOWN";
        case KEY_CODE_4:     return "4";
        case KEY_CODE_5:     return "5";
        case KEY_CODE_6:     return "6";
        case KEY_CODE_OK:    return "OK";
        case KEY_CODE_1:     return "1";
        case KEY_CODE_2:     return "2";
        case KEY_CODE_3:     return "3";
        case KEY_CODE_UP:    return "UP";
        default: break;
    }

    static char fallback_buf[8];
    if (key_code >= 0 && key_code < 64) {
        fallback_buf[0] = 'K';
        fallback_buf[1] = '0' + (key_code / 10);
        fallback_buf[2] = '0' + (key_code % 10);
        fallback_buf[3] = '\0';
    } else {
        return "UNKNOWN";
    }
    return fallback_buf;
}

void keypad_scan(UINT32 *low_keys, UINT32 *high_keys, UINT8 *power_on)
{
    if (low_keys)
        *low_keys = hwp_keypad->KP_DATA_L;
    if (high_keys)
        *high_keys = hwp_keypad->KP_DATA_H;
    if (power_on)
        *power_on = (hwp_keypad->KP_STATUS & KEYPAD_KP_ON) ? 1 : 0;

    if (!g_keypad_irq_mode)
    {
        hwp_keypad->KP_IRQ_CLR = KEYPAD_KP_IRQ_CLR;
    }
}

INT32 keypad_get_active_key(void)
{
    // 1. In IRQ mode, return asynchronously latched active key directly
    if (g_keypad_irq_mode)
    {
        return g_active_key_code;
    }

    // 2. Direct hardware register scan fallback / live polling
    if (hwp_keypad->KP_STATUS & KEYPAD_KP_ON)
    {
        g_last_key_code = KEY_CODE_POWER;
        g_last_key_name_str = "POWER";
        g_key_is_pressed = TRUE;
        return KEY_CODE_POWER;
    }

    UINT32 data_l = hwp_keypad->KP_DATA_L;
    if (data_l != 0)
    {
        for (int i = 0; i < 32; i++)
        {
            if (data_l & (1UL << i))
            {
                g_last_key_code = i;
                g_last_key_name_str = keypad_get_key_name(i);
                g_key_is_pressed = TRUE;
                return i;
            }
        }
    }

    UINT32 data_h = hwp_keypad->KP_DATA_H;
    if (data_h != 0)
    {
        for (int i = 0; i < 32; i++)
        {
            if (data_h & (1UL << i))
            {
                g_last_key_code = i + 32;
                g_last_key_name_str = keypad_get_key_name(i + 32);
                g_key_is_pressed = TRUE;
                return i + 32;
            }
        }
    }

    g_key_is_pressed = FALSE;
    return KEY_CODE_NONE;
}

INT32 keypad_get_last_key(void)
{
    return g_last_key_code;
}

const char* keypad_get_last_key_name(void)
{
    return g_last_key_name_str;
}

BOOL keypad_is_pressed(void)
{
    return g_key_is_pressed;
}

BOOL keypad_is_any_pressed(void)
{
    return g_key_is_pressed || (g_active_key_code != KEY_CODE_NONE);
}

UINT32 keypad_get_irq_count(void)
{
    return g_keypad_irq_count;
}

UINT32 keypad_get_press_count(void)
{
    return g_keypad_press_count;
}

BOOL keypad_is_key_pressed(UINT8 key_index)
{
    if (key_index < 32)
    {
        return (hwp_keypad->KP_DATA_L & (1UL << key_index)) ? TRUE : FALSE;
    }
    else if (key_index < 64)
    {
        return (hwp_keypad->KP_DATA_H & (1UL << (key_index - 32))) ? TRUE : FALSE;
    }
    return FALSE;
}

// =============================================================================
//  KEYPAD IRQ HANDLER (Asynchronous Key Event Capture)
// =============================================================================
void keypad_irq_handler(uint32_t cause, uint32_t epc)
{
    (void)cause;
    (void)epc;
    g_keypad_irq_count++;

    // Decode active key directly from hardware registers
    INT32 detected_key = KEY_CODE_NONE;

    if (hwp_keypad->KP_STATUS & KEYPAD_KP_ON)
    {
        detected_key = KEY_CODE_POWER;
    }
    else
    {
        UINT32 data_l = hwp_keypad->KP_DATA_L;
        if (data_l != 0)
        {
            for (int i = 0; i < 32; i++)
            {
                if (data_l & (1UL << i))
                {
                    detected_key = i;
                    break;
                }
            }
        }
        else
        {
            UINT32 data_h = hwp_keypad->KP_DATA_H;
            if (data_h != 0)
            {
                for (int i = 0; i < 32; i++)
                {
                    if (data_h & (1UL << i))
                    {
                        detected_key = i + 32;
                        break;
                    }
                }
            }
        }
    }

    if (detected_key != KEY_CODE_NONE)
    {
        g_active_key_code   = detected_key;
        g_last_key_code     = detected_key;
        g_last_key_name_str = keypad_get_key_name(detected_key);
        g_key_is_pressed    = TRUE;
        g_keypad_press_count++;
        os_log_printf("[KEYPAD_IRQ] Key DOWN: code=%d name=%s\n", detected_key, g_last_key_name_str);
    }
    else
    {
        // Key Up / Release event
        os_log_printf("[KEYPAD_IRQ] Key UP\n");
        g_active_key_code = KEY_CODE_NONE;
        g_key_is_pressed  = FALSE;
    }

    // Clear ALL keypad IRQ cause bits (EVT0, EVT1, ITV)
    hwp_keypad->KP_IRQ_CLR = 0xFFFFFFFF;

    extern TaskHandle_t g_keypad_task_handle;
    if (g_keypad_task_handle) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(g_keypad_task_handle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}


/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 */

#ifndef _KEYPAD_H_
#define _KEYPAD_H_

#include "cs_types.h"

// =============================================================================
//  MACROS & REGISTERS
// =============================================================================
#define KEY_NB                                   (64)
#define LOW_KEY_NB                               (32)
#define HIGH_KEY_NB                              (32)

#define REG_KEYPAD_BASE                          0x01A05000

typedef volatile struct
{
    REG32                          KP_DATA_L;                    //0x00000000
    REG32                          KP_DATA_H;                    //0x00000004
    REG32                          KP_STATUS;                    //0x00000008
    REG32                          KP_CTRL;                      //0x0000000C
    REG32                          KP_IRQ_MASK;                  //0x00000010
    REG32                          KP_IRQ_CAUSE;                 //0x00000014
    REG32                          KP_IRQ_CLR;                   //0x00000018
} HWP_KEYPAD_T;

#define hwp_keypad                  ((HWP_KEYPAD_T*) KSEG1(REG_KEYPAD_BASE))

// KP_DATA_L
#define KEYPAD_KP_DATA_L(n)         (((n)&0xFFFFFFFF)<<0)
#define KEYPAD_KP_DATA_L_MASK       (0xFFFFFFFF<<0)

// KP_DATA_H
#define KEYPAD_KP_DATA_H(n)         (((n)&0xFFFFFFFF)<<0)
#define KEYPAD_KP_DATA_H_MASK       (0xFFFFFFFF<<0)

// KP_STATUS
#define KEYPAD_KEYIN_STATUS(n)      (((n)&0xFF)<<0)
#define KEYPAD_KP_ON                (1<<31)

// KP_CTRL
#define KEYPAD_KP_EN                (1<<0)
#define KEYPAD_KP_DBN_TIME(n)       (((n)&0xFF)<<2)
#define KEYPAD_KP_ITV_TIME(n)       (((n)&0x3F)<<10)
#define KEYPAD_KP_IN_MASK(n)        (((n)&0xFF)<<16)
#define KEYPAD_KP_OUT_MASK(n)       (((n)&0xFF)<<24)

// KP_IRQ
#define KEYPAD_KP_EVT0_IRQ_MASK     (1<<0)
#define KEYPAD_KP_EVT1_IRQ_MASK     (1<<1)
#define KEYPAD_KP_ITV_IRQ_MASK      (1<<2)
#define KEYPAD_KP_IRQ_CLR           (1<<0)

// Standard RDA8809 Key Code Definitions
typedef enum {
    KEY_CODE_NONE   = -1,
    KEY_CODE_LS     = 0,   // Left Softkey
    KEY_CODE_CALL   = 1,   // Call / Dial
    KEY_CODE_RS     = 2,   // Right Softkey
    KEY_CODE_RIGHT  = 3,   // Nav Right
    KEY_CODE_STAR   = 8,   // *
    KEY_CODE_0      = 9,   // 0
    KEY_CODE_HASH   = 10,  // #
    KEY_CODE_LEFT   = 11,  // Nav Left
    KEY_CODE_7      = 16,  // 7
    KEY_CODE_8      = 17,  // 8
    KEY_CODE_9      = 18,  // 9
    KEY_CODE_DOWN   = 19,  // Nav Down
    KEY_CODE_4      = 24,  // 4
    KEY_CODE_5      = 25,  // 5
    KEY_CODE_6      = 26,  // 6
    KEY_CODE_OK     = 27,  // Center OK
    KEY_CODE_1      = 32,  // 1
    KEY_CODE_2      = 33,  // 2
    KEY_CODE_3      = 34,  // 3
    KEY_CODE_UP     = 35,  // Nav Up
    KEY_CODE_POWER  = 255  // Power Button
} keypad_key_code_t;

// Keypad Driver API
void keypad_init(void);
void keypad_set_irq_mode(BOOL enabled);
void keypad_irq_handler(uint32_t cause, uint32_t epc);
void keypad_scan(UINT32 *low_keys, UINT32 *high_keys, UINT8 *power_on);
INT32 keypad_get_active_key(void);
INT32 keypad_get_last_key(void);
const char* keypad_get_last_key_name(void);
BOOL keypad_is_key_pressed(UINT8 key_index);
BOOL keypad_is_any_pressed(void);
const char* keypad_get_key_name(INT32 key_code);
UINT32 keypad_get_irq_count(void);
UINT32 keypad_get_press_count(void);

#endif // _KEYPAD_H_

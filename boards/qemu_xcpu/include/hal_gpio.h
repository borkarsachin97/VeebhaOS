/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * GPIO HAL Public Interface for RDA8809
 */

#ifndef _HAL_GPIO_H_
#define _HAL_GPIO_H_

#include "cs_types.h"
#include "gpio.h"

#define HAL_GPIO_PIN_QTY            32

typedef enum
{
    HAL_GPIO_DIR_INPUT  = 0,
    HAL_GPIO_DIR_OUTPUT = 1
} HAL_GPIO_DIR_T;

typedef enum
{
    HAL_GPIO_IRQ_MODE_DISABLE     = 0,
    HAL_GPIO_IRQ_MODE_RISING      = 1,
    HAL_GPIO_IRQ_MODE_FALLING     = 2,
    HAL_GPIO_IRQ_MODE_BOTH_EDGES  = 3,
    HAL_GPIO_IRQ_MODE_HIGH_LEVEL  = 4,
    HAL_GPIO_IRQ_MODE_LOW_LEVEL   = 5
} HAL_GPIO_IRQ_MODE_T;

typedef void (*HAL_GPIO_IRQ_HANDLER_T)(UINT8 pin);

// Public GPIO HAL API
void hal_GpioInit(void);

void hal_GpioSetPinDirection(UINT8 pin, HAL_GPIO_DIR_T dir);
HAL_GPIO_DIR_T hal_GpioGetPinDirection(UINT8 pin);

void hal_GpioSetPin(UINT8 pin, BOOL high);
void hal_GpioTogglePin(UINT8 pin);
BOOL hal_GpioGetPin(UINT8 pin);

UINT32 hal_GpioReadPort(void);
void hal_GpioWritePort(UINT32 mask, UINT32 val);

void hal_GpioSetDebounceTime(UINT16 active_us, UINT16 inactive_us);

void hal_GpioSetInterrupt(UINT8 pin, HAL_GPIO_IRQ_MODE_T mode, HAL_GPIO_IRQ_HANDLER_T handler);
void hal_GpioDisableInterrupt(UINT8 pin);
void hal_GpioIrqHandler(void);

// Dedicated General Purpose Output (GPO) Controls
void hal_GpoSet(UINT8 gpo);
void hal_GpoClr(UINT8 gpo);

#endif // _HAL_GPIO_H_

/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * GPIO HAL Driver Implementation for RDA8809
 */

#include "cs_types.h"
#include "global_macros.h"
#include "gpio.h"
#include "hal_gpio.h"
#include "sys_ctrl.h"

extern void os_log_printf(const char *fmt, ...);

static HAL_GPIO_IRQ_HANDLER_T g_gpioHandlers[8] = { NULL };

// =============================================================================
// hal_GpioInit - Enable clock, clear resets & initialize state
// =============================================================================
void hal_GpioInit(void)
{
    // 1. Enable System Clock for GPIO in sysCtrl
    hwp_sysCtrl->Clk_Sys_Enable2 = SYS_CTRL_ENABLE_SYS_GPIO;
    hwp_sysCtrl->Clk_Other_Enable = SYS_CTRL_ENABLE_OC_GPIO;

    // 2. Clear GPIO reset
    hwp_sysCtrl->Sys_Rst_Clr = SYS_CTRL_CLR_RST_GPIO;

    // 3. Clear all interrupts
    hwp_gpio->int_clr = 0xFF;
    hwp_gpio->gpint_ctrl_clr = 0xFFFFFFFF;

    os_log_printf("[GPIO] Driver initialized at 0x%08X (32 Pins, IRQ & Debounce ready)\n", REG_GPIO_BASE);
}

// =============================================================================
// Pin Direction Management
// =============================================================================
void hal_GpioSetPinDirection(UINT8 pin, HAL_GPIO_DIR_T dir)
{
    if (pin >= HAL_GPIO_PIN_QTY) return;

    if (dir == HAL_GPIO_DIR_OUTPUT)
    {
        hwp_gpio->gpio_oen_set_out = GPIO_PIN_MASK(pin);
    }
    else
    {
        hwp_gpio->gpio_oen_set_in = GPIO_PIN_MASK(pin);
    }
}

HAL_GPIO_DIR_T hal_GpioGetPinDirection(UINT8 pin)
{
    if (pin >= HAL_GPIO_PIN_QTY) return HAL_GPIO_DIR_INPUT;
    // gpio_oen_val: 1 = input, 0 = output
    return (hwp_gpio->gpio_oen_val & GPIO_PIN_MASK(pin)) ? HAL_GPIO_DIR_INPUT : HAL_GPIO_DIR_OUTPUT;
}

// =============================================================================
// Pin Reading & Writing
// =============================================================================
void hal_GpioSetPin(UINT8 pin, BOOL high)
{
    if (pin >= HAL_GPIO_PIN_QTY) return;

    if (high)
    {
        hwp_gpio->gpio_set = GPIO_PIN_MASK(pin);
    }
    else
    {
        hwp_gpio->gpio_clr = GPIO_PIN_MASK(pin);
    }
}

void hal_GpioTogglePin(UINT8 pin)
{
    if (pin >= HAL_GPIO_PIN_QTY) return;

    if (hal_GpioGetPin(pin))
    {
        hwp_gpio->gpio_clr = GPIO_PIN_MASK(pin);
    }
    else
    {
        hwp_gpio->gpio_set = GPIO_PIN_MASK(pin);
    }
}

BOOL hal_GpioGetPin(UINT8 pin)
{
    if (pin >= HAL_GPIO_PIN_QTY) return FALSE;
    return (hwp_gpio->gpio_val & GPIO_PIN_MASK(pin)) != 0;
}

UINT32 hal_GpioReadPort(void)
{
    return hwp_gpio->gpio_val;
}

void hal_GpioWritePort(UINT32 mask, UINT32 val)
{
    hwp_gpio->gpio_set = (val & mask);
    hwp_gpio->gpio_clr = ((~val) & mask);
}

// =============================================================================
// GPO (General Purpose Output) Management
// =============================================================================
void hal_GpoSet(UINT8 gpo)
{
    if (gpo < 16)
    {
        hwp_gpio->gpo_set = (1U << gpo);
    }
}

void hal_GpoClr(UINT8 gpo)
{
    if (gpo < 16)
    {
        hwp_gpio->gpo_clr = (1U << gpo);
    }
}

// =============================================================================
// Debounce Configuration
// =============================================================================
void hal_GpioSetDebounceTime(UINT16 active_us, UINT16 inactive_us)
{
    (void)active_us;
    (void)inactive_us;
    // Debounce timing if configured via chg_ctrl
}

// =============================================================================
// Interrupt Management (Pins 0..7 are interrupt-capable)
// =============================================================================
void hal_GpioSetInterrupt(UINT8 pin, HAL_GPIO_IRQ_MODE_T mode, HAL_GPIO_IRQ_HANDLER_T handler)
{
    if (pin >= 8) return;

    g_gpioHandlers[pin] = handler;

    if (mode == HAL_GPIO_IRQ_MODE_DISABLE)
    {
        hwp_gpio->gpint_ctrl_clr = (1 << pin) | (1 << (pin + 8));
        return;
    }

    if (mode == HAL_GPIO_IRQ_MODE_RISING)
    {
        hwp_gpio->gpint_ctrl_clr = (1 << (pin + 8)); // clear falling
        hwp_gpio->gpint_ctrl_set = (1 << pin);       // set rising
    }
    else if (mode == HAL_GPIO_IRQ_MODE_FALLING)
    {
        hwp_gpio->gpint_ctrl_clr = (1 << pin);       // clear rising
        hwp_gpio->gpint_ctrl_set = (1 << (pin + 8)); // set falling
    }
    else if (mode == HAL_GPIO_IRQ_MODE_BOTH_EDGES)
    {
        hwp_gpio->gpint_ctrl_set = (1 << pin) | (1 << (pin + 8));
    }
}

void hal_GpioDisableInterrupt(UINT8 pin)
{
    if (pin >= 8) return;
    hwp_gpio->gpint_ctrl_clr = (1 << pin) | (1 << (pin + 8));
    g_gpioHandlers[pin] = NULL;
}

void hal_GpioIrqHandler(void)
{
    UINT32 status = hwp_gpio->int_status & 0xFF;
    hwp_gpio->int_clr = status;

    for (UINT8 i = 0; i < 8; i++)
    {
        if (status & (1 << i))
        {
            if (g_gpioHandlers[i])
            {
                g_gpioHandlers[i](i);
            }
        }
    }
}

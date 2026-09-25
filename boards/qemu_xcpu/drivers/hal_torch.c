#include "cs_types.h"
#include "global_macros.h"
#include "hal_gpio.h"
#include "hal_ispi.h"
#include "hal_torch.h"

extern void os_log_printf(const char *fmt, ...);

#define TORCH_GPO_PIN           8
#define TORCH_GPIO_PIN          7

static bool             g_torch_on = false;
static uint8_t          g_torch_level = 7;
static uint8_t          g_torch_chan_mask = TORCH_CHAN_ALL;
static bool             g_torch_init_done = false;

// =============================================================================
// hal_TorchInit
// =============================================================================
void hal_TorchInit(void)
{
    if (g_torch_init_done) return;

    // Initialize GPIO Subsystem if not already done
    hal_GpioInit();

    // Ensure dedicated GPO Pin 8 and GPIO Pin 7 are low (OFF)
    hal_GpoClr(TORCH_GPO_PIN);
    hal_GpioSetPinDirection(TORCH_GPIO_PIN, HAL_GPIO_DIR_OUTPUT);
    hal_GpioSetPin(TORCH_GPIO_PIN, FALSE);

    // Turn off PMU LED current sinks (0xFE00)
    hal_PmuWrite(0x3E, 0xFE00);

    g_torch_on = false;
    g_torch_level = 7;
    g_torch_chan_mask = TORCH_CHAN_ALL;
    g_torch_init_done = true;

    os_log_printf("[TORCH] Torch/Flashlight LED Driver ready (GPO %u, GPIO %u, PMU LED Sinks)\n",
                  TORCH_GPO_PIN, TORCH_GPIO_PIN);
}

// =============================================================================
// hal_TorchSetChannels
// =============================================================================
void hal_TorchSetChannels(uint8_t channel_mask, uint8_t level)
{
    if (!g_torch_init_done)
    {
        hal_TorchInit();
    }

    if (level > 7) level = 7;

    g_torch_chan_mask = channel_mask;
    g_torch_level = level;
    g_torch_on = (level > 0 && channel_mask != 0);

    if (g_torch_on)
    {
        // 1. Drive GPO 8 High
        hal_GpoSet(TORCH_GPO_PIN);

        // 2. Drive GPIO 7 High
        hal_GpioSetPinDirection(TORCH_GPIO_PIN, HAL_GPIO_DIR_OUTPUT);
        hal_GpioSetPin(TORCH_GPIO_PIN, TRUE);

        // 3. Enable PMU BLLED / LED Power Supply (Reg 0x02 bit 2, Reg 0x03 bit 2)
        UINT16 reg02 = hal_PmuRead(PMU_REG_LDO_SETTINGS);
        reg02 |= (1 << 2);
        hal_PmuWrite(PMU_REG_LDO_SETTINGS, reg02);

        UINT16 reg03 = hal_PmuRead(PMU_REG_LDO_ACTIVE1);
        reg03 &= ~(1 << 2);
        hal_PmuWrite(PMU_REG_LDO_ACTIVE1, reg03);

        // 4. Activate PMU Reg 0x3E LED current sinks (R, G, B channels at requested level)
        UINT16 reg3e = 0x8000;
        if (channel_mask & TORCH_CHAN_R) reg3e |= ((level & 0x7) << 6);
        if (channel_mask & TORCH_CHAN_G) reg3e |= ((level & 0x7) << 3);
        if (channel_mask & TORCH_CHAN_B) reg3e |= ((level & 0x7) << 0);
        hal_PmuWrite(0x3E, reg3e);
    }
    else
    {
        // 1. Drive GPO 8 Low
        hal_GpoClr(TORCH_GPO_PIN);

        // 2. Drive GPIO 7 Low
        hal_GpioSetPin(TORCH_GPIO_PIN, FALSE);

        // 3. Turn off PMU Reg 0x3E Current Sinks (all off bits set = 0xFE00)
        hal_PmuWrite(0x3E, 0xFE00);

        // 4. Gate PMU BLLED LDO (Reg 0x03 bit 2 = 1, Reg 0x02 bit 2 = 0)
        UINT16 reg03 = hal_PmuRead(PMU_REG_LDO_ACTIVE1);
        reg03 |= (1 << 2);
        hal_PmuWrite(PMU_REG_LDO_ACTIVE1, reg03);

        UINT16 reg02 = hal_PmuRead(PMU_REG_LDO_SETTINGS);
        reg02 &= ~(1 << 2);
        hal_PmuWrite(PMU_REG_LDO_SETTINGS, reg02);
    }
}

// =============================================================================
// hal_TorchSetLevel
// =============================================================================
void hal_TorchSetLevel(uint8_t level)
{
    hal_TorchSetChannels(g_torch_chan_mask, level);
}

// =============================================================================
// hal_TorchSet
// =============================================================================
void hal_TorchSet(bool enable)
{
    if (enable)
    {
        hal_TorchSetChannels(g_torch_chan_mask, g_torch_level ? g_torch_level : 7);
    }
    else
    {
        hal_TorchSetChannels(g_torch_chan_mask, 0);
    }
}

// =============================================================================
// hal_TorchToggle
// =============================================================================
bool hal_TorchToggle(void)
{
    hal_TorchSet(!g_torch_on);
    return g_torch_on;
}

// =============================================================================
// hal_TorchIsOn / hal_TorchGetState
// =============================================================================
bool hal_TorchIsOn(void)
{
    return g_torch_on;
}

bool hal_TorchGetState(void)
{
    return g_torch_on;
}

// =============================================================================
// hal_TorchGetLevel
// =============================================================================
uint8_t hal_TorchGetLevel(void)
{
    return g_torch_level;
}

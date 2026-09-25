/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Standalone Power Management Driver (PMD) for RDA8809 / RDA1203 PMU
 * Communicates exclusively through ISPI CS0 (PMU).
 */

#include "cs_types.h"
#include "hal_ispi.h"
#include "hal_pmd.h"
#include "irq.h"

// Backlight level-to-current mapping (0 to 15 current sink steps)
static const UINT8 g_bl_level_to_ibit[8] = {
    0,   // Level 0: Off
    2,   // Level 1: Very Low (~2mA)
    4,   // Level 2: Low (~5mA)
    6,   // Level 3: Medium-Low (~8mA)
    8,   // Level 4: Medium (~12mA)
    10,  // Level 5: Medium-High (~16mA)
    12,  // Level 6: High (~20mA)
    15   // Level 7: Maximum (~25mA)
};

// Cached register states
static UINT16 g_pmd_ldo_settings = 0x0EF0;
static UINT16 g_pmd_ldo_act1     = 0xA769;
static UINT16 g_pmd_ldo_act2     = 0x8894;
static UINT16 g_pmd_led_setting1 = 0x00AA;
static UINT16 g_pmd_led_setting2 = 0x8F8A;
static UINT16 g_pmd_led_setting5 = 0xFE00;
static UINT16 g_pmd_tcal         = 0xFE20;
static BOOL   g_pmd_initialized  = FALSE;

static UINT8  g_pmd_bl_level     = 6;
static BOOL   g_pmd_kpled_on     = FALSE;
static BOOL   g_pmd_vibr_on      = FALSE;

static inline UINT32 pmd_enter_cs(void)
{
    UINT32 status = cp0_get_status();
    cp0_set_status(status & ~CP0_STATUS_IE);
    return status;
}

static inline void pmd_exit_cs(UINT32 status)
{
    cp0_set_status(status);
}

// ============================================================================
// hal_PmdGetRevision
// ============================================================================
UINT16 hal_PmdGetRevision(void)
{
    return hal_PmdReadReg(PMU_REG_REVISION_ID);
}

// ============================================================================
// hal_PmdReadReg - Safe direct register read
// ============================================================================
UINT16 hal_PmdReadReg(UINT16 reg)
{
    UINT32 status = pmd_enter_cs();
    UINT16 val = hal_PmuRead(reg);
    pmd_exit_cs(status);
    return val;
}

// ============================================================================
// hal_PmdGetCachedAct1
// ============================================================================
UINT16 hal_PmdGetCachedAct1(void)
{
    return g_pmd_ldo_act1;
}

// ============================================================================
// hal_PmdGetCachedAct2
// ============================================================================
UINT16 hal_PmdGetCachedAct2(void)
{
    return g_pmd_ldo_act2;
}

// ============================================================================
// hal_PmdGetCachedLed2
// ============================================================================
UINT16 hal_PmdGetCachedLed2(void)
{
    return g_pmd_led_setting2;
}

// ============================================================================
// hal_PmdGetCachedLed5
// ============================================================================
UINT16 hal_PmdGetCachedLed5(void)
{
    return g_pmd_led_setting5;
}

// ============================================================================
// hal_PmdGetBacklight
// ============================================================================
UINT8 hal_PmdGetBacklight(void)
{
    return g_pmd_bl_level;
}

// ============================================================================
// hal_PmdGetKeypadLed
// ============================================================================
BOOL hal_PmdGetKeypadLed(void)
{
    return g_pmd_kpled_on;
}

// ============================================================================
// hal_PmdGetVibrator
// ============================================================================
BOOL hal_PmdGetVibrator(void)
{
    return g_pmd_vibr_on;
}

// ============================================================================
// hal_PmdInit - Cold-boot PMU & Backlight Initialization
// ============================================================================
void hal_PmdInit(void)
{
    UINT32 status = pmd_enter_cs();

    // 1. Verify ISPI communication with PMU
    hal_IspiInit();
    hal_PmuRead(PMU_REG_REVISION_ID);

    // 2. Configure Charge Pump setting (supplies boost voltage for backlight LEDs)
    hal_PmuWrite(PMU_REG_SPEAKER_PA1, 0xC003);

    // 3. Configure VRGB_LED forward voltage boost in LDO_ACTIVE5
    //    VRGB_LED_VSEL = 5 (bits 5..3 = 0x28), sets LED sink voltage to 2.8V+ boost
    hal_PmuWrite(PMU_REG_LDO_ACTIVE5, 0xF928);

    // 4. Enable software control for essential peripheral LDOs:
    //    VLCD (bit 4), VCAM (bit 5), VMIC (bit 6), VIBR (bit 7),
    //    VRF (bit 9), VABB (bit 10), VMMC (bit 11) -> 0x0EF0
    g_pmd_ldo_settings = PMU_LDO_EN_VLCD | PMU_LDO_EN_VCAM | PMU_LDO_EN_VMIC |
                         PMU_LDO_EN_VIBR | PMU_LDO_EN_VRF  | PMU_LDO_EN_VABB |
                         PMU_LDO_EN_VMMC;
    hal_PmuWrite(PMU_REG_LDO_SETTINGS, g_pmd_ldo_settings);

    // 5. Configure Active Setting 2:
    //    - RGB_LED_OFF = 0 (bit 0 = 0 -> RGB/Backlight LED power block enabled)
    //    - BUCK2_ON = 1
    //    - VLCD, VMMC, VPAD = 2.8V
    //    Value = 0x8894 (matches stock trace)
    g_pmd_ldo_act2 = 0x8894;
    hal_PmuWrite(PMU_REG_LDO_ACTIVE2, g_pmd_ldo_act2);

    // 6. Configure Active Setting 1:
    //    - NORMAL_MODE = 1
    //    - VSPIMEM_OFF = 0 (SPI Flash power ON)
    //    - VBLLED_OFF = 0  (CRITICAL: Backlight LED power rail ON!)
    //    - VUSB_OFF = 0    (USB PHY power ON)
    //    - VLCD_OFF = 0    (LCD display power ON!)
    //    - VPAD_OFF = 0    (I/O Pad power ON)
    //    - VMEM_OFF = 0    (Memory power ON)
    //    Value = 0xA769 (matches stock firmware boot trace)
    g_pmd_ldo_act1 = 0xA769;
    hal_PmuWrite(PMU_REG_LDO_ACTIVE1, g_pmd_ldo_act1);

    // 7. Configure Backlight & Keypad LED direct register control:
    //    DIM_BL_DR = 1, DIM_LED_B/G/R_DR = 1, PWM_BL_ENABLE = 0 (0x00AA)
    //    Enables the internal constant-current sinks directly.
    g_pmd_led_setting1 = PMU_LED1_DIM_BL_DR | PMU_LED1_DIM_LED_B_DR |
                         PMU_LED1_DIM_LED_G_DR | PMU_LED1_DIM_LED_R_DR;
    hal_PmuWrite(PMU_REG_LED_SETTING1, g_pmd_led_setting1);

    // 8. Thermal Calibration & High-Voltage Boost mode for Backlight & Speaker BTL Double mode:
    //    HV_MODE_BL (bit 5) enables high-voltage sink; DOUBLE_MODE_EN_CLG (bit 7) enables BTL inverter
    g_pmd_tcal = 0xFE20 | PMU_TCAL_HV_MODE_BL | PMU_TCAL_DOUBLE_MODE_EN_CLG;
    hal_PmuWrite(PMU_REG_THERMAL_CALIBRATION, g_pmd_tcal);

    // 9. Initial Keypad LED state: Off (0xFE00)
    g_pmd_led_setting5 = 0xFE00;
    hal_PmuWrite(PMU_REG_LED_SETTING5, g_pmd_led_setting5);

    g_pmd_initialized = TRUE;
    pmd_exit_cs(status);

    // 10. Turn on backlight to initial default level 6 (Bright)
    hal_PmdSetBacklight(6);
}

// ============================================================================
// hal_PmdSetBacklight - Set Backlight Brightness Level (0..7)
// ============================================================================
void hal_PmdSetBacklight(UINT8 level)
{
    if (level > 7)
    {
        level = 7;
    }

    UINT32 status = pmd_enter_cs();

    g_pmd_bl_level = level;

    if (level == 0)
    {
        // 1. Power OFF Backlight LED rail in LDO_ACTIVE1 (Bit 2: VBLLED_OFF = 1)
        g_pmd_ldo_act1 |= PMU_ACT1_VBLLED_OFF;
        hal_PmuWrite(PMU_REG_LDO_ACTIVE1, g_pmd_ldo_act1);

        // 2. Shut off Backlight current sink in LED_SETTING2 (Bit 2: BL_OFF_ACT = 1)
        g_pmd_led_setting2 |= PMU_LED2_BL_OFF_ACT;
        hal_PmuWrite(PMU_REG_LED_SETTING2, g_pmd_led_setting2);

        // 3. Set dim register bit in LED_SETTING1
        g_pmd_led_setting1 |= PMU_LED1_DIM_BL_REG;
        hal_PmuWrite(PMU_REG_LED_SETTING1, g_pmd_led_setting1);
    }
    else
    {
        UINT8 ibit = g_bl_level_to_ibit[level];

        // 1. Power ON Backlight LED rail in LDO_ACTIVE1 (Bit 2: VBLLED_OFF = 0)
        g_pmd_ldo_act1 &= ~PMU_ACT1_VBLLED_OFF;
        hal_PmuWrite(PMU_REG_LDO_ACTIVE1, g_pmd_ldo_act1);

        // 2. Ensure boosted VRGB_LED forward voltage (VRGB_LED_VSEL = 5) in LDO_ACTIVE5
        hal_PmuWrite(PMU_REG_LDO_ACTIVE5, 0xF928);

        // 3. Ensure RGB LED block is powered in LDO_ACTIVE2 (Bit 0: RGB_LED_OFF = 0)
        g_pmd_ldo_act2 &= ~PMU_ACT2_RGB_LED_OFF;
        hal_PmuWrite(PMU_REG_LDO_ACTIVE2, g_pmd_ldo_act2);

        // 4. Ensure Charge pump & speaker PA are enabled for VBP boost
        hal_PmuWrite(PMU_REG_SPEAKER_PA1, 0xC003);

        // 5. Clear BL_OFF_ACT (bit 2) and write IBIT (bits 8..11) in LED_SETTING2
        //    Base is 0x808A (matches stock trace 0x8F8A for max, 0x828A for low)
        g_pmd_led_setting2 = (0x808A & ~PMU_LED2_BL_OFF_ACT) | PMU_LED2_BL_IBIT_ACT(ibit);
        hal_PmuWrite(PMU_REG_LED_SETTING2, g_pmd_led_setting2);

        // 6. Direct register control and clear dim bit in LED_SETTING1
        g_pmd_led_setting1 &= ~PMU_LED1_DIM_BL_REG;
        g_pmd_led_setting1 |= PMU_LED1_DIM_BL_DR;
        hal_PmuWrite(PMU_REG_LED_SETTING1, g_pmd_led_setting1);

        // 7. High-Voltage boost mode in Thermal Calibration (preserving DOUBLE_MODE_EN_CLG for BTL speaker)
        g_pmd_tcal = 0xFE20 | PMU_TCAL_HV_MODE_BL | (g_pmd_tcal & PMU_TCAL_DOUBLE_MODE_EN_CLG);
        hal_PmuWrite(PMU_REG_THERMAL_CALIBRATION, g_pmd_tcal);
    }

    pmd_exit_cs(status);
}

// ============================================================================
// hal_PmdSetKeypadLed - Control Keypad RGB / White LEDs
// ============================================================================
void hal_PmdSetKeypadLed(BOOL on, UINT8 level)
{
    if (level > 7)
    {
        level = 7;
    }

    UINT32 status = pmd_enter_cs();

    g_pmd_kpled_on = on && (level > 0);

    if (!on || level == 0)
    {
        // Turn off Keypad RGB LEDs (0xFE00: bits 14..12 = 1)
        g_pmd_led_setting5 = 0xFE00;
        hal_PmuWrite(PMU_REG_LED_SETTING5, g_pmd_led_setting5);
    }
    else
    {
        // 1. Ensure RGB LED block is powered in ACTIVE2
        g_pmd_ldo_act2 &= ~PMU_ACT2_RGB_LED_OFF;
        hal_PmuWrite(PMU_REG_LDO_ACTIVE2, g_pmd_ldo_act2);

        // 2. Stock firmware writes 0xEC01:
        //    Bit 15: GBIT_ABB_EN = 1 (enables ABB bandgap / bias for LED current sinks)
        //    Bit 14: LED_R_OFF_ACT = 1 (Red LED off)
        //    Bit 13: LED_G_OFF_ACT = 1 (Green LED off)
        //    Bit 12: LED_B_OFF_ACT = 0 (Blue/White keypad LED sink active!)
        //    Bit 11..9: 0x6 (LP off bits)
        //    Bit 2..0: LED_B_IBIT = ibit
        UINT8 ibit = (level > 0) ? (level & 0x7) : 1;
        g_pmd_led_setting5 = 0xEC00 | (ibit & 0x7);
        hal_PmuWrite(PMU_REG_LED_SETTING5, g_pmd_led_setting5);
    }

    pmd_exit_cs(status);
}

// ============================================================================
// hal_PmdEnableLdo - Enable / Disable Specific Peripheral LDO Rails
// ============================================================================
void hal_PmdEnableLdo(UINT32 ldo_mask, BOOL on)
{
    UINT32 status = pmd_enter_cs();

    // Read live PMU state to ensure we never clobber rails enabled by other drivers (e.g. SDMMC, LCD, USB)
    g_pmd_ldo_settings = hal_PmuRead(PMU_REG_LDO_SETTINGS);
    g_pmd_ldo_act1     = hal_PmuRead(PMU_REG_LDO_ACTIVE1);

    // Map LDO ID to PMU_REG_LDO_ACTIVE1 '*_OFF' bits (0=ON, 1=OFF)
    UINT16 act1_bits = 0;
    UINT16 ldo_ctrl_bits = 0;

    if (ldo_mask & PMD_LDO_ID_LCD)
    {
        act1_bits |= PMU_ACT1_VLCD_OFF;
        ldo_ctrl_bits |= PMU_LDO_EN_VLCD;
    }
    if (ldo_mask & PMD_LDO_ID_MMC)
    {
        act1_bits |= PMU_ACT1_VMMC_OFF;
        ldo_ctrl_bits |= PMU_LDO_EN_VMMC;
    }
    if (ldo_mask & PMD_LDO_ID_CAM)
    {
        act1_bits |= PMU_ACT1_VCAM_OFF;
        ldo_ctrl_bits |= PMU_LDO_EN_VCAM;
    }
    if (ldo_mask & PMD_LDO_ID_VIBR)
    {
        act1_bits |= PMU_ACT1_VIBR_OFF;
        ldo_ctrl_bits |= PMU_LDO_EN_VIBR;
        g_pmd_vibr_on = on;
    }
    if (ldo_mask & PMD_LDO_ID_MIC)
    {
        act1_bits |= PMU_ACT1_VMIC_OFF;
        ldo_ctrl_bits |= PMU_LDO_EN_VMIC;
    }
    if (ldo_mask & PMD_LDO_ID_ABB)
    {
        act1_bits |= PMU_ACT1_VABB_OFF;
        ldo_ctrl_bits |= PMU_LDO_EN_VABB;
    }
    if (ldo_mask & PMD_LDO_ID_USB)
    {
        act1_bits |= PMU_ACT1_VUSB_OFF;
    }
    if (ldo_mask & PMD_LDO_ID_BL)
    {
        act1_bits |= PMU_ACT1_VBLLED_OFF;
    }

    // Ensure software control is enabled for the requested LDOs
    g_pmd_ldo_settings |= ldo_ctrl_bits;
    hal_PmuWrite(PMU_REG_LDO_SETTINGS, g_pmd_ldo_settings);

    if (on)
    {
        // 0 = Rail Powered ON (active low)
        g_pmd_ldo_act1 &= ~act1_bits;
    }
    else
    {
        // 1 = Rail Powered OFF
        g_pmd_ldo_act1 |= act1_bits;
    }

    hal_PmuWrite(PMU_REG_LDO_ACTIVE1, g_pmd_ldo_act1);
    pmd_exit_cs(status);
}

// ============================================================================
// Convenience Functions
// ============================================================================
void hal_PmdSetLcdPower(BOOL on)
{
    hal_PmdEnableLdo(PMD_LDO_ID_LCD, on);
}

void hal_PmdSetMmcPower(BOOL on)
{
    hal_PmdEnableLdo(PMD_LDO_ID_MMC, on);
}

void hal_PmdSetVibrator(BOOL on)
{
    hal_PmdEnableLdo(PMD_LDO_ID_VIBR, on);
}

void hal_PmdEnableAudioPower(BOOL enable)
{
    hal_PmdEnableLdo(PMD_LDO_ID_ABB | PMD_LDO_ID_MIC, enable);
}

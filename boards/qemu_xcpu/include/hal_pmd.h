/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Standalone Power Management Driver (PMD) for RDA8809 / RDA1203 PMU
 * Communicates exclusively through ISPI CS0 (PMU).
 */

#ifndef _HAL_PMD_H_
#define _HAL_PMD_H_

#include "cs_types.h"
#include "hal_ispi.h"

// ============================================================================
// PMU Register Offsets (RDA1203 / RDA8809)
// ============================================================================
#define PMU_REG_REVISION_ID             0x00    // PMU Chip ID (0x8809)
#define PMU_REG_IRQ_SETTINGS            0x01    // Interrupt settings & power down
#define PMU_REG_LDO_SETTINGS            0x02    // LDO software control enables
#define PMU_REG_LDO_ACTIVE1             0x03    // Active mode LDO power profile (0=ON, 1=OFF)
#define PMU_REG_LDO_ACTIVE2             0x04    // Active mode voltage selectors & RGB_LED
#define PMU_REG_LDO_ACTIVE3             0x05    // Active mode bias currents
#define PMU_REG_LDO_ACTIVE4             0x06    // Active mode LDO bias
#define PMU_REG_LDO_ACTIVE5             0x07    // Active mode VRGB/VABB/VUSB bias
#define PMU_REG_LDO_LP1                 0x08    // Low-power mode LDO power profile
#define PMU_REG_LDO_LP2                 0x09    // Low-power mode voltage selectors
#define PMU_REG_CHARGER_STATUS          0x14    // Charger status (AC on, VREG)
#define PMU_REG_MISC_CONTROL            0x18    // 4M clock, AVDD3 LDO control
#define PMU_REG_LED_SETTING1            0x19    // Direct reg / PWM mode for BL & Keypad LEDs
#define PMU_REG_LED_SETTING2            0x1A    // Backlight current level & power state
#define PMU_REG_LED_SETTING3            0x1B    // Backlight PWM frequency & duty cycle
#define PMU_REG_SPEAKER_PA1             0x27    // Speaker PA / Charge pump control
#define PMU_REG_LDO_BUCK1               0x2D    // Buck 1 converter settings
#define PMU_REG_LDO_BUCK2               0x2E    // Buck 2 converter settings
#define PMU_REG_DCDC_BUCK               0x2F    // Core DCDC voltage levels
#define PMU_REG_PMU_RESET               0x30    // PMU soft & register reset
#define PMU_REG_THERMAL_CALIBRATION     0x36    // Backlight HV mode & thermal calibration
#define PMU_REG_LED_SETTING4            0x38    // RGB LED PWM duty cycle
#define PMU_REG_LED_SETTING5            0x3E    // RGB Keypad LED currents & power state

// ============================================================================
// Bit Definitions for LDO Settings (Register 0x02)
// (1 = software control enabled)
// ============================================================================
#define PMU_LDO_EN_FM                   (1 << 0)
#define PMU_LDO_EN_BT                   (1 << 1)
#define PMU_LDO_EN_KEY                  (1 << 2)
#define PMU_LDO_EN_TSC                  (1 << 3)
#define PMU_LDO_EN_VLCD                 (1 << 4)
#define PMU_LDO_EN_VCAM                 (1 << 5)
#define PMU_LDO_EN_VMIC                 (1 << 6)
#define PMU_LDO_EN_VIBR                 (1 << 7)
#define PMU_LDO_EN_VRF                  (1 << 9)
#define PMU_LDO_EN_VABB                 (1 << 10)
#define PMU_LDO_EN_VMMC                 (1 << 11)

// ============================================================================
// Bit Definitions for LDO Active Setting 1 (Register 0x03)
// (NOTE: Most rails use active-low '*_OFF' logic: 0 = ON, 1 = OFF)
// ============================================================================
#define PMU_ACT1_NORMAL_MODE            (1 << 0)    // 1 = Normal active profile
#define PMU_ACT1_VSPIMEM_OFF            (1 << 1)    // 0 = SPI Flash power ON
#define PMU_ACT1_VBLLED_OFF             (1 << 2)    // 0 = Backlight LED power ON, 1 = OFF
#define PMU_ACT1_VMIC_OFF               (1 << 3)    // 0 = Mic bias ON
#define PMU_ACT1_VUSB_OFF               (1 << 4)    // 0 = USB PHY power ON
#define PMU_ACT1_VIBR_OFF               (1 << 5)    // 0 = Vibrator ON
#define PMU_ACT1_VMMC_OFF               (1 << 6)    // 0 = MMC / SD card power ON
#define PMU_ACT1_VLCD_OFF               (1 << 7)    // 0 = LCD display power ON
#define PMU_ACT1_VCAM_OFF               (1 << 8)    // 0 = Camera power ON
#define PMU_ACT1_VRF_OFF                (1 << 9)    // 0 = RF power ON
#define PMU_ACT1_VABB_OFF               (1 << 10)   // 0 = Analog Baseband ON
#define PMU_ACT1_VPAD_OFF               (1 << 11)   // 0 = I/O Pad power ON
#define PMU_ACT1_VMEM_OFF               (1 << 12)   // 0 = Memory power ON

// ============================================================================
// Bit Definitions for LDO Active Setting 2 (Register 0x04)
// ============================================================================
#define PMU_ACT2_RGB_LED_OFF            (1 << 0)    // 0 = RGB/Keypad LED block ON
#define PMU_ACT2_BUCK2_ON               (1 << 4)    // BUCK2 ON
#define PMU_ACT2_VINTRF_OFF             (1 << 6)    // 0 = Interface power ON
#define PMU_ACT2_VIBR_VSEL_1_8          (1 << 7)    // 1 = 1.8V, 0 = 2.8V
#define PMU_ACT2_VMMC_VSEL_1_8          (1 << 8)    // 1 = 1.8V, 0 = 2.8V
#define PMU_ACT2_VLCD_VSEL_1_8          (1 << 9)    // 1 = 1.8V, 0 = 2.8V
#define PMU_ACT2_VCAM_VSEL_1_8          (1 << 10)   // 1 = 1.8V, 0 = 2.8V
#define PMU_ACT2_VRF_VSEL_1_8           (1 << 11)   // 1 = 1.8V, 0 = 2.8V
#define PMU_ACT2_VPAD_VSEL_1_8          (1 << 12)   // 1 = 1.8V, 0 = 2.8V

// ============================================================================
// Bit Definitions for LED Setting 1 (Register 0x19)
// ============================================================================
#define PMU_LED1_DIM_BL_REG             (1 << 0)    // BL level dim bit
#define PMU_LED1_DIM_BL_DR              (1 << 1)    // 1 = Direct register control of BL
#define PMU_LED1_DIM_LED_B_REG          (1 << 2)
#define PMU_LED1_DIM_LED_B_DR           (1 << 3)    // 1 = Direct reg control of Keypad B
#define PMU_LED1_DIM_LED_G_REG          (1 << 4)
#define PMU_LED1_DIM_LED_G_DR           (1 << 5)    // 1 = Direct reg control of Keypad G
#define PMU_LED1_DIM_LED_R_REG          (1 << 6)
#define PMU_LED1_DIM_LED_R_DR           (1 << 7)    // 1 = Direct reg control of Keypad R
#define PMU_LED1_PWM_RGB_PMU_MODE       (1 << 8)
#define PMU_LED1_PWM_BL_ENABLE          (1 << 9)    // 0 = Direct current sink, 1 = PWM

// ============================================================================
// Bit Definitions for LED Setting 2 (Register 0x1A) - Backlight Sink
// ============================================================================
#define PMU_LED2_BL_OFF_LP              (1 << 1)    // 1 = BL off in Low Power
#define PMU_LED2_BL_OFF_ACT             (1 << 2)    // 0 = BL ON in Active mode, 1 = OFF
#define PMU_LED2_BL_OFF_PON             (1 << 3)    // 1 = BL off in Power-On
#define PMU_LED2_BL_IBIT_ACT(n)         (((n) & 0xF) << 8)
#define PMU_LED2_BL_IBIT_ACT_MASK       (0xF << 8)
#define PMU_LED2_BL_IBIT_LP(n)          (((n) & 0xF) << 4)

// ============================================================================
// Bit Definitions for Thermal Calibration (Register 0x36)
// ============================================================================
#define PMU_TCAL_HV_MODE_BL             (1 << 5)    // 1 = High-voltage mode for backlight current sink
#define PMU_TCAL_IX2_BL                 (1 << 6)    // 1 = Double backlight current
#define PMU_TCAL_DOUBLE_MODE_EN_CLG     (1 << 7)    // 1 = Class-D/K Double BTL mode for differential speaker

// ============================================================================
// Bit Definitions for LED Setting 5 (Register 0x3E) - Keypad RGB LEDs
// ============================================================================
#define PMU_LED5_LED_B_IBIT(n)          (((n) & 0x7) << 0)
#define PMU_LED5_LED_G_IBIT(n)          (((n) & 0x7) << 3)
#define PMU_LED5_LED_R_IBIT(n)          (((n) & 0x7) << 6)
#define PMU_LED5_LED_B_OFF_LP           (1 << 9)
#define PMU_LED5_LED_G_OFF_LP           (1 << 10)
#define PMU_LED5_LED_R_OFF_LP           (1 << 11)
#define PMU_LED5_LED_B_OFF_ACT          (1 << 12)   // 0 = ON, 1 = OFF
#define PMU_LED5_LED_G_OFF_ACT          (1 << 13)   // 0 = ON, 1 = OFF
#define PMU_LED5_LED_R_OFF_ACT          (1 << 14)   // 0 = ON, 1 = OFF
#define PMU_LED5_GBIT_ABB_EN            (1 << 15)   // 1 = ABB bandgap/bias enabled for LED sinks

// ============================================================================
// LDO Power Rail Identifiers
// ============================================================================
typedef enum
{
    PMD_LDO_ID_LCD  = (1 << 0),     // VLCD (LCD controller & display logic)
    PMD_LDO_ID_MMC  = (1 << 1),     // VMMC (SD / MicroSD card)
    PMD_LDO_ID_CAM  = (1 << 2),     // VCAM (Camera sensor)
    PMD_LDO_ID_VIBR = (1 << 3),     // VIBR (Vibrator motor)
    PMD_LDO_ID_MIC  = (1 << 4),     // VMIC (Microphone bias)
    PMD_LDO_ID_ABB  = (1 << 5),     // VABB (Analog Baseband Audio Codec)
    PMD_LDO_ID_USB  = (1 << 6),     // VUSB (USB PHY)
    PMD_LDO_ID_BL   = (1 << 7),     // VBLLED (Backlight LED Power Rail)
} PMD_LDO_ID_T;

// ============================================================================
// Public PMD Driver API
// ============================================================================

/**
 * @brief Initialize the Power Management Unit (PMU) during cold boot.
 *
 * Configures LDO software controls, sets rail voltages to 2.8V (VLCD, VMMC, VPAD),
 * enables primary peripheral power rails, enables backlight High-Voltage mode,
 * and activates direct constant-current drive.
 */
void hal_PmdInit(void);

/**
 * @brief Set the LCD Backlight brightness level.
 *
 * @param level Brightness level [0..7]:
 *              0 = Off (current sink disabled, VBLLED rail powered off)
 *              1..7 = Scaled constant-current brightness (7 = maximum).
 */
void hal_PmdSetBacklight(UINT8 level);

/**
 * @brief Get currently configured LCD Backlight brightness level [0..7].
 */
UINT8 hal_PmdGetBacklight(void);

/**
 * @brief Enable or disable Keypad RGB LEDs.
 *
 * @param on    TRUE to turn on keypad illumination, FALSE to turn off.
 * @param level Brightness level [0..7].
 */
void hal_PmdSetKeypadLed(BOOL on, UINT8 level);

/**
 * @brief Get whether Keypad LED is currently active.
 */
BOOL hal_PmdGetKeypadLed(void);

/**
 * @brief Enable or disable individual peripheral LDO power rails.
 *
 * @param ldo_mask Bitwise OR of PMD_LDO_ID_* flags.
 * @param on       TRUE to enable rail, FALSE to power down.
 */
void hal_PmdEnableLdo(UINT32 ldo_mask, BOOL on);

/**
 * @brief Convenience helper to enable/disable the VLCD power rail.
 * @param on TRUE to power on, FALSE to power down.
 */
void hal_PmdSetLcdPower(BOOL on);

/**
 * @brief Convenience helper to enable/disable the VMMC (SD card) power rail.
 * @param on TRUE to power on, FALSE to power down.
 */
void hal_PmdSetMmcPower(BOOL on);

/**
 * @brief Convenience helper to trigger or stop the vibrator motor.
 * @param on TRUE to vibrate, FALSE to stop.
 */
void hal_PmdSetVibrator(BOOL on);

/**
 * @brief Get whether Vibrator is currently enabled.
 */
BOOL hal_PmdGetVibrator(void);

/**
 * @brief Get cached PMU_REG_LDO_ACTIVE1 register value.
 */
UINT16 hal_PmdGetCachedAct1(void);

/**
 * @brief Get cached PMU_REG_LDO_ACTIVE2 register value.
 */
UINT16 hal_PmdGetCachedAct2(void);

/**
 * @brief Get cached PMU_REG_LED_SETTING2 register value.
 */
UINT16 hal_PmdGetCachedLed2(void);

/**
 * @brief Get cached PMU_REG_LED_SETTING5 register value.
 */
UINT16 hal_PmdGetCachedLed5(void);

/**
 * @brief Direct safe read of any PMU register via ISPI.
 */
UINT16 hal_PmdReadReg(UINT16 reg);

/**
 * @brief Read the PMU hardware revision ID.
 * @return 16-bit hardware revision (expected 0x8809 on RDA8809).
 */
UINT16 hal_PmdGetRevision(void);

/**
 * @brief Enable or disable PMU power rails for audio (VABB & VMIC).
 * @param enable TRUE to energize VABB and VMIC, FALSE to power them down.
 */
void hal_PmdEnableAudioPower(BOOL enable);

#endif // _HAL_PMD_H_

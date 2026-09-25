/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Generic Hardware Abstraction Layer (HAL) Export Interface
 *
 * This header defines generic hardware access APIs for VeebhaOS applications
 * and OS services. All hardware-specific register manipulations and board drivers
 * are implemented inside each target board's hwlayer.c file.
 */

#ifndef SDK_INCLUDE_VEEBHA_HARDWARE_H
#define SDK_INCLUDE_VEEBHA_HARDWARE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ============================================================================
 * 1. TORCH / FLASHLIGHT API
 * ============================================================================ */

/**
 * Turn Torch ON or OFF.
 */
void veebha_hw_torch_set(bool on);

/**
 * Get current Torch state.
 */
bool veebha_hw_torch_get(void);

/**
 * Toggle Torch state.
 */
void veebha_hw_torch_toggle(void);


/* ============================================================================
 * 2. DISPLAY BACKLIGHT & BRIGHTNESS API
 * ============================================================================ */

/**
 * Set LCD backlight brightness (0 to 100%).
 */
void veebha_hw_backlight_set(uint8_t level_pct);

/**
 * Get current LCD backlight brightness level (0 to 100%).
 */
uint8_t veebha_hw_backlight_get(void);


/* ============================================================================
 * 3. HARDWARE REAL-TIME CLOCK (RTC) & CALENDAR API
 * ============================================================================ */

/**
 * Read current time from hardware RTC.
 */
bool veebha_hw_rtc_get_time(uint8_t *hour, uint8_t *min, uint8_t *sec);

/**
 * Program current time into hardware RTC registers.
 */
bool veebha_hw_rtc_set_time(uint8_t hour, uint8_t min, uint8_t sec);

/**
 * Read current date from hardware RTC.
 */
bool veebha_hw_rtc_get_date(uint16_t *year, uint8_t *month, uint8_t *day);

/**
 * Program current date into hardware RTC registers.
 */
bool veebha_hw_rtc_set_date(uint16_t year, uint8_t month, uint8_t day);


/* ============================================================================
 * 4. LOW POWER SCHEME (LPS) & DISPLAY SLEEP API
 * ============================================================================ */

/**
 * Initialize LPS engine with default auto-sleep timeout in seconds (e.g. 10).
 */
void veebha_hw_lps_init(uint32_t auto_sleep_sec);

/**
 * Reset user activity countdown timer (called on any keypad event).
 * If display is currently sleeping, automatically wakes up the display.
 */
void veebha_hw_lps_reset_activity(void);

/**
 * Set auto-sleep inactivity timeout in seconds (0 = disabled).
 */
void veebha_hw_lps_set_timeout(uint32_t timeout_sec);

/**
 * Get auto-sleep inactivity timeout in seconds.
 */
uint32_t veebha_hw_lps_get_timeout(void);

/**
 * Check if the auto-sleep inactivity timer has expired.
 */
bool veebha_hw_lps_is_expired(void);

/**
 * Put the display panel & backlight to sleep (blank / low power).
 */
void veebha_hw_lps_sleep_display(void);

/**
 * Wake the display panel & restore backlight.
 */
void veebha_hw_lps_wake_display(void);

/**
 * Check if the display is currently in low power sleep mode.
 */
bool veebha_hw_lps_is_sleeping(void);

/**
 * Periodic poll for low-power state transitions.
 */
void veebha_hw_lps_poll(void);


/* ============================================================================
 * 5. BLUETOOTH HARDWARE MANAGEMENT API
 * ============================================================================ */

/**
 * Power ON the Bluetooth radio hardware and initialize HCI/RF.
 */
bool veebha_hw_bt_power_on(void);

/**
 * Power OFF the Bluetooth radio hardware.
 */
void veebha_hw_bt_power_off(void);

/**
 * Check if Bluetooth radio is currently powered ON.
 */
bool veebha_hw_bt_is_powered(void);

/**
 * Set the broadcast device name on the Bluetooth hardware.
 */
bool veebha_hw_bt_set_name(const char *name);

/**
 * Get the current local Bluetooth device name.
 */
const char * veebha_hw_bt_get_name(void);

/**
 * Get the local Bluetooth hardware MAC address (format "AA:BB:CC:DD:EE:FF").
 */
bool veebha_hw_bt_get_mac(char *out_mac_str);

/**
 * Start Bluetooth inquiry scan for nearby devices.
 */
bool veebha_hw_bt_start_scan(uint8_t duration_sec);

/**
 * Cancel/stop ongoing Bluetooth inquiry scan.
 */
bool veebha_hw_bt_stop_scan(void);

/**
 * Check if Bluetooth scan is in progress.
 */
bool veebha_hw_bt_is_scanning(void);

/**
 * Get number of discovered remote Bluetooth devices.
 */
uint8_t veebha_hw_bt_get_discovered_count(void);

/**
 * Get details for a discovered device by index.
 */
bool veebha_hw_bt_get_discovered_device(uint8_t idx, char *name, size_t name_sz,
                                        char *mac, size_t mac_sz,
                                        int8_t *rssi, uint32_t *cod);

/**
 * Connect to a remote Bluetooth device by MAC address.
 */
bool veebha_hw_bt_connect(const char *mac);

/**
 * Disconnect active Bluetooth connection.
 */
bool veebha_hw_bt_disconnect(void);

/**
 * Check if an active Bluetooth ACL connection exists.
 */
bool veebha_hw_bt_is_connected(void);

/**
 * Periodic polling hook to service Bluetooth events & L2CAP/BNEP packets.
 */
void veebha_hw_bt_poll(void);

/**
 * Get number of paired Bluetooth devices.
 */
uint8_t veebha_hw_bt_get_paired_count(void);

/**
 * Get details for a paired device by index.
 */
bool veebha_hw_bt_get_paired_device(uint8_t idx, char *name, size_t name_sz,
                                    char *mac, size_t mac_sz);

/**
 * Check if there is an incoming Bluetooth connection request awaiting user approval.
 */
bool veebha_hw_bt_has_pending_conn_req(void);

/**
 * Get remote device name and MAC for the pending incoming connection request.
 */
bool veebha_hw_bt_get_pending_conn_req(char *name, size_t name_sz,
                                       char *mac, size_t mac_sz);

/**
 * Accept or reject the pending incoming Bluetooth connection request.
 */
void veebha_hw_bt_accept_pending_conn(bool accept);


/* ============================================================================
 * 6. USB CDC DEBUG CONSOLE & LOGGING API
 * ============================================================================ */

/**
 * Initialize USB CDC-ACM controller hardware.
 */
void veebha_hw_usb_console_init(void);

/**
 * Enable or disable USB CDC debug console logger.
 */
bool veebha_hw_usb_console_set_enabled(bool enabled);

/**
 * Check if USB CDC console logger is currently enabled.
 */
bool veebha_hw_usb_console_is_enabled(void);

/**
 * Periodic USB controller poll & TX ring buffer flush.
 */
void veebha_hw_usb_console_poll(void);

/**
 * Write log message directly to USB CDC output.
 */
uint32_t veebha_hw_usb_console_write(const char *buf, uint32_t len);


/* ============================================================================
 * 7. SD / MMC STORAGE CARD API
 * ============================================================================ */

/**
 * Initialize hardware SDMMC controller and detect card.
 */
bool veebha_hw_sdcard_init(void);

/**
 * Check if SD card is present and initialized.
 */
bool veebha_hw_sdcard_present(void);

/**
 * Get SD card capacity in Megabytes.
 */
uint32_t veebha_hw_sdcard_get_capacity_mb(void);

/**
 * Read 512-byte blocks from SD card.
 */
bool veebha_hw_sdcard_read_blocks(uint32_t block_addr, uint8_t *dest, uint32_t count);

/**
 * Write 512-byte blocks to SD card.
 */
bool veebha_hw_sdcard_write_blocks(uint32_t block_addr, const uint8_t *src, uint32_t count);


/* ============================================================================
 * 8. BATTERY & CHARGING POWER MANAGEMENT API
 * ============================================================================ */

/**
 * Get current battery level in percentage (0 to 100%).
 */
uint8_t veebha_hw_battery_get_percent(void);

/**
 * Get current battery voltage in millivolts (e.g. 3400..4200 mV).
 */
uint16_t veebha_hw_battery_get_voltage_mv(void);

/**
 * Check if the device is currently connected to AC / USB charger power.
 */
bool veebha_hw_battery_is_charging(void);


#ifdef __cplusplus
}
#endif

#endif /* SDK_INCLUDE_VEEBHA_HARDWARE_H */

/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Mock Hardware Abstraction Layer Implementation (Host Simulator & Unit Tests)
 */

#include "veebha_hardware.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static bool    s_mock_torch = false;
static uint8_t s_mock_backlight = 100;
static uint32_t s_mock_lps_timeout = 10;
static bool    s_mock_lps_sleeping = false;
static time_t  s_mock_last_activity = 0;

static bool    s_mock_bt_powered = false;
static char    s_mock_bt_name[32] = "VeebhaOS";
static char    s_mock_bt_mac[18] = "00:1A:7D:DA:71:13";
static bool    s_mock_bt_scanning = false;
static bool    s_mock_bt_connected = false;

/* ============================================================================
 * Torch Mock
 * ============================================================================ */

void veebha_hw_torch_set(bool on)
{
    s_mock_torch = on;
}

bool veebha_hw_torch_get(void)
{
    return s_mock_torch;
}

void veebha_hw_torch_toggle(void)
{
    s_mock_torch = !s_mock_torch;
}

/* ============================================================================
 * Backlight Mock
 * ============================================================================ */

void veebha_hw_backlight_set(uint8_t level_pct)
{
    s_mock_backlight = (level_pct > 100) ? 100 : level_pct;
}

uint8_t veebha_hw_backlight_get(void)
{
    return s_mock_backlight;
}

/* ============================================================================
 * RTC Mock
 * ============================================================================ */

static uint8_t  s_mock_hour = 12;
static uint8_t  s_mock_min = 0;
static uint8_t  s_mock_sec = 0;
static uint16_t s_mock_year = 2026;
static uint8_t  s_mock_month = 1;
static uint8_t  s_mock_day = 1;

bool veebha_hw_rtc_get_time(uint8_t *hour, uint8_t *min, uint8_t *sec)
{
    if (hour) *hour = s_mock_hour;
    if (min)  *min  = s_mock_min;
    if (sec)  *sec  = s_mock_sec;
    return true;
}

bool veebha_hw_rtc_set_time(uint8_t hour, uint8_t min, uint8_t sec)
{
    s_mock_hour = hour;
    s_mock_min  = min;
    s_mock_sec  = sec;
    return true;
}

bool veebha_hw_rtc_get_date(uint16_t *year, uint8_t *month, uint8_t *day)
{
    if (year)  *year  = s_mock_year;
    if (month) *month = s_mock_month;
    if (day)   *day   = s_mock_day;
    return true;
}

bool veebha_hw_rtc_set_date(uint16_t year, uint8_t month, uint8_t day)
{
    s_mock_year  = year;
    s_mock_month = month;
    s_mock_day   = day;
    return true;
}

/* ============================================================================
 * LPS Mock
 * ============================================================================ */

void veebha_hw_lps_init(uint32_t auto_sleep_sec)
{
    s_mock_lps_timeout = auto_sleep_sec;
    s_mock_lps_sleeping = false;
    s_mock_last_activity = time(NULL);
}

void veebha_hw_lps_reset_activity(void)
{
    s_mock_last_activity = time(NULL);
    if (s_mock_lps_sleeping) {
        s_mock_lps_sleeping = false;
    }
}

void veebha_hw_lps_set_timeout(uint32_t timeout_sec)
{
    s_mock_lps_timeout = timeout_sec;
}

uint32_t veebha_hw_lps_get_timeout(void)
{
    return s_mock_lps_timeout;
}

bool veebha_hw_lps_is_expired(void)
{
    if (s_mock_lps_timeout == 0) return false;
    time_t now = time(NULL);
    return (uint32_t)(now - s_mock_last_activity) >= s_mock_lps_timeout;
}

void veebha_hw_lps_sleep_display(void)
{
    s_mock_lps_sleeping = true;
}

void veebha_hw_lps_wake_display(void)
{
    s_mock_lps_sleeping = false;
}

bool veebha_hw_lps_is_sleeping(void)
{
    return s_mock_lps_sleeping;
}

void veebha_hw_lps_poll(void)
{
    if (!s_mock_lps_sleeping && veebha_hw_lps_is_expired()) {
        s_mock_lps_sleeping = true;
    }
}

/* ============================================================================
 * Bluetooth Mock
 * ============================================================================ */

bool veebha_hw_bt_power_on(void)
{
    s_mock_bt_powered = true;
    return true;
}

void veebha_hw_bt_power_off(void)
{
    s_mock_bt_powered = false;
    s_mock_bt_scanning = false;
    s_mock_bt_connected = false;
}

bool veebha_hw_bt_is_powered(void)
{
    return s_mock_bt_powered;
}

bool veebha_hw_bt_set_name(const char *name)
{
    if (!name || name[0] == '\0') return false;
    strncpy(s_mock_bt_name, name, sizeof(s_mock_bt_name) - 1);
    s_mock_bt_name[sizeof(s_mock_bt_name) - 1] = '\0';
    return true;
}

const char * veebha_hw_bt_get_name(void)
{
    return s_mock_bt_name;
}

bool veebha_hw_bt_get_mac(char *out_mac_str)
{
    if (!out_mac_str) return false;
    strncpy(out_mac_str, s_mock_bt_mac, 17);
    out_mac_str[17] = '\0';
    return true;
}

bool veebha_hw_bt_start_scan(uint8_t duration_sec)
{
    (void)duration_sec;
    s_mock_bt_scanning = true;
    return true;
}

bool veebha_hw_bt_stop_scan(void)
{
    s_mock_bt_scanning = false;
    return true;
}

bool veebha_hw_bt_is_scanning(void)
{
    return s_mock_bt_scanning;
}

static const struct {
    char name[32];
    char mac[18];
    int8_t rssi;
    uint32_t cod;
} s_mock_discovered[] = {
    { "Sony WH-1000XM4", "1C:52:1D:01:23:45", -64, 0x040400 },
    { "Sachin's Laptop", "00:E0:4C:68:02:11", -72, 0x010100 },
    { "Pixel 8",         "98:CD:AC:88:99:AA", -55, 0x020100 }
};

uint8_t veebha_hw_bt_get_discovered_count(void)
{
    return s_mock_bt_powered ? 3 : 0;
}

bool veebha_hw_bt_get_discovered_device(uint8_t idx, char *name, size_t name_sz,
                                        char *mac, size_t mac_sz,
                                        int8_t *rssi, uint32_t *cod)
{
    if (!s_mock_bt_powered || idx >= 3) return false;
    if (name && name_sz > 0) {
        strncpy(name, s_mock_discovered[idx].name, name_sz - 1);
        name[name_sz - 1] = '\0';
    }
    if (mac && mac_sz >= 18) {
        strncpy(mac, s_mock_discovered[idx].mac, mac_sz - 1);
        mac[mac_sz - 1] = '\0';
    }
    if (rssi) *rssi = s_mock_discovered[idx].rssi;
    if (cod)  *cod  = s_mock_discovered[idx].cod;
    return true;
}

bool veebha_hw_bt_connect(const char *mac)
{
    (void)mac;
    s_mock_bt_connected = true;
    return true;
}

bool veebha_hw_bt_disconnect(void)
{
    s_mock_bt_connected = false;
    return true;
}

bool veebha_hw_bt_is_connected(void)
{
    return s_mock_bt_connected;
}

void veebha_hw_bt_poll(void)
{
}

uint8_t veebha_hw_bt_get_paired_count(void)
{
    return s_mock_bt_powered ? 1 : 0;
}

bool veebha_hw_bt_get_paired_device(uint8_t idx, char *name, size_t name_sz,
                                    char *mac, size_t mac_sz)
{
    if (!s_mock_bt_powered || idx >= 1) return false;
    if (name && name_sz > 0) {
        strncpy(name, "Sachin's Laptop", name_sz - 1);
        name[name_sz - 1] = '\0';
    }
    if (mac && mac_sz >= 18) {
        strncpy(mac, "00:E0:4C:68:02:11", mac_sz - 1);
        mac[mac_sz - 1] = '\0';
    }
    return true;
}

static bool s_mock_pending_conn_req = false;
static char s_mock_pending_name[32] = "Sachin's Laptop";
static char s_mock_pending_mac[18]  = "00:E0:4C:68:02:11";

bool veebha_hw_bt_has_pending_conn_req(void)
{
    return s_mock_pending_conn_req;
}

bool veebha_hw_bt_get_pending_conn_req(char *name, size_t name_sz,
                                       char *mac, size_t mac_sz)
{
    if (!s_mock_pending_conn_req) return false;
    if (name && name_sz > 0) {
        strncpy(name, s_mock_pending_name, name_sz - 1);
        name[name_sz - 1] = '\0';
    }
    if (mac && mac_sz >= 18) {
        strncpy(mac, s_mock_pending_mac, mac_sz - 1);
        mac[mac_sz - 1] = '\0';
    }
    return true;
}

void veebha_hw_bt_accept_pending_conn(bool accept)
{
    s_mock_pending_conn_req = false;
    if (accept) {
        s_mock_bt_connected = true;
    }
}


/* ============================================================================
 * USB Console Mock
 * ============================================================================ */
static bool s_mock_usb_console_enabled = false;

void veebha_hw_usb_console_init(void)
{
    s_mock_usb_console_enabled = false;
}

bool veebha_hw_usb_console_set_enabled(bool enabled)
{
    s_mock_usb_console_enabled = enabled;
    return true;
}

bool veebha_hw_usb_console_is_enabled(void)
{
    return s_mock_usb_console_enabled;
}

void veebha_hw_usb_console_poll(void)
{
}

uint32_t veebha_hw_usb_console_write(const char *buf, uint32_t len)
{
    (void)buf;
    return len;
}


/* ============================================================================
 * SD Card Mock
 * ============================================================================ */
static bool s_mock_sd_present = true;

bool veebha_hw_sdcard_init(void)
{
    return s_mock_sd_present;
}

bool veebha_hw_sdcard_present(void)
{
    return s_mock_sd_present;
}

uint32_t veebha_hw_sdcard_get_capacity_mb(void)
{
    return 16384; // 16 GB Mock Card
}

bool veebha_hw_sdcard_read_blocks(uint32_t block_addr, uint8_t *dest, uint32_t count)
{
    (void)block_addr;
    if (dest && count > 0) {
        memset(dest, 0, count * 512);
    }
    return true;
}

bool veebha_hw_sdcard_write_blocks(uint32_t block_addr, const uint8_t *src, uint32_t count)
{
    (void)block_addr;
    (void)src;
    (void)count;
    return true;
}


/* ============================================================================
 * Battery Mock
 * ============================================================================ */
static uint8_t  s_mock_bat_percent = 85;
static uint16_t s_mock_bat_voltage = 3980;
static bool     s_mock_bat_charging = false;

uint8_t veebha_hw_battery_get_percent(void)
{
    return s_mock_bat_percent;
}

uint16_t veebha_hw_battery_get_voltage_mv(void)
{
    return s_mock_bat_voltage;
}

bool veebha_hw_battery_is_charging(void)
{
    return s_mock_bat_charging;
}


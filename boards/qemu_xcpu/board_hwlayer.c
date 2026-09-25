/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Board Hardware Layer (HW Layer) for RDA8809 / QEMU
 *
 * Implements the generic veebha_hardware.h HAL API by routing calls
 * directly to dedicated RDA8809 bare-metal drivers (hal_torch, hal_backlight,
 * hal_calendar, hal_lps, hal_bt, hal_gouda, lcd_ili9225g).
 */

#include "veebha_hardware.h"
#include "hal_torch.h"
#include "hal_backlight.h"
#include "hal_calendar.h"
#include "hal_lps.h"
#include "hal_bt.h"
#include "bnep.h"
#include "ethernetif_bnep.h"
#include "net_app.h"
#include "lcd_panel.h"
#include "lcd_ili9225g.h"
#include "hal_gouda.h"
#include "timer.h"
#include "veebha_log.h"
#include "third_party/lvgl/lvgl.h"
#include "lwip/timeouts.h"
#include <stdio.h>
#include <string.h>

#define TAG "BOARD_HW"

extern void ethernetif_bnep_link_changed(BOOL up);

static uint8_t s_saved_backlight_level = 100;
static bool    s_display_is_sleeping = false;

/* ============================================================================
 * 1. TORCH / FLASHLIGHT API
 * ============================================================================ */

void veebha_hw_torch_set(bool on)
{
    hal_TorchSet(on ? TRUE : FALSE);
    OS_LOGI(TAG, "Hardware Torch set to %s", on ? "ON" : "OFF");
}

bool veebha_hw_torch_get(void)
{
    return hal_TorchIsOn() ? true : false;
}

void veebha_hw_torch_toggle(void)
{
    hal_TorchToggle();
    OS_LOGI(TAG, "Hardware Torch toggled -> %s", hal_TorchIsOn() ? "ON" : "OFF");
}


/* ============================================================================
 * 2. DISPLAY BACKLIGHT & BRIGHTNESS API
 * ============================================================================ */

void veebha_hw_backlight_set(uint8_t level_pct)
{
    if (level_pct > 100) level_pct = 100;
    if (level_pct > 0) {
        s_saved_backlight_level = level_pct;
    }
    hal_BacklightSetLevel((UINT8)level_pct);
    OS_LOGI(TAG, "Hardware Backlight brightness set to %u%%", level_pct);
}

uint8_t veebha_hw_backlight_get(void)
{
    return (uint8_t)hal_BacklightGetLevel();
}


/* ============================================================================
 * 3. HARDWARE REAL-TIME CLOCK (RTC) & CALENDAR API
 * ============================================================================ */

bool veebha_hw_rtc_get_time(uint8_t *hour, uint8_t *min, uint8_t *sec)
{
    hal_calendar_time_t tm;
    if (hal_CalendarGetTime(&tm)) {
        if (hour) *hour = tm.hour;
        if (min)  *min  = tm.min;
        if (sec)  *sec  = tm.sec;
        return true;
    }
    return false;
}

bool veebha_hw_rtc_set_time(uint8_t hour, uint8_t min, uint8_t sec)
{
    hal_calendar_time_t tm;
    if (!hal_CalendarGetTime(&tm)) {
        tm.year = 2026;
        tm.month = 1;
        tm.day = 1;
        tm.wDay = 4;
    }
    tm.hour = hour;
    tm.min  = min;
    tm.sec  = sec;
    return hal_CalendarSetTime(&tm) ? true : false;
}

bool veebha_hw_rtc_get_date(uint16_t *year, uint8_t *month, uint8_t *day)
{
    hal_calendar_time_t tm;
    if (hal_CalendarGetTime(&tm)) {
        if (year)  *year  = tm.year;
        if (month) *month = tm.month;
        if (day)   *day   = tm.day;
        return true;
    }
    return false;
}

bool veebha_hw_rtc_set_date(uint16_t year, uint8_t month, uint8_t day)
{
    hal_calendar_time_t tm;
    if (!hal_CalendarGetTime(&tm)) {
        tm.hour = 12;
        tm.min = 0;
        tm.sec = 0;
        tm.wDay = 0;
    }
    tm.year  = year;
    tm.month = month;
    tm.day   = day;
    int y = year;
    int m = month;
    int d = day;
    if (m < 3) { m += 12; y -= 1; }
    int w = (d + (13 * (m + 1)) / 5 + y + y / 4 - y / 100 + y / 400 + 6) % 7;
    tm.wDay = (uint8_t)w;
    return hal_CalendarSetTime(&tm) ? true : false;
}


/* ============================================================================
 * 4. LOW POWER SCHEME (LPS) & DISPLAY SLEEP API
 * ============================================================================ */

typedef enum {
    LPS_STATE_ACTIVE = 0,
    LPS_STATE_DIMMED,
    LPS_STATE_SLEEPING
} lps_display_state_t;

static lps_display_state_t s_display_state = LPS_STATE_ACTIVE;
static uint32_t s_auto_sleep_sec = 10;
static uint32_t s_last_activity_ms = 0;

void veebha_hw_lps_init(uint32_t auto_sleep_sec)
{
    hal_LpsInit();
    s_auto_sleep_sec = auto_sleep_sec;
    s_last_activity_ms = timer_get_ms();
    s_display_state = LPS_STATE_ACTIVE;
    OS_LOGI(TAG, "LPS Initialized with %u-second auto-sleep timeout", (unsigned int)auto_sleep_sec);
}

void veebha_hw_lps_reset_activity(void)
{
    s_last_activity_ms = timer_get_ms();
    hal_LpsResetActivityTimer();
    if (s_display_state == LPS_STATE_SLEEPING) {
        veebha_hw_lps_wake_display();
    } else if (s_display_state == LPS_STATE_DIMMED) {
        s_display_state = LPS_STATE_ACTIVE;
        hal_BacklightSetLevel(s_saved_backlight_level ? s_saved_backlight_level : 100);
    }
}

void veebha_hw_lps_set_timeout(uint32_t timeout_sec)
{
    s_auto_sleep_sec = timeout_sec;
    hal_LpsSetAutoSleepTimeout(timeout_sec);
}

uint32_t veebha_hw_lps_get_timeout(void)
{
    return s_auto_sleep_sec;
}

bool veebha_hw_lps_is_expired(void)
{
    if (s_auto_sleep_sec == 0) return false;
    return (timer_get_ms() - s_last_activity_ms) >= (s_auto_sleep_sec * 1000);
}

static volatile bool s_display_in_transition = false;
static volatile bool s_display_need_repaint = false;

void veebha_hw_lps_sleep_display(void)
{
    if (s_display_state == LPS_STATE_SLEEPING || s_display_in_transition) return;

    s_display_in_transition = true;
    OS_LOGI(TAG, "Entering Display Sleep mode...");
    s_display_state = LPS_STATE_SLEEPING;

    /* 1. Turn OFF Backlight */
    hal_BacklightSetLevel(0);

    /* 2. Wait for GOUDA controller idle before sending panel sleep */
    hal_GoudaWaitIdle(100);

    /* 3. Put ILI9225G panel into sleep */
    const lcd_panel_t *panel = lcd_ili9225g_get_panel();
    if (panel && panel->sleep) {
        panel->sleep();
    }
    s_display_in_transition = false;
}

void veebha_hw_lps_wake_display(void)
{
    if (s_display_state == LPS_STATE_ACTIVE || s_display_in_transition) return;

    s_display_in_transition = true;
    OS_LOGI(TAG, "Waking Display from sleep...");
    s_display_state = LPS_STATE_ACTIVE;
    s_last_activity_ms = timer_get_ms();

    /* 1. Wait for GOUDA controller idle before waking panel */
    hal_GoudaWaitIdle(100);

    /* 2. Wake ILI9225G panel */
    const lcd_panel_t *panel = lcd_ili9225g_get_panel();
    if (panel && panel->wakeup) {
        panel->wakeup();
    }

    /* 3. Restore saved Backlight level */
    hal_BacklightSetLevel(s_saved_backlight_level ? s_saved_backlight_level : 100);

    /* 4. Request LVGL repaint safely on UI thread */
    s_display_need_repaint = true;
    s_display_in_transition = false;
}

bool veebha_hw_lps_is_sleeping(void)
{
    return (s_display_state == LPS_STATE_SLEEPING || s_display_state == LPS_STATE_DIMMED);
}

void veebha_hw_lps_poll(void)
{
    if (s_display_need_repaint) {
        s_display_need_repaint = false;
        lv_obj_invalidate(lv_scr_act());
    }

    if (s_auto_sleep_sec == 0) return;
    uint32_t now = timer_get_ms();
    uint32_t elapsed_ms = now - s_last_activity_ms;
    uint32_t dim_timeout_ms = s_auto_sleep_sec * 1000;
    uint32_t sleep_timeout_ms = dim_timeout_ms + 5000; // 5 seconds after dimming

    if (elapsed_ms >= sleep_timeout_ms) {
        if (s_display_state != LPS_STATE_SLEEPING) {
            veebha_hw_lps_sleep_display();
        }
    } else if (elapsed_ms >= dim_timeout_ms) {
        if (s_display_state == LPS_STATE_ACTIVE) {
            s_display_state = LPS_STATE_DIMMED;
            uint8_t dim_level = s_saved_backlight_level / 4;
            if (dim_level < 15) dim_level = 15;
            hal_BacklightSetLevel(dim_level);
            OS_LOGI(TAG, "LPS auto-dimming display (inactivity %u ms)", (unsigned int)elapsed_ms);
        }
    }
}


/* ============================================================================
 * 5. BLUETOOTH HARDWARE MANAGEMENT API
 * ============================================================================ */

extern hal_bt_status_t g_bt_status;

bool veebha_hw_bt_power_on(void)
{
    if (hal_BtIsPowered()) return true;

    OS_LOGI(TAG, "Powering ON RDA8809 Bluetooth hardware...");
    hal_BtInit();
    if (!hal_BtPowerOn()) {
        OS_LOGE(TAG, "Failed to power on BT PMU rail");
        return false;
    }

    if (!hal_BtProbe(&g_bt_status)) {
        OS_LOGE(TAG, "Failed to probe BT core/RF over I2C");
        return false;
    }

    hal_BtInitRf();
    g_bt_status.hci_ready = hal_BtTestHci(g_bt_status.bd_addr);
    bnep_init();
    net_stack_init();
    hal_BtMakeDiscoverable(TRUE);
    OS_LOGI(TAG, "RDA8809 Bluetooth Ready! MAC: %02X:%02X:%02X:%02X:%02X:%02X",
            g_bt_status.bd_addr[5], g_bt_status.bd_addr[4], g_bt_status.bd_addr[3],
            g_bt_status.bd_addr[2], g_bt_status.bd_addr[1], g_bt_status.bd_addr[0]);
    return true;
}

void veebha_hw_bt_power_off(void)
{
    if (hal_BtIsPowered()) {
        OS_LOGI(TAG, "Powering OFF RDA8809 Bluetooth hardware...");
        bnep_disconnect();
        ethernetif_bnep_link_changed(FALSE);
        hal_BtPowerOff();
    }
}

bool veebha_hw_bt_is_powered(void)
{
    return hal_BtIsPowered() ? true : false;
}

bool veebha_hw_bt_set_name(const char *name)
{
    return hal_BtSetLocalName(name) ? true : false;
}

const char * veebha_hw_bt_get_name(void)
{
    return hal_BtGetLocalName();
}

bool veebha_hw_bt_get_mac(char *out_mac_str)
{
    if (!out_mac_str || !hal_BtIsPowered()) return false;
    snprintf(out_mac_str, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
             g_bt_status.bd_addr[5], g_bt_status.bd_addr[4], g_bt_status.bd_addr[3],
             g_bt_status.bd_addr[2], g_bt_status.bd_addr[1], g_bt_status.bd_addr[0]);
    return true;
}

bool veebha_hw_bt_start_scan(uint8_t duration_sec)
{
    if (!hal_BtIsPowered()) return false;
    hal_BtClearDiscoveredDevices();
    return hal_BtStartInquiry(duration_sec ? duration_sec : 10) ? true : false;
}

bool veebha_hw_bt_stop_scan(void)
{
    if (!hal_BtIsPowered()) return false;
    return hal_BtCancelInquiry() ? true : false;
}

bool veebha_hw_bt_is_scanning(void)
{
    return (hal_BtIsPowered() && hal_BtIsScanning());
}

uint8_t veebha_hw_bt_get_discovered_count(void)
{
    if (!hal_BtIsPowered()) return 0;
    return hal_BtGetDiscoveredCount();
}

bool veebha_hw_bt_get_discovered_device(uint8_t idx, char *name, size_t name_sz,
                                        char *mac, size_t mac_sz,
                                        int8_t *rssi, uint32_t *cod)
{
    if (!hal_BtIsPowered()) return false;
    const bt_remote_dev_t *d = hal_BtGetDiscoveredDevice(idx);
    if (!d) return false;

    if (name && name_sz > 0) {
        if (d->name[0] != '\0') {
            strncpy(name, d->name, name_sz - 1);
            name[name_sz - 1] = '\0';
        } else {
            snprintf(name, name_sz, "Dev %02X:%02X:%02X",
                     d->bd_addr[2], d->bd_addr[1], d->bd_addr[0]);
        }
    }

    if (mac && mac_sz >= 18) {
        snprintf(mac, mac_sz, "%02X:%02X:%02X:%02X:%02X:%02X",
                 d->bd_addr[5], d->bd_addr[4], d->bd_addr[3],
                 d->bd_addr[2], d->bd_addr[1], d->bd_addr[0]);
    }

    if (rssi) *rssi = d->rssi;
    if (cod)  *cod  = d->cod;

    return true;
}

static uint8_t hex_char_to_val(char c)
{
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
    return 0;
}

static bool parse_mac_string(const char *str, uint8_t *bd)
{
    if (!str || !bd) return false;
    const char *p = str;
    for (int i = 5; i >= 0; i--) {
        while (*p == ' ' || *p == ':') p++;
        if (!*p) return false;
        uint8_t high = hex_char_to_val(*p++);
        if (!*p) return false;
        uint8_t low = hex_char_to_val(*p++);
        bd[i] = (high << 4) | low;
    }
    return true;
}

bool veebha_hw_bt_connect(const char *mac)
{
    if (!mac || !hal_BtIsPowered()) return false;
    uint8_t bd[6];
    if (!parse_mac_string(mac, bd)) return false;

    if (g_bt_status.connected && memcmp(g_bt_status.remote_bd_addr, bd, 6) == 0) {
        hal_BtSetEncryption(g_bt_status.conn_handle, 1);
        bnep_connect(g_bt_status.conn_handle, g_bt_status.remote_bd_addr);
        return true;
    }

    return hal_BtConnect(bd) ? true : false;
}

bool veebha_hw_bt_disconnect(void)
{
    if (g_bt_status.connected) {
        bnep_disconnect();
        return hal_BtDisconnect(g_bt_status.conn_handle) ? true : false;
    }
    return false;
}

bool veebha_hw_bt_is_connected(void)
{
    return g_bt_status.connected ? true : false;
}

void veebha_hw_bt_poll(void)
{
    if (hal_BtIsPowered()) {
        hal_BtPollEvents();
        bnep_poll();
        net_app_poll();
        sys_check_timeouts();
    }
}

uint8_t veebha_hw_bt_get_paired_count(void)
{
    if (!hal_BtIsPowered()) return 0;
    return hal_BtGetPairedCount();
}

bool veebha_hw_bt_get_paired_device(uint8_t idx, char *name, size_t name_sz,
                                    char *mac, size_t mac_sz)
{
    if (!hal_BtIsPowered()) return false;
    const hal_bt_paired_dev_t *p = hal_BtGetPairedDevice(idx);
    if (!p) return false;

    if (name && name_sz > 0) {
        if (p->name[0] != '\0') {
            strncpy(name, p->name, name_sz - 1);
            name[name_sz - 1] = '\0';
        } else {
            snprintf(name, name_sz, "Dev %02X:%02X:%02X",
                     p->bd_addr[2], p->bd_addr[1], p->bd_addr[0]);
        }
    }
    if (mac && mac_sz >= 18) {
        snprintf(mac, mac_sz, "%02X:%02X:%02X:%02X:%02X:%02X",
                 p->bd_addr[5], p->bd_addr[4], p->bd_addr[3],
                 p->bd_addr[2], p->bd_addr[1], p->bd_addr[0]);
    }
    return true;
}

bool veebha_hw_bt_has_pending_conn_req(void)
{
    if (!hal_BtIsPowered()) return false;
    return hal_BtHasPendingConnection() ? true : false;
}

bool veebha_hw_bt_get_pending_conn_req(char *name, size_t name_sz,
                                       char *mac, size_t mac_sz)
{
    if (!hal_BtIsPowered()) return false;
    return hal_BtGetPendingConnectionInfo(name, (UINT16)name_sz, mac, (UINT16)mac_sz) ? true : false;
}

void veebha_hw_bt_accept_pending_conn(bool accept)
{
    if (hal_BtIsPowered()) {
        hal_BtAcceptPendingConnection(accept ? TRUE : FALSE);
    }
}


/* ============================================================================
 * 6. USB CDC DEBUG CONSOLE & LOGGING API
 * ============================================================================ */

#include "usb_cdc.h"
#include "hal_usb.h"
#include "veebha_log.h"

static bool s_usb_console_enabled = false;

static void usb_cdc_log_sink(os_log_level_t level, const char *tag, const char *msg)
{
    (void)level;
    char line[256];
    int n = snprintf(line, sizeof(line), "[%s] %s\r\n", tag ? tag : "OS", msg ? msg : "");
    if (n > 0) {
        usb_cdc_log_write(line, (UINT32)n);
    }
}

void veebha_hw_usb_console_init(void)
{
    usb_cdc_init();
    s_usb_console_enabled = false;
}

bool veebha_hw_usb_console_set_enabled(bool enabled)
{
    s_usb_console_enabled = enabled;
    if (enabled) {
        usb_cdc_init();
        hal_UsbOpen();
        os_log_set_sink(usb_cdc_log_sink);
        OS_LOGI(TAG, "USB CDC Debug Console enabled! Connect PC terminal to CDC ACM port.");
    } else {
        os_log_set_sink(NULL);
        hal_UsbClose();
        OS_LOGI(TAG, "USB CDC Debug Console disabled");
    }
    return true;
}

bool veebha_hw_usb_console_is_enabled(void)
{
    return s_usb_console_enabled;
}

void veebha_hw_usb_console_poll(void)
{
    if (s_usb_console_enabled) {
        usb_cdc_poll();
    }
}

uint32_t veebha_hw_usb_console_write(const char *buf, uint32_t len)
{
    if (!buf || len == 0 || !s_usb_console_enabled) return 0;
    return usb_cdc_log_write(buf, len);
}


/* ============================================================================
 * 7. SD / MMC STORAGE CARD API
 * ============================================================================ */

#include "hal_sdmmc.h"
#include "mcd_sdmmc.h"

static bool s_sdcard_ready = false;

bool veebha_hw_sdcard_init(void)
{
    hal_SdmmcOpen(0);
    MCD_ERR_T err = mcd_Open();
    if (err == MCD_ERR_NO) {
        s_sdcard_ready = true;
        mcd_card_info_t card_info;
        if (mcd_GetCardInfo(&card_info)) {
            OS_LOGI(TAG, "SD Card detected: Size=%u MB, Sectors=%u",
                    (unsigned int)card_info.capacityMB, (unsigned int)card_info.sectorCount);
        }
        return true;
    } else {
        s_sdcard_ready = false;
        OS_LOGW(TAG, "SD Card open failed (err %d)", (int)err);
        return false;
    }
}

bool veebha_hw_sdcard_present(void)
{
    return s_sdcard_ready;
}

uint32_t veebha_hw_sdcard_get_capacity_mb(void)
{
    if (!s_sdcard_ready) return 0;
    mcd_card_info_t info;
    if (mcd_GetCardInfo(&info)) {
        return info.capacityMB;
    }
    return 0;
}

bool veebha_hw_sdcard_read_blocks(uint32_t block_addr, uint8_t *dest, uint32_t count)
{
    if (!s_sdcard_ready || !dest || count == 0) return false;
    MCD_ERR_T err = mcd_Read(block_addr, dest, count * 512);
    return (err == MCD_ERR_NO);
}

bool veebha_hw_sdcard_write_blocks(uint32_t block_addr, const uint8_t *src, uint32_t count)
{
    if (!s_sdcard_ready || !src || count == 0) return false;
    MCD_ERR_T err = mcd_Write(block_addr, (UINT8*)src, count * 512);
    return (err == MCD_ERR_NO);
}


/* ============================================================================
 * 8. BATTERY & CHARGING POWER MANAGEMENT API
 * ============================================================================ */

#include "hal_pmd.h"
#include "pwm.h"

bool veebha_hw_battery_is_charging(void)
{
    /* Read PMU Register 0x14 (Charger Status)
     * Bit 7 (RDA_PMU_CHR_AC_ON) is high when USB VBUS / AC power is connected.
     */
    UINT16 status = hal_PmdReadReg(PMU_REG_CHARGER_STATUS);
    return (status & (1 << 7)) != 0;
}

uint16_t veebha_hw_battery_get_voltage_mv(void)
{
    /* Read GPADC Channel 7 (Battery VBAT) from PWM controller register */
    UINT32 reg = hwp_pwm->GPADC_DATA;
    if (reg & PWM_GPADC_VALUE_VALID) {
        UINT16 raw = (UINT16)(reg & 0x3FF);
        if (raw > 0) {
            uint32_t mv = ((uint32_t)raw * 4200) / 1023;
            if (mv >= 3000 && mv <= 4500) {
                return (uint16_t)mv;
            }
        }
    }
    /* Fallback default nominal voltage: 3950 mV (~85%) */
    return 3950;
}

uint8_t veebha_hw_battery_get_percent(void)
{
    uint16_t mv = veebha_hw_battery_get_voltage_mv();
    if (mv <= 3400) return 0;
    if (mv >= 4200) return 100;
    uint32_t pct = ((uint32_t)(mv - 3400) * 100) / (4200 - 3400);
    if (pct > 100) pct = 100;
    return (uint8_t)pct;
}


/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */

#include "veebha_connectivity.h"
#include "drivers/mock/mock_connectivity.h"
#include "veebha_event.h"
#include "veebha_status_bar.h"
#include "veebha_log.h"
#include <stdio.h>
#include <string.h>

#define TAG "CONNECTIVITY"
#define MAX_DEV_POOL 4

static os_bt_state_t s_bt_state = BT_STATE_OFF;
static bool s_bt_visible = true;
static char s_bt_device_name[32] = "VeebhaOS";

static os_usb_mode_t s_usb_mode = USB_MODE_NONE;
static bool s_usb_connected = false;
static bool s_usb_tethering = false;
static bool s_bt_tethering = false;

static os_bt_device_t s_devices[MAX_DEV_POOL] = {
    {
        .name = "Sony WH-1000XM4",
        .mac = "1C:52:1D:01:23:45",
        .type = BT_DEV_AUDIO,
        .rssi = -64,
        .is_paired = false,
        .is_connected = false
    },
    {
        .name = "Sachin's Laptop",
        .mac = "00:E0:4C:68:02:11",
        .type = BT_DEV_COMPUTER,
        .rssi = -72,
        .is_paired = true,
        .is_connected = false
    },
    {
        .name = "Pixel 8",
        .mac = "98:CD:AC:88:99:AA",
        .type = BT_DEV_PHONE,
        .rssi = -55,
        .is_paired = false,
        .is_connected = false
    }
};
static uint16_t s_device_count = 3;

static void update_status_bar_bt_state(void)
{
    bool enabled = (s_bt_state != BT_STATE_OFF);
    bool audio_connected = false;
    for (uint16_t i = 0; i < s_device_count; i++) {
        if (s_devices[i].is_connected && s_devices[i].type == BT_DEV_AUDIO) {
            audio_connected = true;
            break;
        }
    }
    status_bar_set_bt_state(enabled, audio_connected);
}

void connectivity_init(void)
{
    s_bt_state = BT_STATE_OFF;
    s_bt_visible = true;
    strncpy(s_bt_device_name, "VeebhaOS", sizeof(s_bt_device_name) - 1);
    s_usb_mode = USB_MODE_NONE;
    s_usb_connected = false;
    s_usb_tethering = false;
    s_bt_tethering = false;
    update_status_bar_bt_state();
    status_bar_set_usb_mode((uint8_t)s_usb_mode);
    status_bar_set_tethering(false);
    OS_LOGI(TAG, "Mock connectivity subsystem initialized");
}

void mock_connectivity_init(void)
{
    connectivity_init();
}

os_bt_state_t connectivity_bt_get_state(void)
{
    return s_bt_state;
}

void connectivity_bt_set_enabled(bool enabled)
{
    if (enabled) {
        if (s_bt_state == BT_STATE_OFF) {
            s_bt_state = BT_STATE_ON;
            OS_LOGI(TAG, "Bluetooth Radio ON");
        }
    } else {
        s_bt_state = BT_STATE_OFF;
        /* Disconnect all connected devices */
        for (uint16_t i = 0; i < s_device_count; i++) {
            s_devices[i].is_connected = false;
        }
        s_bt_tethering = false;
        OS_LOGI(TAG, "Bluetooth Radio OFF");
    }

    update_status_bar_bt_state();

    os_event_t evt = {
        .type = OS_EVT_BT_STATE_CHANGED,
        .timestamp = 0,
        .payload = {
            .bt = { .bt_state = (uint8_t)s_bt_state }
        }
    };
    os_event_post(&evt);
}

bool connectivity_bt_is_enabled(void)
{
    return (s_bt_state != BT_STATE_OFF);
}

bool connectivity_bt_is_visible(void)
{
    return s_bt_visible;
}

void connectivity_bt_set_visible(bool visible)
{
    s_bt_visible = visible;
    OS_LOGI(TAG, "Bluetooth Visibility set to: %s", visible ? "Shown" : "Hidden");
}

const char * connectivity_bt_get_device_name(void)
{
    return s_bt_device_name;
}

void connectivity_bt_set_device_name(const char *name)
{
    if (!name || name[0] == '\0') return;
    strncpy(s_bt_device_name, name, sizeof(s_bt_device_name) - 1);
    s_bt_device_name[sizeof(s_bt_device_name) - 1] = '\0';
    OS_LOGI(TAG, "Bluetooth Device Name renamed to: '%s'", s_bt_device_name);
}

uint16_t connectivity_bt_get_paired_devices(os_bt_device_t *out_devices, uint16_t max_count)
{
    if (!out_devices || max_count == 0) return 0;
    uint16_t count = 0;
    for (uint16_t i = 0; i < s_device_count && count < max_count; i++) {
        if (s_devices[i].is_paired) {
            out_devices[count++] = s_devices[i];
        }
    }
    return count;
}

uint16_t connectivity_bt_get_discovered_devices(os_bt_device_t *out_devices, uint16_t max_count)
{
    if (!out_devices || max_count == 0) return 0;
    uint16_t count = 0;
    for (uint16_t i = 0; i < s_device_count && count < max_count; i++) {
        if (!s_devices[i].is_paired) {
            out_devices[count++] = s_devices[i];
        }
    }
    return count;
}

void connectivity_bt_start_scan(void)
{
    if (s_bt_state != BT_STATE_OFF) {
        s_bt_state = BT_STATE_SCANNING;
        OS_LOGI(TAG, "Bluetooth Inquiry Scan started");
    }
}

void connectivity_bt_stop_scan(void)
{
    if (s_bt_state == BT_STATE_SCANNING) {
        bool any_connected = false;
        for (uint16_t i = 0; i < s_device_count; i++) {
            if (s_devices[i].is_connected) {
                any_connected = true;
                break;
            }
        }
        s_bt_state = any_connected ? BT_STATE_CONNECTED : BT_STATE_ON;
        OS_LOGI(TAG, "Bluetooth Inquiry Scan stopped");
    }
}

bool connectivity_bt_pair_device(const char *mac)
{
    if (!mac) return false;
    for (uint16_t i = 0; i < s_device_count; i++) {
        if (strcmp(s_devices[i].mac, mac) == 0) {
            s_devices[i].is_paired = true;
            s_devices[i].is_connected = true;
            s_bt_state = BT_STATE_CONNECTED;
            update_status_bar_bt_state();
            OS_LOGI(TAG, "Device '%s' (%s) Paired and Connected", s_devices[i].name, mac);
            return true;
        }
    }
    return false;
}

bool connectivity_bt_unpair_device(const char *mac)
{
    if (!mac) return false;
    for (uint16_t i = 0; i < s_device_count; i++) {
        if (strcmp(s_devices[i].mac, mac) == 0) {
            s_devices[i].is_paired = false;
            s_devices[i].is_connected = false;
            update_status_bar_bt_state();
            OS_LOGI(TAG, "Device '%s' (%s) Unpaired", s_devices[i].name, mac);
            return true;
        }
    }
    return false;
}

bool connectivity_bt_connect_device(const char *mac)
{
    if (!mac) return false;
    for (uint16_t i = 0; i < s_device_count; i++) {
        if (strcmp(s_devices[i].mac, mac) == 0) {
            if (s_devices[i].is_paired) {
                s_devices[i].is_connected = true;
                s_bt_state = BT_STATE_CONNECTED;
                update_status_bar_bt_state();
                OS_LOGI(TAG, "Device '%s' (%s) Connected", s_devices[i].name, mac);
                return true;
            }
        }
    }
    return false;
}

bool connectivity_bt_disconnect_device(const char *mac)
{
    if (!mac) return false;
    for (uint16_t i = 0; i < s_device_count; i++) {
        if (strcmp(s_devices[i].mac, mac) == 0) {
            s_devices[i].is_connected = false;
            bool any_connected = false;
            for (uint16_t j = 0; j < s_device_count; j++) {
                if (s_devices[j].is_connected) {
                    any_connected = true;
                    break;
                }
            }
            s_bt_state = any_connected ? BT_STATE_CONNECTED : BT_STATE_ON;
            update_status_bar_bt_state();
            OS_LOGI(TAG, "Device '%s' (%s) Disconnected", s_devices[i].name, mac);
            return true;
        }
    }
    return false;
}

os_usb_mode_t connectivity_usb_get_mode(void)
{
    return s_usb_mode;
}

void connectivity_usb_set_mode(os_usb_mode_t mode)
{
    s_usb_mode = mode;
    if (mode == USB_MODE_TETHERING) {
        s_usb_tethering = true;
    } else {
        s_usb_tethering = false;
    }
    status_bar_set_usb_mode((uint8_t)s_usb_mode);
    status_bar_set_tethering(s_usb_tethering || s_bt_tethering);
    const char *mode_names[] = {
        "None", "Charge Only", "Mass Storage", "USB Tethering", "Serial Debug"
    };
    OS_LOGI(TAG, "USB Mode set to: %s", mode_names[mode % 5]);
}

bool connectivity_usb_is_connected(void)
{
    return s_usb_connected;
}

void connectivity_usb_set_connected(bool connected)
{
    s_usb_connected = connected;
    if (!connected) {
        s_usb_mode = USB_MODE_NONE;
        s_usb_tethering = false;
    }
    status_bar_set_usb_mode((uint8_t)s_usb_mode);
    status_bar_set_tethering(s_usb_tethering || s_bt_tethering);
    os_event_t evt = {
        .type = connected ? OS_EVT_USB_INSERTED : OS_EVT_USB_REMOVED,
        .timestamp = 0,
        .payload = {
            .usb = { .usb_mode = (uint8_t)s_usb_mode }
        }
    };
    os_event_post(&evt);
    OS_LOGI(TAG, "USB Cable %s", connected ? "Inserted" : "Removed");
}

bool connectivity_tethering_get_usb(void)
{
    return s_usb_tethering;
}

void connectivity_tethering_set_usb(bool enabled)
{
    s_usb_tethering = enabled;
    if (enabled) {
        s_usb_mode = USB_MODE_TETHERING;
    } else if (s_usb_mode == USB_MODE_TETHERING) {
        s_usb_mode = USB_MODE_CHARGE_ONLY;
    }
    status_bar_set_usb_mode((uint8_t)s_usb_mode);
    status_bar_set_tethering(s_usb_tethering || s_bt_tethering);
    OS_LOGI(TAG, "USB Tethering: %s", enabled ? "Enabled" : "Disabled");
}

bool connectivity_tethering_get_bt(void)
{
    return s_bt_tethering;
}

void connectivity_tethering_set_bt(bool enabled)
{
    s_bt_tethering = enabled;
    status_bar_set_tethering(s_usb_tethering || s_bt_tethering);
    OS_LOGI(TAG, "Bluetooth Tethering: %s", enabled ? "Enabled" : "Disabled");
}

/* ============================================================================
 * Bluetooth OBEX / OPP File Transfer Implementation
 * ============================================================================ */
static os_bt_file_xfer_t s_xfer = {
    .filename = "",
    .target_path = "",
    .remote_device = "",
    .total_bytes = 0,
    .bytes_transferred = 0,
    .status = BT_XFER_IDLE
};

bool connectivity_bt_send_file(const char *mac, const char *filepath)
{
    if (!filepath || s_bt_state == BT_STATE_OFF) return false;

    const char *fn = strrchr(filepath, '/');
    fn = fn ? (fn + 1) : filepath;

    strncpy(s_xfer.filename, fn, sizeof(s_xfer.filename) - 1);
    strncpy(s_xfer.target_path, filepath, sizeof(s_xfer.target_path) - 1);
    strncpy(s_xfer.remote_device, mac ? mac : "Unknown", sizeof(s_xfer.remote_device) - 1);
    s_xfer.total_bytes = 16384;
    s_xfer.bytes_transferred = 16384;
    s_xfer.status = BT_XFER_COMPLETE;

    OS_LOGI(TAG, "Bluetooth OBEX Sent '%s' to %s (Status: Complete)", fn, s_xfer.remote_device);
    return true;
}

bool connectivity_bt_receive_file(const char *filename, const uint8_t *data, size_t size)
{
    if (!filename || s_bt_state == BT_STATE_OFF) return false;

    char target[128];
    const char *dot = strrchr(filename, '.');

    if (dot && strcasecmp(dot, ".vapp") == 0) {
        snprintf(target, sizeof(target), "./vapps/%s", filename);
    } else if (dot && (strcasecmp(dot, ".mp3") == 0 || strcasecmp(dot, ".wav") == 0)) {
        snprintf(target, sizeof(target), "./sdcard/Music/%s", filename);
    } else if (dot && (strcasecmp(dot, ".raw") == 0 || strcasecmp(dot, ".bmp") == 0 || strcasecmp(dot, ".png") == 0 || strcasecmp(dot, ".jpg") == 0)) {
        snprintf(target, sizeof(target), "./sdcard/Photos/%s", filename);
    } else {
        snprintf(target, sizeof(target), "./sdcard/Documents/%s", filename);
    }

#if defined(CONFIG_SIMULATOR) && !defined(CONFIG_IS_RAMRUN_ONLY)
    if (data && size > 0) {
        FILE *f = fopen(target, "wb");
        if (f) {
            fwrite(data, 1, size, f);
            fclose(f);
        }
    }
#endif

    strncpy(s_xfer.filename, filename, sizeof(s_xfer.filename) - 1);
    strncpy(s_xfer.target_path, target, sizeof(s_xfer.target_path) - 1);
    strncpy(s_xfer.remote_device, "Sachin's Laptop", sizeof(s_xfer.remote_device) - 1);
    s_xfer.total_bytes = (uint32_t)size;
    s_xfer.bytes_transferred = (uint32_t)size;
    s_xfer.status = BT_XFER_COMPLETE;

    OS_LOGI(TAG, "Bluetooth OBEX Received '%s' -> saved to '%s' (%u bytes)",
            filename, target, (unsigned int)size);
    return true;
}

os_bt_file_xfer_t connectivity_bt_get_xfer_status(void)
{
    return s_xfer;
}

/* ============================================================================
 * Network & HTTP Client Implementation (Powered over Bluetooth Tethering)
 * ============================================================================ */

bool connectivity_net_is_available(void)
{
    /* Network is available if Bluetooth Tethering (PAN) or USB Tethering is active */
    return s_bt_tethering || s_usb_tethering;
}

bool connectivity_http_get(const char *url, os_http_response_t *out_resp)
{
    if (!out_resp) return false;
    memset(out_resp, 0, sizeof(os_http_response_t));

    if (!connectivity_net_is_available()) {
        out_resp->status_code = 503;
        strncpy(out_resp->content_type, "text/html", sizeof(out_resp->content_type) - 1);
        const char *offline_body = 
            "<h1>No Network Connection</h1>\n"
            "<p>Bluetooth Tethering is currently disabled.</p>\n"
            "<p>To connect to the internet, please go to Settings &gt; Tethering and enable Bluetooth Tethering.</p>\n"
            "<hr>\n"
            "<p><a href=\"http://home.veebha\">Try Home Portal</a></p>";
        out_resp->body = strdup(offline_body);
        out_resp->body_len = strlen(offline_body);
        OS_LOGW(TAG, "HTTP GET '%s' failed: Tethered Network Offline", url ? url : "null");
        return false;
    }

    /* Check if external real internet request over Bluetooth Tethering */
    if (url && (strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0) &&
        strstr(url, "home.veebha") == NULL &&
        strstr(url, "news.wap") == NULL &&
        strstr(url, "weather.local") == NULL &&
        strstr(url, "tech.veebha") == NULL) {

        /* Sanitize URL string to prevent shell injection */
        char safe_url[256];
        size_t slen = 0;
        for (size_t i = 0; url[i] && slen < sizeof(safe_url) - 1; i++) {
            if (url[i] != '"' && url[i] != '`' && url[i] != '$' && url[i] != ';' && url[i] != '&' && url[i] != '|') {
                safe_url[slen++] = url[i];
            }
        }
        safe_url[slen] = '\0';

        char cmd[512];
        snprintf(cmd, sizeof(cmd), "curl -s -L --max-time 4 -A \"Mozilla/5.0 (Mobile; VeebhaOS/1.0)\" \"%s\" 2>/dev/null", safe_url);

#if defined(CONFIG_SIMULATOR) && !defined(CONFIG_IS_RAMRUN_ONLY)
        FILE *pipe = popen(cmd, "r");
        if (pipe) {
            char *buf = (char *)malloc(32768);
            if (buf) {
                size_t bytes_read = fread(buf, 1, 32767, pipe);
                buf[bytes_read] = '\0';
                pclose(pipe);
                if (bytes_read > 0) {
                    out_resp->status_code = 200;
                    strncpy(out_resp->content_type, "text/html", sizeof(out_resp->content_type) - 1);
                    out_resp->body = buf;
                    out_resp->body_len = bytes_read;
                    OS_LOGI(TAG, "HTTP GET '%s' -> Live Internet 200 OK (%zu bytes)", safe_url, bytes_read);
                    return true;
                }
                free(buf);
            } else {
                pclose(pipe);
            }
        }
#endif
    }

    out_resp->status_code = 200;
    strncpy(out_resp->content_type, "text/html", sizeof(out_resp->content_type) - 1);

    const char *body_template = NULL;

    if (!url || strlen(url) == 0 || strstr(url, "home.veebha") != NULL) {
        body_template = 
            "<h1>VeebhaOS Web Portal</h1>\n"
            "<p>Welcome to VeebhaOS Internet Lite. Fast browsing powered over Bluetooth Tethering.</p>\n"
            "<hr>\n"
            "<h2>Quick Links</h2>\n"
            "<p><a href=\"http://news.wap\">Latest News &amp; Headlines</a></p>\n"
            "<p><a href=\"http://weather.local\">Local Weather Forecast</a></p>\n"
            "<p><a href=\"http://m.wikipedia.org\">Wikipedia Mobile Lite</a></p>\n"
            "<p><a href=\"http://tech.veebha\">VeebhaOS Tech Specifications</a></p>\n"
            "<hr>\n"
            "<p>Data Status: Bluetooth Tethering Active</p>";
    } else if (strstr(url, "news.wap") != NULL) {
        body_template = 
            "<h1>Global News WAP</h1>\n"
            "<p>Updated: 24 Sep 2026</p>\n"
            "<hr>\n"
            "<h2>Technology</h2>\n"
            "<p>New embedded microkernel architecture delivers 30-day standby on feature phones.</p>\n"
            "<h2>Science &amp; Space</h2>\n"
            "<p>Deep space probe transmits high resolution telemetry from asteroid belt.</p>\n"
            "<h2>Sports</h2>\n"
            "<p>Cricket championship finals set for thrilling weekend climax.</p>\n"
            "<hr>\n"
            "<p><a href=\"http://home.veebha\">Back to Portal</a></p>";
    } else if (strstr(url, "weather.local") != NULL) {
        body_template = 
            "<h1>Local Weather</h1>\n"
            "<p>City: Global Forecast</p>\n"
            "<hr>\n"
            "<h2>Current Conditions</h2>\n"
            "<p>Temperature: 28 C (82 F)</p>\n"
            "<p>Condition: Clear Sky, Light Breeze</p>\n"
            "<p>Humidity: 45%  Wind: 12 km/h</p>\n"
            "<h2>7-Day Outlook</h2>\n"
            "<p>Thu: Sunny 29 C</p>\n"
            "<p>Fri: Partly Cloudy 27 C</p>\n"
            "<p>Sat: Light Showers 25 C</p>\n"
            "<hr>\n"
            "<p><a href=\"http://home.veebha\">Back to Portal</a></p>";
    } else if (strstr(url, "tech.veebha") != NULL) {
        body_template = 
            "<h1>VeebhaOS Tech Spec</h1>\n"
            "<p>Architecture: C11 / FreeRTOS / LVGL 9</p>\n"
            "<p>Display: 176x220 16-bit RGB565 TFT</p>\n"
            "<p>Connectivity: Bluetooth 5.0 PAN Tethering, USB RNDIS, OBEX File Xfer</p>\n"
            "<p>Storage: SPI NOR Flash LittleFS + microSD</p>\n"
            "<hr>\n"
            "<p><a href=\"http://home.veebha\">Back to Portal</a></p>";
    } else {
        /* Generic dynamic page for any custom URL */
        static char custom_page[512];
        snprintf(custom_page, sizeof(custom_page),
                 "<h1>Web Page</h1>\n"
                 "<p>Loaded: %s</p>\n"
                 "<hr>\n"
                 "<p>This page was fetched over Bluetooth PAN Tethering.</p>\n"
                 "<p>VeebhaOS Lightweight HTML engine rendered this content dynamically.</p>\n"
                 "<hr>\n"
                 "<p><a href=\"http://home.veebha\">Back to Portal</a></p>",
                 url);
        body_template = custom_page;
    }

    out_resp->body = strdup(body_template);
    out_resp->body_len = strlen(body_template);

    OS_LOGI(TAG, "HTTP GET '%s' -> 200 OK (%u bytes)", url, (unsigned int)out_resp->body_len);
    return true;
}

void connectivity_http_response_free(os_http_response_t *resp)
{
    if (resp && resp->body) {
        free(resp->body);
        resp->body = NULL;
        resp->body_len = 0;
    }
}


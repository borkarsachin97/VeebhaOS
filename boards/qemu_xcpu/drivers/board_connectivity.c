/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Board Connectivity Subsystem Implementation
 *
 * Routes OS Bluetooth and Networking requests to generic veebha_hardware.h
 * and manages lwIP / BNEP tethering connections.
 */

#include "veebha_connectivity.h"
#include "veebha_hardware.h"
#include "veebha_event.h"
#include "veebha_status_bar.h"
#include "veebha_log.h"
#include "ethernetif_bnep.h"
#include "net_app.h"
#include "hal_bt.h"
#include "bnep.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TAG "BOARD_CONNECT"

static bool s_bt_visible = true;
static os_usb_mode_t s_usb_mode = USB_MODE_NONE;
static bool s_usb_connected = false;
static bool s_usb_tethering = false;
static bool s_bt_tethering = false;

static os_bt_file_xfer_t s_xfer = {
    .filename = {0},
    .target_path = {0},
    .remote_device = {0},
    .total_bytes = 0,
    .bytes_transferred = 0,
    .status = BT_XFER_IDLE
};

static void update_status_bar_bt_state(void)
{
    bool enabled = veebha_hw_bt_is_powered();
    bool connected = veebha_hw_bt_is_connected();
    status_bar_set_bt_state(enabled, connected);
}

void connectivity_init(void)
{
    s_bt_visible = true;
    s_usb_mode = USB_MODE_NONE;
    s_usb_connected = false;
    s_usb_tethering = false;
    s_bt_tethering = false;

    net_app_init();
    update_status_bar_bt_state();
    status_bar_set_usb_mode((uint8_t)s_usb_mode);
    status_bar_set_tethering(false);
    OS_LOGI(TAG, "VeebhaOS Connectivity Subsystem Initialized");
}

void mock_connectivity_init(void)
{
    connectivity_init();
}

os_bt_state_t connectivity_bt_get_state(void)
{
    if (!veebha_hw_bt_is_powered()) {
        return BT_STATE_OFF;
    }
    if (veebha_hw_bt_is_scanning()) {
        return BT_STATE_SCANNING;
    }
    if (veebha_hw_bt_is_connected()) {
        return BT_STATE_CONNECTED;
    }
    return BT_STATE_ON;
}

void connectivity_bt_set_enabled(bool enabled)
{
    if (enabled) {
        if (!veebha_hw_bt_is_powered()) {
            veebha_hw_bt_power_on();
        }
    } else {
        if (veebha_hw_bt_is_powered()) {
            veebha_hw_bt_power_off();
            s_bt_tethering = false;
        }
    }

    update_status_bar_bt_state();

    os_event_t evt = {
        .type = OS_EVT_BT_STATE_CHANGED,
        .timestamp = 0,
        .payload = {
            .bt = { .bt_state = (uint8_t)connectivity_bt_get_state() }
        }
    };
    os_event_post(&evt);
}

bool connectivity_bt_is_enabled(void)
{
    return veebha_hw_bt_is_powered();
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
    return veebha_hw_bt_get_name();
}

void connectivity_bt_set_device_name(const char *name)
{
    if (!name || name[0] == '\0') return;
    veebha_hw_bt_set_name(name);
    OS_LOGI(TAG, "Bluetooth Device Name renamed to: '%s'", name);
}

uint16_t connectivity_bt_get_paired_devices(os_bt_device_t *out_devices, uint16_t max_count)
{
    if (!out_devices || max_count == 0 || !veebha_hw_bt_is_powered()) return 0;
    uint8_t count = veebha_hw_bt_get_paired_count();
    uint16_t res = 0;
    for (uint8_t i = 0; i < count && res < max_count; i++) {
        char name[32] = {0};
        char mac[18] = {0};
        if (!veebha_hw_bt_get_paired_device(i, name, sizeof(name), mac, sizeof(mac))) {
            continue;
        }
        strncpy(out_devices[res].name, name, sizeof(out_devices[res].name) - 1);
        out_devices[res].name[sizeof(out_devices[res].name) - 1] = '\0';
        strncpy(out_devices[res].mac, mac, sizeof(out_devices[res].mac) - 1);
        out_devices[res].mac[sizeof(out_devices[res].mac) - 1] = '\0';
        out_devices[res].type = BT_DEV_GENERIC;
        out_devices[res].rssi = -50;
        out_devices[res].is_paired = true;
        out_devices[res].is_connected = veebha_hw_bt_is_connected();
        res++;
    }
    return res;
}

uint16_t connectivity_bt_get_discovered_devices(os_bt_device_t *out_devices, uint16_t max_count)
{
    if (!out_devices || max_count == 0 || !veebha_hw_bt_is_powered()) return 0;
    uint8_t disc_count = veebha_hw_bt_get_discovered_count();
    uint16_t count = 0;

    for (uint8_t i = 0; i < disc_count && count < max_count; i++) {
        char name[32] = {0};
        char mac[18] = {0};
        int8_t rssi = -60;
        uint32_t cod = 0;

        if (!veebha_hw_bt_get_discovered_device(i, name, sizeof(name), mac, sizeof(mac), &rssi, &cod)) {
            continue;
        }

        strncpy(out_devices[count].name, name, sizeof(out_devices[count].name) - 1);
        out_devices[count].name[sizeof(out_devices[count].name) - 1] = '\0';
        strncpy(out_devices[count].mac, mac, sizeof(out_devices[count].mac) - 1);
        out_devices[count].mac[sizeof(out_devices[count].mac) - 1] = '\0';

        uint32_t major_cod = (cod >> 8) & 0x1F;
        if (major_cod == 0x01) {
            out_devices[count].type = BT_DEV_COMPUTER;
        } else if (major_cod == 0x02) {
            out_devices[count].type = BT_DEV_PHONE;
        } else if (major_cod == 0x04) {
            out_devices[count].type = BT_DEV_AUDIO;
        } else {
            out_devices[count].type = BT_DEV_GENERIC;
        }

        out_devices[count].rssi = rssi;
        out_devices[count].is_paired = false;
        out_devices[count].is_connected = false;
        count++;
    }
    return count;
}

void connectivity_bt_start_scan(void)
{
    if (veebha_hw_bt_is_powered()) {
        veebha_hw_bt_start_scan(10);
        OS_LOGI(TAG, "Bluetooth Inquiry Scan started");
    }
}

void connectivity_bt_stop_scan(void)
{
    if (veebha_hw_bt_is_powered() && veebha_hw_bt_is_scanning()) {
        veebha_hw_bt_stop_scan();
        OS_LOGI(TAG, "Bluetooth Inquiry Scan stopped");
    }
}

bool connectivity_bt_pair_device(const char *mac)
{
    if (!mac || !veebha_hw_bt_is_powered()) return false;
    return veebha_hw_bt_connect(mac);
}

bool connectivity_bt_unpair_device(const char *mac)
{
    (void)mac;
    return true;
}

bool connectivity_bt_connect_device(const char *mac)
{
    if (!mac || !veebha_hw_bt_is_powered()) return false;
    return veebha_hw_bt_connect(mac);
}

bool connectivity_bt_disconnect_device(const char *mac)
{
    (void)mac;
    return veebha_hw_bt_disconnect();
}

/* ============================================================================
 * USB & Tethering APIs
 * ============================================================================ */

os_usb_mode_t connectivity_usb_get_mode(void)
{
    return s_usb_mode;
}

void connectivity_usb_set_mode(os_usb_mode_t mode)
{
    s_usb_mode = mode;
    status_bar_set_usb_mode((uint8_t)s_usb_mode);
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
        status_bar_set_tethering(s_bt_tethering);
    }
    status_bar_set_usb_mode((uint8_t)s_usb_mode);
}

bool connectivity_tethering_get_usb(void)
{
    return s_usb_tethering;
}

void connectivity_tethering_set_usb(bool enabled)
{
    s_usb_tethering = enabled;
    status_bar_set_tethering(s_usb_tethering || s_bt_tethering);
}

bool connectivity_tethering_get_bt(void)
{
    return s_bt_tethering;
}

void connectivity_tethering_set_bt(bool enabled)
{
    s_bt_tethering = enabled;
    if (enabled && veebha_hw_bt_is_connected()) {
        extern hal_bt_status_t g_bt_status;
        extern BOOL bnep_connect(UINT16 handle, const UINT8 *remote_bd_addr);
        if (g_bt_status.connected) {
            bnep_connect(g_bt_status.conn_handle, g_bt_status.remote_bd_addr);
        }
    } else if (!enabled) {
        extern void bnep_disconnect(void);
        extern BOOL bnep_is_connected(void);
        if (bnep_is_connected()) {
            bnep_disconnect();
        }
    }
    status_bar_set_tethering(s_usb_tethering || s_bt_tethering);
}

bool connectivity_bt_send_file(const char *mac, const char *filepath)
{
    (void)mac; (void)filepath;
    return false;
}

bool connectivity_bt_receive_file(const char *filename, const uint8_t *data, size_t size)
{
    (void)filename; (void)data; (void)size;
    return false;
}

os_bt_file_xfer_t connectivity_bt_get_xfer_status(void)
{
    return s_xfer;
}

/* ============================================================================
 * Network & HTTP Client Implementation
 * ============================================================================ */

bool connectivity_net_is_available(void)
{
    return ethernetif_is_dhcp_bound();
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
            "<p>Bluetooth PAN / Internet Tethering is not connected.</p>\n"
            "<p>Connect to a Bluetooth NAP/PAN host in Settings -> Bluetooth to access the internet.</p>\n";
        out_resp->body_len = strlen(offline_body);
        out_resp->body = (char *)malloc(out_resp->body_len + 1);
        if (out_resp->body) {
            memcpy(out_resp->body, offline_body, out_resp->body_len + 1);
        }
        return true;
    }

    char host[64] = "google.com";
    char path[64] = "/";
    const char *p = url;
    if (strncmp(p, "http://", 7) == 0) p += 7;

    const char *slash = strchr(p, '/');
    if (slash) {
        size_t hlen = (size_t)(slash - p);
        if (hlen >= sizeof(host)) hlen = sizeof(host) - 1;
        strncpy(host, p, hlen);
        host[hlen] = '\0';
        strncpy(path, slash, sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
    } else {
        strncpy(host, p, sizeof(host) - 1);
        host[sizeof(host) - 1] = '\0';
    }

    net_app_http_test(host, path);

    for (int iter = 0; iter < 300; iter++) {
        veebha_hw_bt_poll();
        net_app_poll();

        const net_app_diag_t *diag = net_app_get_diag();
        if (!diag->http_in_progress) {
            out_resp->status_code = diag->http_status_code ? diag->http_status_code : 200;
            strncpy(out_resp->content_type, "text/html", sizeof(out_resp->content_type) - 1);

            char redirect_loc[128] = {0};
            const char *loc = strstr(diag->http_payload_preview, "Location: ");
            if (!loc) loc = strstr(diag->http_payload_preview, "location: ");
            if (loc) {
                loc += 10;
                size_t llen = 0;
                while (loc[llen] && loc[llen] != '\r' && loc[llen] != '\n' && llen < sizeof(redirect_loc) - 1) {
                    llen++;
                }
                strncpy(redirect_loc, loc, llen);
                redirect_loc[llen] = '\0';
            }

            char body_buf[768];
            if (out_resp->status_code == 301 || out_resp->status_code == 302 ||
                out_resp->status_code == 303 || out_resp->status_code == 307 ||
                out_resp->status_code == 308) {
                if (redirect_loc[0] != '\0') {
                    snprintf(body_buf, sizeof(body_buf),
                             "<h1>HTTP %d Redirect</h1>\n"
                             "<p><b>Host:</b> %s</p>\n"
                             "<p>Page moved to:</p>\n"
                             "<p><a href=\"%s\">%s</a></p>\n"
                             "<hr>\n"
                             "<p><a href=\"http://frogfind.com\">Search Lite Web (FrogFind)</a></p>\n"
                             "<p><a href=\"http://home.veebha\">VeebhaOS Home</a></p>\n",
                             out_resp->status_code, host, redirect_loc, redirect_loc);
                } else {
                    snprintf(body_buf, sizeof(body_buf),
                             "<h1>HTTP %d Redirect</h1>\n"
                             "<p><b>Host:</b> %s</p>\n"
                             "<p>Bytes Received: %u</p>\n"
                             "<p><b>Response:</b> %s</p>\n"
                             "<hr>\n"
                             "<p><a href=\"http://home.veebha\">VeebhaOS Home</a></p>\n",
                             out_resp->status_code, host, (unsigned int)diag->http_bytes_recv, diag->http_payload_preview);
                }
            } else {
                snprintf(body_buf, sizeof(body_buf),
                         "<h1>HTTP %d OK</h1>\n"
                         "<p><b>Host:</b> %s</p>\n"
                         "<p><b>Bytes:</b> %u</p>\n"
                         "<p><b>Preview:</b> %s</p>\n"
                         "<hr>\n"
                         "<p><a href=\"http://home.veebha\">VeebhaOS Home</a></p>\n",
                         out_resp->status_code, host, (unsigned int)diag->http_bytes_recv, diag->http_payload_preview);
            }

            out_resp->body_len = strlen(body_buf);
            out_resp->body = (char *)malloc(out_resp->body_len + 1);
            if (out_resp->body) {
                memcpy(out_resp->body, body_buf, out_resp->body_len + 1);
            }
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    out_resp->status_code = 408;
    const char *timeout_body = "<h1>Request Timeout</h1><p>The HTTP request timed out.</p>";
    out_resp->body_len = strlen(timeout_body);
    out_resp->body = (char *)malloc(out_resp->body_len + 1);
    if (out_resp->body) {
        memcpy(out_resp->body, timeout_body, out_resp->body_len + 1);
    }
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

#include "veebha_templates.h"

static char s_incoming_bt_name[32] = {0};
static char s_incoming_bt_mac[18] = {0};
static char s_incoming_bt_msg[128] = {0};

static void on_incoming_bt_accept(void)
{
    OS_LOGI(TAG, "User accepted incoming Bluetooth connection from %s (%s)", s_incoming_bt_name, s_incoming_bt_mac);
    veebha_hw_bt_accept_pending_conn(true);
    tpl_dialog_close();
}

static void on_incoming_bt_reject(void)
{
    OS_LOGI(TAG, "User rejected incoming Bluetooth connection from %s (%s)", s_incoming_bt_name, s_incoming_bt_mac);
    veebha_hw_bt_accept_pending_conn(false);
    tpl_dialog_close();
}

void board_connectivity_poll(void)
{
    veebha_hw_bt_poll();

    if (veebha_hw_bt_has_pending_conn_req() && !tpl_dialog_is_active()) {
        if (veebha_hw_bt_get_pending_conn_req(s_incoming_bt_name, sizeof(s_incoming_bt_name),
                                              s_incoming_bt_mac, sizeof(s_incoming_bt_mac))) {
            snprintf(s_incoming_bt_msg, sizeof(s_incoming_bt_msg),
                     "%s wants to connect to you.\nPress OK to connect.",
                     s_incoming_bt_name[0] ? s_incoming_bt_name : s_incoming_bt_mac);

            static tpl_dialog_desc_t conn_dlg = {
                .title = "Bluetooth Request",
                .icon = LV_SYMBOL_BLUETOOTH,
                .message = s_incoming_bt_msg,
                .lsk_label = "OK",
                .rsk_label = "Cancel",
                .on_confirm = on_incoming_bt_accept,
                .on_cancel = on_incoming_bt_reject
            };
            tpl_dialog_show(&conn_dlg);
        }
    }
}

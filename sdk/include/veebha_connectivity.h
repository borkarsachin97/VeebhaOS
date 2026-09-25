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

#ifndef SDK_INCLUDE_VEEBHA_CONNECTIVITY_H
#define SDK_INCLUDE_VEEBHA_CONNECTIVITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define BT_MAX_PAIRED_DEVICES 8
#define BT_MAX_DISCOVERED_DEVICES 8

typedef enum {
    BT_STATE_OFF = 0,
    BT_STATE_ON,
    BT_STATE_SCANNING,
    BT_STATE_CONNECTED
} os_bt_state_t;

typedef enum {
    BT_DEV_AUDIO = 0,    /* Headset / Speaker */
    BT_DEV_COMPUTER,     /* Laptop / PC */
    BT_DEV_PHONE,        /* Other feature phone / smartphone */
    BT_DEV_GENERIC
} os_bt_device_type_t;

typedef struct {
    char                name[32];
    char                mac[18];        /* e.g. "00:1A:7D:DA:71:13" */
    os_bt_device_type_t type;
    int8_t              rssi;
    bool                is_paired;
    bool                is_connected;
} os_bt_device_t;

typedef enum {
    USB_MODE_NONE = 0,
    USB_MODE_CHARGE_ONLY,
    USB_MODE_MASS_STORAGE,  /* Exposes /sdcard to host */
    USB_MODE_TETHERING,     /* RNDIS / Cellular modem sharing */
    USB_MODE_SERIAL_DEBUG   /* AT / Console debug port */
} os_usb_mode_t;

/**
 * Initialize mock connectivity subsystem.
 */
void connectivity_init(void);

/**
 * Bluetooth Radio & State Queries
 */
os_bt_state_t connectivity_bt_get_state(void);
void connectivity_bt_set_enabled(bool enabled);
bool connectivity_bt_is_enabled(void);
bool connectivity_bt_is_visible(void);
void connectivity_bt_set_visible(bool visible);
const char * connectivity_bt_get_device_name(void);
void connectivity_bt_set_device_name(const char *name);

/**
 * Bluetooth Device Management
 */
uint16_t connectivity_bt_get_paired_devices(os_bt_device_t *out_devices, uint16_t max_count);
uint16_t connectivity_bt_get_discovered_devices(os_bt_device_t *out_devices, uint16_t max_count);
void connectivity_bt_start_scan(void);
void connectivity_bt_stop_scan(void);
bool connectivity_bt_pair_device(const char *mac);
bool connectivity_bt_unpair_device(const char *mac);
bool connectivity_bt_connect_device(const char *mac);
bool connectivity_bt_disconnect_device(const char *mac);

/**
 * USB & Tethering APIs
 */
os_usb_mode_t connectivity_usb_get_mode(void);
void connectivity_usb_set_mode(os_usb_mode_t mode);
bool connectivity_usb_is_connected(void);
void connectivity_usb_set_connected(bool connected);

bool connectivity_tethering_get_usb(void);
void connectivity_tethering_set_usb(bool enabled);
bool connectivity_tethering_get_bt(void);
void connectivity_tethering_set_bt(bool enabled);

/**
 * Bluetooth OBEX / OPP File Transfer APIs
 */
typedef enum {
    BT_XFER_IDLE = 0,
    BT_XFER_SENDING,
    BT_XFER_RECEIVING,
    BT_XFER_COMPLETE,
    BT_XFER_FAILED
} os_bt_xfer_status_t;

typedef struct {
    char                filename[64];
    char                target_path[128];
    char                remote_device[32];
    uint32_t            total_bytes;
    uint32_t            bytes_transferred;
    os_bt_xfer_status_t status;
} os_bt_file_xfer_t;

bool connectivity_bt_send_file(const char *mac, const char *filepath);
bool connectivity_bt_receive_file(const char *filename, const uint8_t *data, size_t size);
os_bt_file_xfer_t connectivity_bt_get_xfer_status(void);

/**
 * Network & HTTP Client APIs (Powered over Bluetooth Tethering / PAN)
 */
typedef struct {
    int         status_code;       /* 200, 404, 503, etc. */
    char        content_type[32];  /* e.g. "text/html", "text/plain" */
    char       *body;              /* Dynamically allocated response buffer */
    size_t      body_len;
} os_http_response_t;

/**
 * Check if active data network is available (Bluetooth Tethering / PAN or USB Tethering).
 */
bool connectivity_net_is_available(void);

/**
 * Perform a simulated/mock or real HTTP GET request over the active tethered network.
 *
 * @param url The target HTTP URL.
 * @param out_resp Output HTTP response struct (caller must free with connectivity_http_response_free).
 * @return true on network success, false on network error or offline.
 */
bool connectivity_http_get(const char *url, os_http_response_t *out_resp);

/**
 * Free resources allocated within an HTTP response.
 */
void connectivity_http_response_free(os_http_response_t *resp);

#ifdef __cplusplus
}
#endif

#endif /* SDK_INCLUDE_VEEBHA_CONNECTIVITY_H */

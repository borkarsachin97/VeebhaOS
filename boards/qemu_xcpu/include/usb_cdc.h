/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Bare-metal USB CDC-ACM (Virtual Serial Port) Logger Driver Header
 */

#ifndef _USB_CDC_H_
#define _USB_CDC_H_

#include "cs_types.h"
#include <stdarg.h>

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

// =============================================================================
//  USB STANDARD CONSTANTS & DESCRIPTOR TYPES
// =============================================================================
#define USB_DESCRIPTOR_TYPE_DEVICE               0x01
#define USB_DESCRIPTOR_TYPE_CONFIGURATION        0x02
#define USB_DESCRIPTOR_TYPE_STRING               0x03
#define USB_DESCRIPTOR_TYPE_INTERFACE            0x04
#define USB_DESCRIPTOR_TYPE_ENDPOINT             0x05
#define USB_DESCRIPTOR_TYPE_CS_INTERFACE         0x24

// CDC Class / Subclass / Protocol Codes
#define USB_CLASS_CDC_COMM                       0x02
#define USB_CLASS_CDC_DATA                       0x0A
#define USB_SUBCLASS_ACM                         0x02
#define USB_PROTO_NONE                           0x00
#define USB_PROTO_AT_COMMANDS                    0x01

// CDC Subtype Descriptors
#define CDC_HEADER_DESCRIPTOR_SUBTYPE            0x00
#define CDC_CALL_MANAGEMENT_SUBTYPE              0x01
#define CDC_ABSTRACT_CONTROL_MANAGEMENT_SUBTYPE  0x02
#define CDC_UNION_DESCRIPTOR_SUBTYPE             0x06

// CDC Class Request Codes
#define CDC_REQUEST_SET_LINE_CODING              0x20
#define CDC_REQUEST_GET_LINE_CODING              0x21
#define CDC_REQUEST_SET_CONTROL_LINE_STATE       0x22

// Standard USB Request Codes
#define USB_REQUEST_GET_STATUS                   0x00
#define USB_REQUEST_CLEAR_FEATURE                0x01
#define USB_REQUEST_SET_FEATURE                  0x03
#define USB_REQUEST_SET_ADDRESS                  0x05
#define USB_REQUEST_GET_DESCRIPTOR               0x06
#define USB_REQUEST_SET_CONFIGURATION            0x09

// USB Endpoints
#define USB_EP0_OUT                              0x00
#define USB_EP0_IN                               0x80
#define USB_EP1_IN                               0x81
#define USB_EP2_IN                               0x82
#define USB_EP3_OUT                              0x03

// Ring Buffer Size (16 KiB) - stores logs in RAM while USB is suspended during playback
#define LOG_RING_BUFFER_SIZE                     16384

// =============================================================================
//  USB DESCRIPTOR STRUCTURES
// =============================================================================

typedef struct PACKED {
    UINT8  bLength;
    UINT8  bDescriptorType;
    UINT16 bcdUSB;
    UINT8  bDeviceClass;
    UINT8  bDeviceSubClass;
    UINT8  bDeviceProtocol;
    UINT8  bMaxPacketSize0;
    UINT16 idVendor;
    UINT16 idProduct;
    UINT16 bcdDevice;
    UINT8  iManufacturer;
    UINT8  iProduct;
    UINT8  iSerialNumber;
    UINT8  bNumConfigurations;
} usb_device_descriptor_t;

typedef struct PACKED {
    UINT8  bLength;
    UINT8  bDescriptorType;
    UINT16 wTotalLength;
    UINT8  bNumInterfaces;
    UINT8  bConfigurationValue;
    UINT8  iConfiguration;
    UINT8  bmAttributes;
    UINT8  bMaxPower;
} usb_config_descriptor_t;

typedef struct PACKED {
    UINT8  bLength;
    UINT8  bDescriptorType;
    UINT8  bInterfaceNumber;
    UINT8  bAlternateSetting;
    UINT8  bNumEndpoints;
    UINT8  bInterfaceClass;
    UINT8  bInterfaceSubClass;
    UINT8  bInterfaceProtocol;
    UINT8  iInterface;
} usb_interface_descriptor_t;

typedef struct PACKED {
    UINT8  bLength;
    UINT8  bDescriptorType;
    UINT8  bEndpointAddress;
    UINT8  bmAttributes;
    UINT16 wMaxPacketSize;
    UINT8  bInterval;
} usb_endpoint_descriptor_t;

// CDC Functional Descriptors
typedef struct PACKED {
    UINT8  bFunctionLength;
    UINT8  bDescriptorType;
    UINT8  bDescriptorSubtype;
    UINT16 bcdCDC;
} usb_cdc_header_descriptor_t;

typedef struct PACKED {
    UINT8  bFunctionLength;
    UINT8  bDescriptorType;
    UINT8  bDescriptorSubtype;
    UINT8  bmCapabilities;
    UINT8  bDataInterface;
} usb_cdc_call_mgmt_descriptor_t;

typedef struct PACKED {
    UINT8  bFunctionLength;
    UINT8  bDescriptorType;
    UINT8  bDescriptorSubtype;
    UINT8  bmCapabilities;
} usb_cdc_acm_descriptor_t;

typedef struct PACKED {
    UINT8  bFunctionLength;
    UINT8  bDescriptorType;
    UINT8  bDescriptorSubtype;
    UINT8  bMasterInterface;
    UINT8  bSlaveInterface0;
} usb_cdc_union_descriptor_t;

// CDC Line Coding (7 bytes)
typedef struct PACKED {
    UINT32 dwDTERate;
    UINT8  bCharFormat;
    UINT8  bParityType;
    UINT8  bDataBits;
} usb_cdc_line_coding_t;

// USB Setup Packet (8 bytes)
typedef struct PACKED {
    UINT8  bmRequestType;
    UINT8  bRequest;
    UINT16 wValue;
    UINT16 wIndex;
    UINT16 wLength;
} usb_setup_packet_t;

// USB Device State Enums
typedef enum {
    USB_STATE_DETACHED = 0,
    USB_STATE_WAIT_ENUM,
    USB_STATE_ATTACHED,
    USB_STATE_DEFAULT,
    USB_STATE_ADDRESS,
    USB_STATE_CONFIGURED
} usb_device_state_t;

// =============================================================================
//  PUBLIC API PROTOTYPES
// =============================================================================

void usb_cdc_init(void);
void usb_phy_force_reattach(void);
void usb_cdc_poll(void);
void usb_cdc_irq_handler(uint32_t cause, uint32_t epc);
UINT32 usb_cdc_log_write(const char *buf, UINT32 len);
UINT32 os_log_printf(const char *fmt, ...);
UINT32 os_log_get_used_bytes(void);
UINT32 os_log_get_total_capacity(void);
void usb_cdc_flush_tx(void);
UINT32 hal_UsbGetIrqCause(void);
usb_device_state_t usb_cdc_get_state(void);
void usb_cdc_set_state(usb_device_state_t state);
const char *usb_cdc_get_state_string(void);
BOOL usb_cdc_is_disconnect_pending(void);
void usb_cdc_clear_disconnect_pending(void);
void usb_cdc_request_disconnect(void);
BOOL usb_cdc_is_suspended(void);
UINT32 usb_cdc_get_total_bytes_sent(void);

#endif // _USB_CDC_H_

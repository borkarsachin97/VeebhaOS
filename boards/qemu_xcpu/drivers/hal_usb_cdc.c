/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Bare-metal USB CDC-ACM Virtual Serial Port Driver for RDA8809 / RDA8955 SoC
 * (Synopsys DesignWare DWC2 USB OTG Controller Core)
 * Ported from: soft/platform/chip/hal/src/hal_usb.c
 *              soft/platform/chip/boot/8809/src/boot_usb.c
 *              soft/platform/chip/regs/8809/include/usbc.h
 */

#include "cs_types.h"
#include "global_macros.h"
#include "sys_ctrl.h"
#include "sys_irq.h"
#include "usbc.h"
#include "hal_usb.h"
#include "hal_ispi.h"
#include "usb_cdc.h"
#include "timer.h"
#include "os_vector_table.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdarg.h>

// =============================================================================
//  STATIC CONFIGURATION & DESCRIPTORS
// =============================================================================

static volatile usb_device_state_t g_usb_state = USB_STATE_DETACHED;
static volatile BOOL g_usb_disconnect_pending = FALSE;
static UINT8 g_usb_address = 0;
static UINT32 g_total_bytes_sent = 0;

// Standard USB Device Descriptor (VID: 0x1D6B, PID: 0x0104 - CDC-ACM)
static const usb_device_descriptor_t g_cdc_device_descriptor = {
    .bLength            = sizeof(usb_device_descriptor_t),
    .bDescriptorType    = USB_DESCRIPTOR_TYPE_DEVICE,
    .bcdUSB             = 0x0110, // USB 1.1
    .bDeviceClass       = USB_CLASS_CDC_COMM,
    .bDeviceSubClass    = USB_SUBCLASS_ACM,
    .bDeviceProtocol    = USB_PROTO_NONE,
    .bMaxPacketSize0    = 64,
    .idVendor           = 0x1D6B, // Linux Foundation / Generic CDC
    .idProduct          = 0x0104, // Multifunction Composite / CDC ACM
    .bcdDevice          = 0x0100, // v1.0
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1
};

// Complete CDC-ACM Configuration Descriptor Structure (67 bytes total)
typedef struct PACKED {
    usb_config_descriptor_t        config;
    usb_interface_descriptor_t     comm_interface;
    usb_cdc_header_descriptor_t    cdc_header;
    usb_cdc_call_mgmt_descriptor_t cdc_call_mgmt;
    usb_cdc_acm_descriptor_t       cdc_acm;
    usb_cdc_union_descriptor_t      cdc_union;
    usb_endpoint_descriptor_t      ep1_notification;
    usb_interface_descriptor_t     data_interface;
    usb_endpoint_descriptor_t      ep2_bulk_in;
    usb_endpoint_descriptor_t      ep3_bulk_out;
} cdc_config_group_t;

static const cdc_config_group_t g_cdc_config_descriptor = {
    .config = {
        .bLength             = sizeof(usb_config_descriptor_t),
        .bDescriptorType     = USB_DESCRIPTOR_TYPE_CONFIGURATION,
        .wTotalLength        = sizeof(cdc_config_group_t),
        .bNumInterfaces      = 2,
        .bConfigurationValue = 1,
        .iConfiguration      = 0,
        .bmAttributes        = 0xC0, // Self-powered
        .bMaxPower           = 50    // 100 mA
    },
    .comm_interface = {
        .bLength            = sizeof(usb_interface_descriptor_t),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_INTERFACE,
        .bInterfaceNumber   = 0,
        .bAlternateSetting  = 0,
        .bNumEndpoints      = 1,
        .bInterfaceClass    = USB_CLASS_CDC_COMM,
        .bInterfaceSubClass = USB_SUBCLASS_ACM,
        .bInterfaceProtocol = USB_PROTO_AT_COMMANDS,
        .iInterface         = 0
    },
    .cdc_header = {
        .bFunctionLength    = sizeof(usb_cdc_header_descriptor_t),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .bDescriptorSubtype = CDC_HEADER_DESCRIPTOR_SUBTYPE,
        .bcdCDC             = 0x0110 // CDC 1.10
    },
    .cdc_call_mgmt = {
        .bFunctionLength    = sizeof(usb_cdc_call_mgmt_descriptor_t),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .bDescriptorSubtype = CDC_CALL_MANAGEMENT_SUBTYPE,
        .bmCapabilities     = 0x00,
        .bDataInterface     = 1
    },
    .cdc_acm = {
        .bFunctionLength    = sizeof(usb_cdc_acm_descriptor_t),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .bDescriptorSubtype = CDC_ABSTRACT_CONTROL_MANAGEMENT_SUBTYPE,
        .bmCapabilities     = 0x02 // Line coding & control state support
    },
    .cdc_union = {
        .bFunctionLength    = sizeof(usb_cdc_union_descriptor_t),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .bDescriptorSubtype = CDC_UNION_DESCRIPTOR_SUBTYPE,
        .bMasterInterface   = 0,
        .bSlaveInterface0   = 1
    },
    .ep1_notification = {
        .bLength          = sizeof(usb_endpoint_descriptor_t),
        .bDescriptorType  = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = USB_EP1_IN,
        .bmAttributes     = 0x03, // Interrupt
        .wMaxPacketSize   = 8,
        .bInterval        = 10
    },
    .data_interface = {
        .bLength            = sizeof(usb_interface_descriptor_t),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_INTERFACE,
        .bInterfaceNumber   = 1,
        .bAlternateSetting  = 0,
        .bNumEndpoints      = 2,
        .bInterfaceClass    = USB_CLASS_CDC_DATA,
        .bInterfaceSubClass = 0x00,
        .bInterfaceProtocol = 0x00,
        .iInterface         = 0
    },
    .ep2_bulk_in = {
        .bLength          = sizeof(usb_endpoint_descriptor_t),
        .bDescriptorType  = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = USB_EP2_IN,
        .bmAttributes     = 0x02, // Bulk
        .wMaxPacketSize   = 64,
        .bInterval        = 0
    },
    .ep3_bulk_out = {
        .bLength          = sizeof(usb_endpoint_descriptor_t),
        .bDescriptorType  = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = USB_EP3_OUT,
        .bmAttributes     = 0x02, // Bulk
        .wMaxPacketSize   = 64,
        .bInterval        = 0
    }
};

// String Descriptors
static const UINT8 g_string0[] = { 4, USB_DESCRIPTOR_TYPE_STRING, 0x09, 0x04 }; // English (US)
static const UINT8 g_string_mfr[] = {
    18, USB_DESCRIPTOR_TYPE_STRING,
    'R',0, 'D',0, 'A',0, ' ',0, 'M',0, 'i',0, 'c',0, 'r',0
};
static const UINT8 g_string_prod[] = {
    34, USB_DESCRIPTOR_TYPE_STRING,
    'R',0, 'D',0, 'A',0, ' ',0, 'U',0, 'S',0, 'B',0, ' ',0,
    'C',0, 'D',0, 'C',0, ' ',0, 'L',0, 'o',0, 'g',0, 'g',0
};
static const UINT8 g_string_serial[] = {
    32, USB_DESCRIPTOR_TYPE_STRING,
    'R',0, 'D',0, 'A',0, '8',0, '8',0, '0',0, '9',0, '-',0,
    '0',0, '0',0, '1',0, '0',0, '0',0, '0',0, '0',0
};

// CDC Line Coding Default: 115200 Baud, 1 Stop Bit, No Parity, 8 Data Bits
static usb_cdc_line_coding_t g_cdc_line_coding = {
    .dwDTERate   = 115200,
    .bCharFormat = 0, // 1 Stop bit
    .bParityType = 0, // None
    .bDataBits   = 8  // 8 Data bits
};

// =============================================================================
//  8 KiB LOCKLESS RAM RING BUFFER FOR LOGGING
// =============================================================================
static UINT8  g_log_ring_buffer[LOG_RING_BUFFER_SIZE];
static volatile UINT32 g_log_head = 0;
static volatile UINT32 g_log_tail = 0;

// Write raw bytes into ring buffer — ALWAYS saved whether USB is configured or not.
// If buffer is full, advances tail pointer so newest logs overwrite oldest.
UINT32 usb_cdc_log_write(const char *buf, UINT32 len)
{
    if (!buf || len == 0) return 0;

    extern volatile uint32_t g_freertos_scheduler_started;
    BOOL in_rtos = (g_freertos_scheduler_started != 0);

    if (in_rtos) {
        taskENTER_CRITICAL();
    }

    UINT32 written = 0;
    while (written < len)
    {
        UINT32 next_head = (g_log_head + 1) & (LOG_RING_BUFFER_SIZE - 1);
        if (next_head == g_log_tail) {
            // Buffer full: advance tail to overwrite oldest byte
            g_log_tail = (g_log_tail + 1) & (LOG_RING_BUFFER_SIZE - 1);
        }
        g_log_ring_buffer[g_log_head] = (UINT8)buf[written];
        g_log_head = next_head;
        written++;
    }

    if (in_rtos) {
        taskEXIT_CRITICAL();
    }

    return written;
}

// Minimal printf-style log — always formats and stores into ring buffer.
// Supports: %s %d %i %u %x %X and width/zero-pad e.g. %02u %08x %5d
UINT32 os_log_printf(const char *fmt, ...)
{
    char buf[256];
    int  len = 0;
    char *p  = buf;

    va_list args;
    va_start(args, fmt);

    for (const char *f = fmt; *f && len < 252; f++)
    {
        if (*f != '%') { *p++ = *f; len++; continue; }

        f++; // skip '%'
        if (*f == '\0') break;

        // --- Parse optional flags/width: e.g. '0', '8' in %08x ---
        char pad_ch  = ' ';
        int  pad_w   = 0;
        int  prec    = -1;

        if (*f == '0') { pad_ch = '0'; f++; }
        while (*f >= '0' && *f <= '9') { pad_w = pad_w * 10 + (*f - '0'); f++; }
        if (*f == '.')
        {
            f++;
            prec = 0;
            while (*f >= '0' && *f <= '9') { prec = prec * 10 + (*f - '0'); f++; }
        }

        // --- Render type ---
        if (*f == 's')
        {
            const char *s = va_arg(args, const char*);
            if (!s) s = "(null)";
            int s_cnt = 0;
            while (*s && len < 252 && (prec < 0 || s_cnt < prec)) { *p++ = *s++; len++; s_cnt++; }
        }
        else if (*f == 'd' || *f == 'i')
        {
            int val = va_arg(args, int);
            char nbuf[12]; int ni = 0;
            if (val < 0) { *p++ = '-'; len++; val = -val; }
            if (val == 0) nbuf[ni++] = '0';
            while (val > 0) { nbuf[ni++] = '0' + (val % 10); val /= 10; }
            // Zero/space pad
            for (int pad = pad_w - ni; pad > 0 && len < 252; pad--) { *p++ = pad_ch; len++; }
            while (ni > 0 && len < 252) { *p++ = nbuf[--ni]; len++; }
        }
        else if (*f == 'u')
        {
            UINT32 val = va_arg(args, UINT32);
            char nbuf[12]; int ni = 0;
            if (val == 0) nbuf[ni++] = '0';
            while (val > 0) { nbuf[ni++] = '0' + (val % 10); val /= 10; }
            for (int pad = pad_w - ni; pad > 0 && len < 252; pad--) { *p++ = pad_ch; len++; }
            while (ni > 0 && len < 252) { *p++ = nbuf[--ni]; len++; }
        }
        else if (*f == 'x' || *f == 'X')
        {
            UINT32 val = va_arg(args, UINT32);
            char nbuf[12]; int ni = 0;
            const char *hx = (*f == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";
            if (val == 0) nbuf[ni++] = '0';
            while (val > 0) { nbuf[ni++] = hx[val & 0xF]; val >>= 4; }
            for (int pad = pad_w - ni; pad > 0 && len < 252; pad--) { *p++ = pad_ch; len++; }
            while (ni > 0 && len < 252) { *p++ = nbuf[--ni]; len++; }
        }
        else if (*f == 'p')
        {
            UINT32 val = (UINT32)va_arg(args, void*);
            const char *hx = "0123456789abcdef";
            if (len < 242) {
                *p++ = '0'; *p++ = 'x'; len += 2;
                for (int i = 7; i >= 0; i--) {
                    *p++ = hx[(val >> (i * 4)) & 0xF];
                    len++;
                }
            }
        }
        else if (*f == '%') { *p++ = '%'; len++; }
        // Unknown specifier: consume & ignore
    }

    va_end(args);
    *p = '\0';
    return os_log_write(buf, len);
}

UINT32 os_log_get_used_bytes(void)
{
    UINT32 head = g_log_head;
    UINT32 tail = g_log_tail;
    if (head >= tail) {
        return head - tail;
    } else {
        return LOG_RING_BUFFER_SIZE - (tail - head);
    }
}

UINT32 os_log_get_total_capacity(void)
{
    return LOG_RING_BUFFER_SIZE;
}

BOOL usb_cdc_is_disconnect_pending(void)
{
    return g_usb_disconnect_pending;
}

void usb_cdc_clear_disconnect_pending(void)
{
    g_usb_disconnect_pending = FALSE;
}

void usb_cdc_request_disconnect(void)
{
    g_usb_disconnect_pending = TRUE;
}

BOOL usb_cdc_is_suspended(void)
{
    if (g_usb_state == USB_STATE_DETACHED) return FALSE;
    return (hwp_usbc->DSTS & 1) ? TRUE : FALSE;
}

void usb_cdc_set_state(usb_device_state_t state)
{
    g_usb_state = state;
}

const char *usb_cdc_get_state_string(void)
{
    switch (g_usb_state) {
        case USB_STATE_DETACHED:   return "DETACHED";
        case USB_STATE_WAIT_ENUM:  return "WAIT 2S";
        case USB_STATE_ATTACHED:   return "ATTACHED";
        case USB_STATE_DEFAULT:    return "DEFAULT";
        case USB_STATE_ADDRESS:    return "ADDRESS";
        case USB_STATE_CONFIGURED: return "CONFIGURED";
        default:                   return "UNKNOWN";
    }
}


// =============================================================================
//  BARE-METAL USB CDC DRIVER CORE & HARDWARE CONTROL
// ============================================================================

UINT32 hal_UsbGetIrqCause(void)
{
    if (g_usb_state == USB_STATE_DETACHED) return 0;
    UINT32 cause = hwp_usbc->GINTSTS & hwp_usbc->GINTMSK;
    // Clear handled write-1-to-clear interrupt bits
    hwp_usbc->GINTSTS = cause & (USBC_GINTSTS_USBRST | USBC_GINTSTS_ENUMDONE);
    return cause;
}

// Flush all TX FIFOs and RX FIFO
static void hal_UsbFlushFifos(void)
{
    if (g_usb_state == USB_STATE_DETACHED) return;
    hwp_usbc->GRSTCTL = (1 << 4) | (0x10 << 6); // Flush all TX FIFOs
    volatile UINT32 to1 = 10000;
    while ((hwp_usbc->GRSTCTL & (1 << 5)) && --to1) ;

    hwp_usbc->GRSTCTL = (1 << 4); // Flush RX FIFO
    volatile UINT32 to2 = 10000;
    while ((hwp_usbc->GRSTCTL & (1 << 4)) && --to2) ;
}

void hal_UsbOpen(void)
{
    // Step 1: Unlock sysCtrl and assert PHY / USBC clock enables
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_UNLOCK;
    hwp_sysCtrl->Clk_Other_Enable = SYS_CTRL_ENABLE_OC_USBPHY;
    hwp_sysCtrl->Sys_Rst_Clr = SYS_CTRL_CLR_RST_USBPHY | SYS_CTRL_CLR_RST_USBC;
    hwp_sysCtrl->Clk_Per_Enable |= SYS_CTRL_ENABLE_PER_USBC;
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_LOCK;
    timer_delay_ms(10);

    // Step 2: Un-gate PHY clock in USBC
    hwp_usbc->PCGCCTL &= ~(1 << 0);
    timer_delay_ms(5);

    // Step 3: Enable USB PHY in ABB analog controller via ISPI
    hal_AbbWrite(ABB_REG_USB_CONTROL, (1 << 4) | (7 << 1) | (3 << 14) | (3 << 12) | (4 << 9));
    timer_delay_ms(5);

    // Step 4: Core Soft Reset
    hwp_usbc->GRSTCTL = 1;
    volatile UINT32 reset_timeout = 100000;
    while ((hwp_usbc->GRSTCTL & 1) && --reset_timeout) ;
    timer_delay_ms(10);

    // Step 5: Configure FIFO boundaries
    hwp_usbc->GRXFSIZ = 64;
    hwp_usbc->GNPTXFSIZ = 64 | (48 << 16);
    hwp_usbc->DIEPTXF[0].DIEnPTXF = 112 | (32 << 16);
    hwp_usbc->DIEPTXF[1].DIEnPTXF = 144 | (64 << 16);

    hal_UsbFlushFifos();

    // Step 6: AHB Configuration
    hwp_usbc->GAHBCFG = (1 << 0) | (1 << 7);

    // Step 7: Device Configuration: Full-Speed PHY (12 Mbps, speed=3) and clear address
    hwp_usbc->DCFG = (hwp_usbc->DCFG & ~((0x7F << 4) | 3)) | 3;

    // Step 8: Configure EP0 OUT to receive SETUP packets
    hwp_usbc->DOEPTSIZ0 = (1 << 29) | (1 << 19) | 64;
    hwp_usbc->DOEPCTL0 |= (1 << 31) | (1 << 26);

    // Step 9: Clear pending interrupt flags and enable USB interrupt masks
    // Only enable USBRST, ENUMDONE, and RXFLVL. Do NOT enable USBSUSP, DISCONNINT,
    // OTGINT, IEPINT, or OEPINT to avoid infinite unhandled level-triggered IRQ storms!
    hwp_usbc->GINTSTS = 0xFFFFFFFF;
    hwp_usbc->GINTMSK = USBC_GINTSTS_USBRST | USBC_GINTSTS_ENUMDONE | USBC_GINTSTS_RXFLVL;

    // Step 10: Clear Soft Disconnect bit (asserts D+ pull-up)
    hwp_usbc->DCTL &= ~(1 << 1);

    // Step 11: Enable USBC in system interrupt controller
    hwp_sysIrq->Mask_Set = SYS_IRQ_SYS_IRQ_USBC;

    g_usb_address = 0;
    g_usb_disconnect_pending = FALSE;
    g_total_bytes_sent = 0;
    g_usb_state = USB_STATE_ATTACHED;
}

void hal_UsbClose(void)
{
    g_usb_state = USB_STATE_DETACHED;

    // 1. Immediately mask USBC in system interrupt controller to prevent IRQ storms
    hwp_sysIrq->Mask_Clear = SYS_IRQ_SYS_IRQ_USBC;
    hwp_sysIrq->Pulse_Clear = SYS_IRQ_SYS_IRQ_USBC;

    // Enable peripheral clock temporarily to issue soft disconnect
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_UNLOCK;
    hwp_sysCtrl->Clk_Per_Enable |= SYS_CTRL_ENABLE_PER_USBC;
    hwp_sysCtrl->Sys_Rst_Clr = SYS_CTRL_CLR_RST_USBC;
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_LOCK;

    // 2. Clear any pending USB core interrupts and disable masks
    hwp_usbc->GINTMSK = 0;
    hwp_usbc->GINTSTS = 0xFFFFFFFF;

    // 3. Soft disconnect in USBC core (detaches D+ pull-up)
    hwp_usbc->DCTL |= (1 << 1);

    // 4. Power down USB PHY in ABB analog controller
    hal_AbbWrite(ABB_REG_USB_CONTROL, 0xA819);

    // 5. Gate USBC PHY clock
    hwp_usbc->PCGCCTL |= (1 << 0);

    // 6. Reset USBC clocks in sysCtrl
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_UNLOCK;
    hwp_sysCtrl->Clk_Other_Disable = SYS_CTRL_DISABLE_OC_USBPHY;
    hwp_sysCtrl->Sys_Rst_Set = SYS_CTRL_SET_RST_USBPHY | SYS_CTRL_SET_RST_USBC;
    hwp_sysCtrl->Clk_Per_Disable = SYS_CTRL_DISABLE_PER_USBC;
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_LOCK;
}

INT32 hal_UsbSend(UINT8 ep, UINT8* buffer, UINT16 size, UINT32 flag)
{
    (void)flag;
    if (g_usb_state == USB_STATE_DETACHED) return 0;
    UINT8 ep_num = HAL_USB_EP_NUM(ep);

    volatile REG32 *fifo = &hwp_usbc->EPnFIFO[ep_num].TxRxData;
    UINT32 words = (size + 3) / 4;

    if (ep_num == 0) {
        UINT32 pkt_cnt = (size == 0) ? 1 : ((size + 63) / 64);
        hwp_usbc->DIEPTSIZ0 = (pkt_cnt << 19) | (size & 0x7F);
        hwp_usbc->DIEPCTL0 |= (1 << 31) | (1 << 26);
    } else if (ep_num <= 3) {
        UINT32 pkt_cnt = (size == 0) ? 1 : ((size + 63) / 64);

        // If endpoint is busy (waiting for host IN token), skip sending to prevent hang/overflow
        if (hwp_usbc->DIEPnCONFIG[ep_num - 1].DIEPCTL & (1 << 31)) return 0;

        // Check TX FIFO has enough space (DIEPFSTS[15:0] = words free)
        UINT32 fifo_space = hwp_usbc->DIEPnCONFIG[ep_num - 1].DIEPFSTS & 0xFFFF;
        if (fifo_space < words) return 0;

        hwp_usbc->DIEPnCONFIG[ep_num - 1].DIEPTSIZ = (pkt_cnt << 19) | (size & 0x7FF);
        hwp_usbc->DIEPnCONFIG[ep_num - 1].DIEPCTL |= (1 << 31) | (1 << 26);
    }

    if (size > 0 && buffer != NULL)
    {
        const UINT8 *b = (const UINT8 *)buffer;
        for (UINT32 i = 0; i < size; i += 4) {
            UINT32 word = 0;
            UINT32 rem = size - i;
            if (rem >= 4) {
                word = (UINT32)b[i] | ((UINT32)b[i+1] << 8) | ((UINT32)b[i+2] << 16) | ((UINT32)b[i+3] << 24);
            } else {
                for (UINT32 k = 0; k < rem; k++) {
                    word |= ((UINT32)b[i + k] << (k * 8));
                }
            }
            *fifo = word;
        }
        g_total_bytes_sent += size;
    }

    return size;
}

INT32 hal_UsbRecv(UINT8 ep, UINT8* buffer, UINT16 size, UINT32 flag)
{
    (void)buffer;
    (void)flag;
    if (g_usb_state == USB_STATE_DETACHED) return 0;
    UINT8 ep_num = HAL_USB_EP_NUM(ep);

    if (ep_num == 0) {
        hwp_usbc->DOEPTSIZ0 = (1 << 19) | (size & 0x7F);
        hwp_usbc->DOEPCTL0 |= (1 << 31) | (1 << 26);
    } else if (ep_num <= 2) {
        hwp_usbc->DOEPnCONFIG[ep_num - 1].DOEPTSIZ = (1 << 19) | (size & 0x7FF);
        hwp_usbc->DOEPnCONFIG[ep_num - 1].DOEPCTL |= (1 << 31) | (1 << 26);
    }
    return 0;
}

void hal_UsbEpStall(UINT8 ep, BOOL stall)
{
    if (g_usb_state == USB_STATE_DETACHED) return;
    UINT8 ep_num = HAL_USB_EP_NUM(ep);
    if (HAL_USB_IS_EP_DIRECTION_IN(ep)) {
        if (ep_num == 0) {
            if (stall) hwp_usbc->DIEPCTL0 |= (1 << 21);
            else hwp_usbc->DIEPCTL0 &= ~(1 << 21);
        } else if (ep_num <= 3) {
            if (stall) hwp_usbc->DIEPnCONFIG[ep_num - 1].DIEPCTL |= (1 << 21);
            else hwp_usbc->DIEPnCONFIG[ep_num - 1].DIEPCTL &= ~(1 << 21);
        }
    }
}

// =============================================================================
//  EP0 CONTROL SETUP PACKET HANDLER & STATE MACHINE
// =============================================================================

static void usb_cdc_handle_setup(usb_setup_packet_t *setup)
{
    if (!setup || g_usb_state == USB_STATE_DETACHED) return;

    UINT8 req_type = setup->bmRequestType;
    UINT8 req      = setup->bRequest;
    UINT16 val     = setup->wValue;
    UINT16 len     = setup->wLength;

    // Standard Setup Requests
    if ((req_type & 0x60) == 0x00)
    {
        switch (req)
        {
            case USB_REQUEST_GET_DESCRIPTOR:
            {
                UINT8 desc_type = (val >> 8) & 0xFF;
                UINT8 desc_idx  = val & 0xFF;

                if (desc_type == USB_DESCRIPTOR_TYPE_DEVICE)
                {
                    UINT16 send_len = (len < sizeof(usb_device_descriptor_t)) ? len : sizeof(usb_device_descriptor_t);
                    hal_UsbSend(USB_EP0_IN, (UINT8*)&g_cdc_device_descriptor, send_len, 0);
                }
                else if (desc_type == USB_DESCRIPTOR_TYPE_CONFIGURATION)
                {
                    UINT16 send_len = (len < sizeof(cdc_config_group_t)) ? len : sizeof(cdc_config_group_t);
                    hal_UsbSend(USB_EP0_IN, (UINT8*)&g_cdc_config_descriptor, send_len, 0);
                }
                else if (desc_type == USB_DESCRIPTOR_TYPE_STRING)
                {
                    const UINT8 *str_ptr = NULL;
                    UINT16 str_len = 0;

                    if (desc_idx == 0) { str_ptr = g_string0; str_len = sizeof(g_string0); }
                    else if (desc_idx == 1) { str_ptr = g_string_mfr; str_len = sizeof(g_string_mfr); }
                    else if (desc_idx == 2) { str_ptr = g_string_prod; str_len = sizeof(g_string_prod); }
                    else if (desc_idx == 3) { str_ptr = g_string_serial; str_len = sizeof(g_string_serial); }

                    if (str_ptr) {
                        UINT16 send_len = (len < str_len) ? len : str_len;
                        hal_UsbSend(USB_EP0_IN, (UINT8*)str_ptr, send_len, 0);
                    } else {
                        hal_UsbEpStall(USB_EP0_IN, TRUE);
                    }
                }
                else {
                    hal_UsbEpStall(USB_EP0_IN, TRUE);
                }
                break;
            }

            case USB_REQUEST_SET_ADDRESS:
            {
                g_usb_address = (UINT8)(val & 0x7F);
                hwp_usbc->DCFG = (hwp_usbc->DCFG & ~(0x7F << 4)) | (g_usb_address << 4);
                g_usb_state = USB_STATE_ADDRESS;
                hal_UsbSend(USB_EP0_IN, NULL, 0, 0);
                break;
            }

            case USB_REQUEST_SET_CONFIGURATION:
            {
                if ((val & 0xFF) == 1) {
                    g_usb_state = USB_STATE_CONFIGURED;

                    // EP1 IN (Interrupt, 8-byte MPS, FIFO 1, DATA0 PID, SNAK, USBAEP)
                    // Do NOT set bit 31 (EPENA)! EPENA is set dynamically when queuing a packet.
                    hwp_usbc->DIEPnCONFIG[0].DIEPCTL = (1 << 28) | (1 << 27) | (1 << 22) | (1 << 15) | (3 << 18) | (8 << 0);

                    // EP2 IN (Bulk TX, 64-byte MPS, FIFO 2, DATA0 PID, SNAK, USBAEP)
                    // Do NOT set bit 31 (EPENA)! EPENA is set dynamically when queuing a packet.
                    hwp_usbc->DIEPnCONFIG[1].DIEPCTL = (1 << 28) | (1 << 27) | (2 << 22) | (1 << 15) | (2 << 18) | (64 << 0);

                    // EP3 OUT (Bulk RX, 64-byte MPS)
                    hwp_usbc->DOEPnCONFIG[0].DOEPTSIZ = (1 << 19) | 64;
                    hwp_usbc->DOEPnCONFIG[0].DOEPCTL = (1 << 31) | (1 << 28) | (1 << 26) | (1 << 15) | (2 << 18) | (64 << 0);

                    hal_UsbSend(USB_EP0_IN, NULL, 0, 0);
                } else {
                    g_usb_state = USB_STATE_ADDRESS;
                    hal_UsbSend(USB_EP0_IN, NULL, 0, 0);
                }
                break;
            }

            case USB_REQUEST_GET_STATUS:
            {
                static const UINT16 status_data = 0x0001;
                hal_UsbSend(USB_EP0_IN, (UINT8*)&status_data, 2, 0);
                break;
            }

            default:
                hal_UsbSend(USB_EP0_IN, NULL, 0, 0);
                break;
        }
    }
    // CDC Class Requests
    else if ((req_type & 0x60) == 0x20)
    {
        switch (req)
        {
            case CDC_REQUEST_SET_LINE_CODING:
            {
                hal_UsbRecv(USB_EP0_OUT, (UINT8*)&g_cdc_line_coding, sizeof(usb_cdc_line_coding_t), 0);
                hal_UsbSend(USB_EP0_IN, NULL, 0, 0);
                break;
            }

            case CDC_REQUEST_GET_LINE_CODING:
            {
                UINT16 send_len = (len < sizeof(usb_cdc_line_coding_t)) ? len : sizeof(usb_cdc_line_coding_t);
                hal_UsbSend(USB_EP0_IN, (UINT8*)&g_cdc_line_coding, send_len, 0);
                break;
            }

            case CDC_REQUEST_SET_CONTROL_LINE_STATE:
            {
                hal_UsbSend(USB_EP0_IN, NULL, 0, 0);
                break;
            }

            default:
                hal_UsbEpStall(USB_EP0_IN, TRUE);
                break;
        }
    }
}

// =============================================================================
//  USB PHY FORCE DISCONNECT & RE-ATTACH SEQUENCE
// =============================================================================

void usb_phy_force_reattach(void)
{
    hal_UsbClose();
    g_usb_state = USB_STATE_DETACHED;
    g_usb_address = 0;

    timer_delay_ms(5000);

    hal_UsbOpen();
    g_log_head = 0;
    g_log_tail = 0;
    g_total_bytes_sent = 0;
}

void usb_cdc_init(void)
{
    hal_UsbClose();
    g_usb_state = USB_STATE_DETACHED;
    g_usb_disconnect_pending = FALSE;
    os_register_irq_handler(IRQ_INDEX_USB, usb_cdc_irq_handler);
}

void usb_cdc_flush_tx(void)
{
    if (g_usb_state != USB_STATE_CONFIGURED) return;
    if (g_log_head == g_log_tail) return;

    UINT8 __attribute__((aligned(4))) tx_chunk[64];
    UINT32 chunk_len = 0;
    UINT32 tail = g_log_tail;

    while (g_log_head != tail && chunk_len < 64)
    {
        tx_chunk[chunk_len++] = g_log_ring_buffer[tail];
        tail = (tail + 1) & (LOG_RING_BUFFER_SIZE - 1);
    }

    if (chunk_len > 0)
    {
        INT32 sent = hal_UsbSend(USB_EP2_IN, tx_chunk, (UINT16)chunk_len, 0);
        if (sent > 0)
        {
            g_log_tail = tail;
        }
    }
}

void usb_cdc_poll(void)
{
    if (g_usb_state == USB_STATE_DETACHED) return;
    UINT32 irq_cause = hal_UsbGetIrqCause();

    if (irq_cause & USBC_GINTSTS_USBRST)
    {
        g_usb_state = USB_STATE_DEFAULT;
        g_usb_address = 0;
        // Do NOT reset log ring on bus reset so buffered logs are flushed upon reconnection
        hwp_usbc->DCFG = (hwp_usbc->DCFG & ~((0x7F << 4) | 3)) | 3;
        hal_UsbFlushFifos();
        hwp_usbc->DOEPTSIZ0 = (1 << 29) | (1 << 19) | 64;
        hwp_usbc->DOEPCTL0 |= (1 << 31) | (1 << 26);
    }

    if (irq_cause & USBC_GINTSTS_ENUMDONE)
    {
        g_usb_state = USB_STATE_DEFAULT;
        g_usb_address = 0;
        hwp_usbc->DCFG = (hwp_usbc->DCFG & ~((0x7F << 4) | 3)) | 3;
        hwp_usbc->DOEPTSIZ0 = (1 << 29) | (1 << 19) | 64;
        hwp_usbc->DOEPCTL0 |= (1 << 31) | (1 << 26);
    }

    // Drain pending RX FIFO entries — bounded to 32 iterations max to avoid infinite loop
    UINT32 rx_drain_limit = 32;
    while ((hwp_usbc->GINTSTS & USBC_GINTSTS_RXFLVL) && rx_drain_limit--)
    {
        UINT32 rx_sts    = hwp_usbc->GRXSTSP;
        UINT8 pkt_status = (rx_sts >> 17) & 0x0F;
        UINT8 ep_num     = rx_sts & 0x0F;
        UINT32 byte_cnt  = (rx_sts >> 4) & 0x7FF;

        if (pkt_status == 0x06 && ep_num == 0) // SETUP packet on EP0
        {
            volatile UINT32 *fifo = (volatile UINT32*)&hwp_usbc->EPnFIFO[0].TxRxData;
            usb_setup_packet_t setup_pkt;
            UINT32 *setup_words = (UINT32*)&setup_pkt;
            setup_words[0] = *fifo;
            setup_words[1] = *fifo;

            usb_cdc_handle_setup(&setup_pkt);

            hwp_usbc->DOEPTSIZ0 = (1 << 29) | (1 << 19) | 64;
            hwp_usbc->DOEPCTL0 |= (1 << 31) | (1 << 26);
        }
        else if (pkt_status == 0x02 && byte_cnt > 0) // OUT data packet
        {
            volatile UINT32 *fifo = (volatile UINT32*)&hwp_usbc->EPnFIFO[ep_num].TxRxData;
            UINT32 words = (byte_cnt + 3) / 4;
            // Safety: cap drain to 16 words (64 bytes max)
            if (words > 16) words = 16;
            for (UINT32 i = 0; i < words; i++) {
                volatile UINT32 dummy = *fifo;
                (void)dummy;
            }
        }
        // For all other status codes (0x01=GOUT_NAK, 0x03=DATA_TOGGLE_ERR,
        // 0x04=SETUP_COMP, 0x07=OUT_COMP) just consume the RXSTS entry
    }

    usb_cdc_flush_tx();
}

usb_device_state_t usb_cdc_get_state(void)
{
    return g_usb_state;
}

UINT32 usb_cdc_get_total_bytes_sent(void)
{
    return g_total_bytes_sent;
}

static volatile UINT32 g_usb_irq_count = 0;

void usb_cdc_irq_handler(uint32_t cause, uint32_t epc)
{
    (void)cause;
    (void)epc;
    g_usb_irq_count++;
    usb_cdc_poll();
}

UINT32 usb_cdc_get_irq_count(void)
{
    return g_usb_irq_count;
}


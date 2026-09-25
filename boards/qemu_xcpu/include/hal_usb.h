/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * RDA HAL USB Interface Header
 */

#ifndef _HAL_USB_H_
#define _HAL_USB_H_

#include "cs_types.h"
#include "usb_cdc.h"

#define HAL_USB_EP_DIRECTION                  0x80
#define HAL_USB_EP_DIRECTION_IN(X)            ((X)|HAL_USB_EP_DIRECTION)
#define HAL_USB_EP_DIRECTION_OUT(X)           ((X)&~HAL_USB_EP_DIRECTION)
#define HAL_USB_IS_EP_DIRECTION_IN(X)         ((X)&HAL_USB_EP_DIRECTION)
#define HAL_USB_EP_NUM(X)                     ((X)&(~HAL_USB_EP_DIRECTION))
#define HAL_USB_MPS                           64

typedef enum {
    HAL_USB_CALLBACK_TYPE_CMD,
    HAL_USB_CALLBACK_TYPE_DATA_CMD,
    HAL_USB_CALLBACK_TYPE_RECEIVE_END,
    HAL_USB_CALLBACK_TYPE_TRANSMIT_END,
    HAL_USB_CALLBACK_TYPE_ENABLE,
    HAL_USB_CALLBACK_TYPE_DISABLE
} HAL_USB_CALLBACK_EP_TYPE_T;

typedef struct {
    UINT8  requestDest     :5;
    UINT8  requestType     :2;
    UINT8  requestDirection:1;
} HAL_USB_SETUP_REQUEST_DESC_T;

typedef struct {
    HAL_USB_SETUP_REQUEST_DESC_T  requestDesc;
    UINT8                         request;
    UINT16                        value;
    UINT16                        index;
    UINT16                        lenght;
} PACKED HAL_USB_SETUP_T;

typedef enum {
    HAL_USB_CALLBACK_RETURN_OK,
    HAL_USB_CALLBACK_RETURN_RUNNING,
    HAL_USB_CALLBACK_RETURN_KO
} HAL_USB_CALLBACK_RETURN_T;

typedef HAL_USB_CALLBACK_RETURN_T (*HAL_USB_CALLBACK_T)(HAL_USB_CALLBACK_EP_TYPE_T type, HAL_USB_SETUP_T* setup);

void hal_UsbOpen(void);
void hal_UsbClose(void);
INT32 hal_UsbSend(UINT8 ep, UINT8* buffer, UINT16 size, UINT32 flag);
INT32 hal_UsbRecv(UINT8 ep, UINT8* buffer, UINT16 size, UINT32 flag);
void hal_UsbEpStall(UINT8 ep, BOOL stall);
UINT32 hal_UsbGetIrqCause(void);
BOOL hal_UsbEpTransfertDone(UINT8 ep);

#endif // _HAL_USB_H_

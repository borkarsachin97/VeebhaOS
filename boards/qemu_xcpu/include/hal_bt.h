/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * RDABT 8809 Bluetooth HAL Driver Interface
 */

#ifndef _HAL_BT_H_
#define _HAL_BT_H_

#include "cs_types.h"

#define RDABT_I2C_RF_ADDR       0x16
#define RDABT_I2C_CORE_ADDR     0x15

typedef struct {
    UINT8 bd_addr[6];
    UINT8 link_key[16];
    UINT8 key_type;
    BOOL  valid;
    char  name[32];
} hal_bt_paired_dev_t;

typedef struct {
    BOOL   powered;
    BOOL   rf_detected;
    BOOL   core_detected;
    UINT32 chip_id;       // 32-bit: e.g. 0x11005990 (R17) or 0x12005990 (R18)
    UINT16 rf_reg0;       // Page 0 Reg 0x00
    UINT8  bd_addr[6];    // Bluetooth Device MAC Address
    BOOL   hci_ready;     // HCI Reset verified over UART
    BOOL   connected;     // Active ACL connection
    BOOL   paired;        // Link key authenticated
    UINT16 conn_handle;   // HCI Connection Handle
    UINT8  remote_bd_addr[6]; // Connected Peer MAC
} hal_bt_status_t;

#define BT_MAX_DISCOVERED_DEVICES 16

typedef struct {
    UINT8  bd_addr[6];
    char   name[32];
    INT8   rssi;
    UINT32 cod;
    BOOL   is_paired;
    BOOL   is_connected;
    UINT8  rep_mode;
    UINT16 clock_offset;
} bt_remote_dev_t;

// Bluetooth HAL Functions
void hal_BtInit(void);
BOOL hal_BtPowerOn(void);
void hal_BtPowerOff(void);
BOOL hal_BtIsPowered(void);
BOOL hal_BtProbe(hal_bt_status_t *status);
BOOL hal_BtInitRf(void);
BOOL hal_BtTestHci(UINT8 *out_bd_addr);
void hal_BtDumpRegisters(void);
void hal_BtPollEvents(void);
uint32_t hal_BtSendPacket(const uint8_t *pkt, uint32_t len);
BOOL hal_BtAcceptConnection(const UINT8 *bd_addr, UINT8 role);
BOOL hal_BtRejectConnection(const UINT8 *bd_addr, UINT8 reason);
BOOL hal_BtLinkKeyNegativeReply(const UINT8 *bd_addr);
BOOL hal_BtLinkKeyReply(const UINT8 *bd_addr, const UINT8 *link_key);
BOOL hal_BtPinCodeReply(const UINT8 *bd_addr, const char *pin);
BOOL hal_BtIoCapabilityReply(const UINT8 *bd_addr, UINT8 io_cap, UINT8 oob, UINT8 auth);
BOOL hal_BtUserConfirmationReply(const UINT8 *bd_addr);
BOOL hal_BtUserPasskeyReply(const UINT8 *bd_addr, UINT32 passkey);

// Inquiry & Connection Management
BOOL hal_BtStartInquiry(UINT8 duration_sec);
BOOL hal_BtCancelInquiry(void);
BOOL hal_BtConnect(const UINT8 *bd_addr);
BOOL hal_BtDisconnect(UINT16 handle);
BOOL hal_BtRequestRemoteName(const UINT8 *bd_addr, uint8_t rep_mode, uint16_t clk_off);
BOOL hal_BtCancelRemoteNameRequest(const UINT8 *bd_addr);
BOOL hal_BtSetEncryption(UINT16 handle, uint8_t enable);
BOOL hal_BtRequestAuthentication(UINT16 handle);
BOOL hal_BtMakeDiscoverable(BOOL discoverable);
UINT8 hal_BtGetDiscoveredCount(void);
const bt_remote_dev_t* hal_BtGetDiscoveredDevice(UINT8 index);
BOOL hal_BtIsScanning(void);
void hal_BtClearDiscoveredDevices(void);
BOOL hal_BtIsPairedWith(const UINT8 *bd_addr);
BOOL hal_BtIsPairingInProgress(void);
void hal_BtSetPairingInProgress(BOOL in_progress);
BOOL hal_BtSetLocalName(const char *name);
const char* hal_BtGetLocalName(void);
UINT8 hal_BtGetPairedCount(void);
const hal_bt_paired_dev_t* hal_BtGetPairedDevice(UINT8 index);
BOOL hal_BtHasPendingConnection(void);
BOOL hal_BtGetPendingConnectionInfo(char *name, UINT16 name_sz, char *mac, UINT16 mac_sz);
void hal_BtAcceptPendingConnection(BOOL accept);

#endif // _HAL_BT_H_

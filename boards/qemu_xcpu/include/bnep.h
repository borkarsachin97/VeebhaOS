#ifndef _BNEP_H_
#define _BNEP_H_

#include "cs_types.h"

// BNEP Packet Types (Bluetooth SIG BNEP 1.0 Table 2.1)
#define BNEP_GENERAL_ETHERNET               0x00
#define BNEP_CONTROL                        0x01
#define BNEP_COMPRESSED_ETHERNET            0x02
#define BNEP_COMPRESSED_ETHERNET_SRC_ONLY   0x03
#define BNEP_COMPRESSED_ETHERNET_DEST_ONLY  0x04

// BNEP Control Command Codes
#define BNEP_CMD_SETUP_CONNECTION_REQ_MSG   0x01
#define BNEP_CMD_SETUP_CONNECTION_RSP_MSG   0x02
#define BNEP_CMD_FILTER_NET_TYPE_SET_MSG    0x03
#define BNEP_CMD_FILTER_NET_TYPE_RSP_MSG    0x04
#define BNEP_CMD_FILTER_MULTI_ADDR_SET_MSG  0x05
#define BNEP_CMD_FILTER_MULTI_ADDR_RSP_MSG  0x06

// BNEP Response Codes
#define BNEP_RSP_SUCCESS                    0x0000
#define BNEP_RSP_INVALID_DEST_UUID          0x0001
#define BNEP_RSP_INVALID_SRC_UUID           0x0002
#define BNEP_RSP_INVALID_UUID_SIZE          0x0003
#define BNEP_RSP_NOT_ALLOWED                0x0004

// Service UUIDs
#define BNEP_UUID_PANU                      0x1115
#define BNEP_UUID_NAP                       0x1116
#define BNEP_UUID_GN                        0x1117

// L2CAP PSM for BNEP
#define L2CAP_PSM_BNEP                      0x000F

// Dynamic Local CID assigned for BNEP
#define BNEP_LOCAL_CID                      0x0040

typedef enum {
    BNEP_STATE_IDLE = 0,
    BNEP_STATE_L2CAP_CONNECTING,
    BNEP_STATE_L2CAP_CONFIGURING,
    BNEP_STATE_SETUP_REQ_SENT,
    BNEP_STATE_CONNECTED,
    BNEP_STATE_DISCONNECTING
} bnep_state_t;

typedef struct {
    bnep_state_t state;
    uint16_t     acl_handle;
    uint16_t     local_cid;
    uint16_t     remote_cid;
    uint8_t      remote_mac[6];
    uint32_t     rx_packets;
    uint32_t     tx_packets;
    uint32_t     rx_bytes;
    uint32_t     tx_bytes;
} bnep_conn_t;

void bnep_init(void);
BOOL bnep_connect(uint16_t acl_handle, const uint8_t *remote_mac);
void bnep_disconnect(void);
BOOL bnep_is_connected(void);
const bnep_conn_t* bnep_get_status(void);
void bnep_accept_incoming_connection(uint16_t acl_handle, uint16_t remote_scid);

// L2CAP Dispatch hooks from HAL
void bnep_handle_l2cap_conn_rsp(uint16_t scid, uint16_t dcid, uint16_t result);
void bnep_handle_l2cap_cfg_req(uint16_t dcid, uint8_t req_id, uint16_t mtu);
void bnep_handle_l2cap_cfg_rsp(uint16_t scid, uint16_t result);
void bnep_handle_l2cap_disc_req(uint16_t dcid, uint16_t scid);
void bnep_handle_l2cap_data(uint16_t handle, uint16_t cid, const uint8_t *data, uint16_t len);

// Data Send hook from lwIP
BOOL bnep_send_ethernet_packet(const uint8_t *eth_frame, uint16_t len);

// Periodic polling hook for state transitions & retransmissions
void bnep_poll(void);

#endif // _BNEP_H_

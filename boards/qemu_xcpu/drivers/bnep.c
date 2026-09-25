/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Bluetooth Network Encapsulation Protocol (BNEP) Implementation
 * Personal Area Network User (PANU) Profile Client
 */

#include "cs_types.h"
#include "hal_uart.h"
#include "timer.h"
#include "bnep.h"
#include "hal_bt.h"
#include "extern.h"

extern void os_log_printf(const char *fmt, ...);
extern hal_bt_status_t g_bt_status;
extern void ethernetif_bnep_input(const uint8_t *frame, uint16_t len);
extern void ethernetif_bnep_link_changed(BOOL up);

static bnep_conn_t s_bnep_conn = {0};
static uint8_t s_l2cap_req_id = 0x10;
static uint32_t s_bnep_connect_start_ms = 0;
static uint32_t s_bnep_setup_req_ms = 0;
static uint8_t  s_bnep_setup_retry = 0;
static BOOL s_bnep_cfg_in_done = FALSE;
static BOOL s_bnep_cfg_out_done = FALSE;
static BOOL s_bnep_is_incoming = FALSE;

void bnep_init(void)
{
    memset(&s_bnep_conn, 0, sizeof(s_bnep_conn));
    s_bnep_conn.state = BNEP_STATE_IDLE;
    s_bnep_conn.local_cid = BNEP_LOCAL_CID;
    s_bnep_connect_start_ms = 0;
    s_bnep_setup_req_ms = 0;
    s_bnep_setup_retry = 0;
    s_bnep_cfg_in_done = FALSE;
    s_bnep_cfg_out_done = FALSE;
    s_bnep_is_incoming = FALSE;
}

BOOL bnep_is_connected(void)
{
    return (s_bnep_conn.state == BNEP_STATE_CONNECTED);
}

const bnep_conn_t* bnep_get_status(void)
{
    return &s_bnep_conn;
}

void bnep_accept_incoming_connection(uint16_t acl_handle, uint16_t remote_scid)
{
    if (s_bnep_conn.state == BNEP_STATE_CONNECTED && s_bnep_conn.remote_cid == remote_scid)
    {
        os_log_printf("[BNEP] Re-accepting duplicate connection request on same CID 0x%04X\n", remote_scid);
        return;
    }

    s_bnep_conn.acl_handle = acl_handle;
    s_bnep_conn.local_cid = BNEP_LOCAL_CID;
    s_bnep_conn.remote_cid = remote_scid;
    memcpy(s_bnep_conn.remote_mac, g_bt_status.remote_bd_addr, 6);
    s_bnep_conn.state = BNEP_STATE_L2CAP_CONFIGURING;
    s_bnep_is_incoming = TRUE;
    s_bnep_connect_start_ms = timer_get_ms();
    s_bnep_cfg_in_done = FALSE;
    s_bnep_cfg_out_done = FALSE;
    s_bnep_conn.rx_packets = 0;
    s_bnep_conn.tx_packets = 0;
    s_bnep_conn.rx_bytes = 0;
    s_bnep_conn.tx_bytes = 0;
    os_log_printf("[BNEP] Accepted incoming BNEP L2CAP connection (Handle 0x%04X, Remote CID 0x%04X)\n",
                  acl_handle, remote_scid);
}

BOOL bnep_connect(uint16_t acl_handle, const uint8_t *remote_mac)
{
    if (s_bnep_conn.state != BNEP_STATE_IDLE)
    {
        // If stuck in a non-connected state for over 10 seconds, allow retry/re-send
        if (s_bnep_conn.state != BNEP_STATE_CONNECTED &&
            (timer_get_ms() - s_bnep_connect_start_ms) > 10000)
        {
            os_log_printf("[BNEP] Retrying connection attempt (prev state=%d)...\n", s_bnep_conn.state);
            s_bnep_conn.state = BNEP_STATE_IDLE;
        }
        else
        {
            os_log_printf("[BNEP] Cannot connect: state=%d\n", s_bnep_conn.state);
            return FALSE;
        }
    }

    if (s_bnep_conn.state == BNEP_STATE_CONNECTED) {
        os_log_printf("[BNEP] Already connected!\n");
        return TRUE;
    }
    if (s_bnep_conn.state == BNEP_STATE_L2CAP_CONNECTING ||
        s_bnep_conn.state == BNEP_STATE_L2CAP_CONFIGURING ||
        s_bnep_conn.state == BNEP_STATE_SETUP_REQ_SENT)
    {
        os_log_printf("[BNEP] Connection already in progress (state=%d), waiting...\n", s_bnep_conn.state);
        return TRUE;
    }

    s_bnep_connect_start_ms = timer_get_ms();
    s_bnep_is_incoming = FALSE;

    s_bnep_conn.acl_handle = acl_handle;
    s_bnep_conn.local_cid = BNEP_LOCAL_CID;
    s_bnep_conn.remote_cid = 0;
    if (remote_mac) {
        memcpy(s_bnep_conn.remote_mac, remote_mac, 6);
    }
    s_bnep_conn.state = BNEP_STATE_L2CAP_CONNECTING;
    s_bnep_cfg_in_done = FALSE;
    s_bnep_cfg_out_done = FALSE;
    s_bnep_conn.rx_packets = 0;
    s_bnep_conn.tx_packets = 0;
    s_bnep_conn.rx_bytes = 0;
    s_bnep_conn.tx_bytes = 0;

    os_log_printf("[BNEP] Initiating L2CAP Connection (PSM 0x000F, SCID 0x%04X, Handle 0x%04X)...\n",
                  BNEP_LOCAL_CID, acl_handle);

    // L2CAP Connection Request (Code 0x02, PSM 0x000F, SCID 0x0040)
    uint8_t req_id = ++s_l2cap_req_id;
    uint8_t l2cap_conn_req[17] = {
        0x02,                                                       // HCI ACL Packet
        (uint8_t)(acl_handle & 0xFF), (uint8_t)(((acl_handle >> 8) & 0x0F) | 0x20), // Handle + PB=2
        12, 0x00,                                                   // ACL Data Length = 12
        8, 0x00,                                                    // L2CAP Length = 8
        0x01, 0x00,                                                 // CID = 0x0001 (Signaling)
        0x02,                                                       // Code: Connection Request
        req_id,                                                     // Identifier
        0x04, 0x00,                                                 // Command Length: 4
        (uint8_t)(L2CAP_PSM_BNEP & 0xFF), (uint8_t)(L2CAP_PSM_BNEP >> 8), // PSM: 0x000F
        (uint8_t)(BNEP_LOCAL_CID & 0xFF), (uint8_t)(BNEP_LOCAL_CID >> 8)  // SCID: 0x0040
    };

    hal_BtSendPacket(l2cap_conn_req, sizeof(l2cap_conn_req));
    return TRUE;
}

void bnep_disconnect(void)
{
    if (s_bnep_conn.state == BNEP_STATE_IDLE) return;

    os_log_printf("[BNEP] Disconnecting BNEP / L2CAP Channel 0x%04X...\n", s_bnep_conn.remote_cid);
    s_bnep_conn.state = BNEP_STATE_DISCONNECTING;

    if (s_bnep_conn.remote_cid != 0)
    {
        uint8_t req_id = ++s_l2cap_req_id;
        uint8_t disc_req[17] = {
            0x02,
            (uint8_t)(s_bnep_conn.acl_handle & 0xFF), (uint8_t)(((s_bnep_conn.acl_handle >> 8) & 0x0F) | 0x20),
            12, 0x00,
            8, 0x00,
            0x01, 0x00,
            0x06,                                                   // Code: Disconnection Request
            req_id,
            0x04, 0x00,
            (uint8_t)(s_bnep_conn.remote_cid & 0xFF), (uint8_t)(s_bnep_conn.remote_cid >> 8), // DCID
            (uint8_t)(s_bnep_conn.local_cid & 0xFF), (uint8_t)(s_bnep_conn.local_cid >> 8)    // SCID
        };
        hal_BtSendPacket(disc_req, sizeof(disc_req));
    }

    s_bnep_conn.state = BNEP_STATE_IDLE;
    s_bnep_is_incoming = FALSE;
    s_bnep_cfg_in_done = FALSE;
    s_bnep_cfg_out_done = FALSE;
    s_bnep_setup_req_ms = 0;
    s_bnep_setup_retry = 0;
    ethernetif_bnep_link_changed(FALSE);
}

void bnep_handle_l2cap_conn_rsp(uint16_t scid, uint16_t dcid, uint16_t result)
{
    if (scid != BNEP_LOCAL_CID) return;

    if (result == 0x0000)
    {
        s_bnep_conn.remote_cid = dcid;
        s_bnep_conn.state = BNEP_STATE_L2CAP_CONFIGURING;
        os_log_printf("[BNEP] L2CAP Connection Accepted! DCID=0x%04X, SCID=0x%04X\n", dcid, scid);

        // Send L2CAP Configuration Request: Negotiate MTU 1691 bytes (0x069B)
        uint8_t req_id = ++s_l2cap_req_id;
        uint8_t cfg_req[21] = {
            0x02,
            (uint8_t)(s_bnep_conn.acl_handle & 0xFF), (uint8_t)(((s_bnep_conn.acl_handle >> 8) & 0x0F) | 0x20),
            16, 0x00,                                               // ACL Length = 16
            12, 0x00,                                               // L2CAP Length = 12
            0x01, 0x00,                                             // CID: 0x0001 (Signaling)
            0x04,                                                   // Code: Configuration Request
            req_id,                                                 // Identifier
            0x08, 0x00,                                             // Length: 8
            (uint8_t)(dcid & 0xFF), (uint8_t)(dcid >> 8),           // Destination CID
            0x00, 0x00,                                             // Flags: 0
            0x01, 0x02,                                             // Option: MTU (type 1, len 2)
            0x9B, 0x06                                              // MTU = 1691 bytes
        };
        hal_BtSendPacket(cfg_req, sizeof(cfg_req));
    }
    else if (result == 0x0001)
    {
        s_bnep_conn.remote_cid = dcid;
        os_log_printf("[BNEP] L2CAP Connection Pending (Peer DCID 0x%04X, awaiting authorization/pairing)...\n", dcid);
    }
    else
    {
        os_log_printf("[BNEP] L2CAP Connection Refused (Result 0x%04X). Is Bluetooth Tethering enabled on phone?\n", result);
        s_bnep_conn.state = BNEP_STATE_IDLE;
    }
}

static void bnep_send_setup_req_packet(void)
{
    os_log_printf("[BNEP] Sending BNEP_SETUP_CONNECTION_REQ (PANU -> NAP, Remote CID 0x%04X)...\n",
                  s_bnep_conn.remote_cid);

    // Send BNEP Setup Connection Request:
    // UUID Size = 2 bytes, Dest UUID = 0x1116 (NAP), Source UUID = 0x1115 (PANU)
    uint8_t bnep_setup_req[] = {
        0x02,
        (uint8_t)(s_bnep_conn.acl_handle & 0xFF), (uint8_t)(((s_bnep_conn.acl_handle >> 8) & 0x0F) | 0x20),
        11, 0x00,                                               // ACL Length = 11 (4 L2CAP hdr + 7 BNEP payload)
        7, 0x00,                                                // L2CAP Length = 7
        (uint8_t)(s_bnep_conn.remote_cid & 0xFF), (uint8_t)(s_bnep_conn.remote_cid >> 8), // Destination CID
        BNEP_CONTROL,                                           // Type 0x01: Control packet
        BNEP_CMD_SETUP_CONNECTION_REQ_MSG,                      // Code 0x01: Setup Req
        0x02,                                                   // UUID size: 2 bytes
        (uint8_t)(BNEP_UUID_NAP >> 8), (uint8_t)(BNEP_UUID_NAP & 0xFF),   // Dest: 0x1116 (NAP, big-endian)
        (uint8_t)(BNEP_UUID_PANU >> 8), (uint8_t)(BNEP_UUID_PANU & 0xFF)  // Src : 0x1115 (PANU, big-endian)
    };

    s_bnep_conn.state = BNEP_STATE_SETUP_REQ_SENT;
    s_bnep_setup_req_ms = timer_get_ms();
    hal_BtSendPacket(bnep_setup_req, sizeof(bnep_setup_req));
}

static void bnep_check_send_setup_req(void)
{
    // Proceed ONLY once BOTH sides have successfully completed L2CAP configuration
    if (s_bnep_cfg_in_done && s_bnep_cfg_out_done && s_bnep_conn.state == BNEP_STATE_L2CAP_CONFIGURING)
    {
        if (s_bnep_is_incoming)
        {
            os_log_printf("[BNEP] L2CAP configured for incoming channel. Awaiting peer setup request...\n");
        }
        else
        {
            s_bnep_setup_retry = 0;
            bnep_send_setup_req_packet();
        }
    }
}

void bnep_handle_l2cap_cfg_req(uint16_t dcid, uint8_t req_id, uint16_t mtu)
{
    // Accept config requests targeting our BNEP channel (local CID 0x0040 or remote CID)
    if (dcid != BNEP_LOCAL_CID && dcid != s_bnep_conn.remote_cid && dcid != 0x0001)
    {
        if (s_bnep_conn.state != BNEP_STATE_L2CAP_CONFIGURING) return;
    }

    os_log_printf("[BNEP] Received Remote L2CAP Config Request (DCID=0x%04X, Peer MTU=%u, ID=%u)\n",
                  dcid, mtu, req_id);

    // Send L2CAP Configuration Response (Success)
    uint16_t resp_scid = s_bnep_conn.remote_cid ? s_bnep_conn.remote_cid : BNEP_LOCAL_CID;
    uint8_t cfg_rsp[19] = {
        0x02,
        (uint8_t)(s_bnep_conn.acl_handle & 0xFF), (uint8_t)(((s_bnep_conn.acl_handle >> 8) & 0x0F) | 0x20),
        14, 0x00,                                                   // ACL Length = 14
        10, 0x00,                                                   // L2CAP Length = 10
        0x01, 0x00,                                                 // CID: 0x0001 (Signaling)
        0x05,                                                       // Code: Configuration Response
        req_id,                                                     // Identifier
        0x06, 0x00,                                                 // Length: 6
        (uint8_t)(resp_scid & 0xFF), (uint8_t)(resp_scid >> 8),     // SCID = peer's SCID
        0x00, 0x00,                                                 // Flags: 0
        0x00, 0x00                                                  // Result: 0x0000 (Success)
    };
    hal_BtSendPacket(cfg_rsp, sizeof(cfg_rsp));

    s_bnep_cfg_in_done = TRUE;
    bnep_check_send_setup_req();
}

void bnep_handle_l2cap_cfg_rsp(uint16_t scid, uint16_t result)
{
    if (scid != BNEP_LOCAL_CID && scid != s_bnep_conn.remote_cid) return;

    if (result == 0x0000)
    {
        os_log_printf("[BNEP] Remote accepted our L2CAP Config Request (Result 0x0000)!\n");
        s_bnep_cfg_out_done = TRUE;
        bnep_check_send_setup_req();
    }
    else
    {
        os_log_printf("[BNEP] L2CAP Config Failed (Result 0x%04X)\n", result);
        s_bnep_conn.state = BNEP_STATE_IDLE;
    }
}

void bnep_handle_l2cap_disc_req(uint16_t dcid, uint16_t scid)
{
    if (scid == s_bnep_conn.remote_cid && (dcid == BNEP_LOCAL_CID || dcid == s_bnep_conn.local_cid))
    {
        os_log_printf("[BNEP] Active L2CAP Channel 0x%04X disconnected by peer\n", scid);
        s_bnep_conn.state = BNEP_STATE_IDLE;
        s_bnep_conn.remote_cid = 0;
        ethernetif_bnep_link_changed(FALSE);
    }
    else
    {
        os_log_printf("[BNEP] Stale/inactive L2CAP Channel 0x%04X closed (active remote is 0x%04X)\n",
                      scid, s_bnep_conn.remote_cid);
    }
}

void bnep_poll(void)
{
    if (s_bnep_conn.state == BNEP_STATE_SETUP_REQ_SENT)
    {
        uint32_t now = timer_get_ms();
        if ((now - s_bnep_setup_req_ms) > 1500)
        {
            if (s_bnep_setup_retry < 3)
            {
                s_bnep_setup_retry++;
                os_log_printf("[BNEP] Resending BNEP_SETUP_CONNECTION_REQ (retry %u/3)...\n", s_bnep_setup_retry);
                bnep_send_setup_req_packet();
            }
            else
            {
                os_log_printf("[BNEP] Setup Connection Request timed out after 3 retries.\n");
                s_bnep_conn.state = BNEP_STATE_IDLE;
            }
        }
    }
    else if (s_bnep_conn.state == BNEP_STATE_L2CAP_CONFIGURING)
    {
        if (s_bnep_cfg_in_done && s_bnep_cfg_out_done)
        {
            if (s_bnep_is_incoming)
            {
                // Fallback: If incoming peer doesn't send setup request within 2500 ms, send setup request ourselves
                uint32_t now = timer_get_ms();
                if ((now - s_bnep_connect_start_ms) > 2500)
                {
                    os_log_printf("[BNEP] Incoming peer setup request timeout - fallback sending setup req...\n");
                    s_bnep_is_incoming = FALSE;
                    s_bnep_setup_retry = 0;
                    bnep_send_setup_req_packet();
                }
            }
            else
            {
                bnep_check_send_setup_req();
            }
        }
    }
}

void bnep_handle_l2cap_data(uint16_t handle, uint16_t cid, const uint8_t *data, uint16_t len)
{
    if (cid != BNEP_LOCAL_CID && cid != s_bnep_conn.local_cid) return;
    if (len < 1) return;

    s_bnep_conn.rx_packets++;
    s_bnep_conn.rx_bytes += len;

    uint8_t pkt_type = data[0] & 0x7F; // bit 7 is extension flag
    uint8_t has_ext = (data[0] & 0x80) != 0;

    // 1. BNEP Control Packet
    if (pkt_type == BNEP_CONTROL)
    {
        if (len < 2) return;
        uint8_t ctrl_type = data[1];

        if (ctrl_type == BNEP_CMD_SETUP_CONNECTION_REQ_MSG)
        {
            uint8_t uuid_size = (len >= 3) ? data[2] : 0;
            uint16_t dst_uuid = 0;
            uint16_t src_uuid = 0;
            if (uuid_size == 2 && len >= 7)
            {
                dst_uuid = (uint16_t)((data[3] << 8) | data[4]);
                src_uuid = (uint16_t)((data[5] << 8) | data[6]);
            }
            os_log_printf("[BNEP] Received Remote Setup Request: Dst=0x%04X, Src=0x%04X (Size=%u)\n",
                          dst_uuid, src_uuid, uuid_size);

            if (dst_uuid == BNEP_UUID_NAP)
            {
                // Peer asked us to be NAP (0x1116), but we are PANU client!
                os_log_printf("[BNEP] Peer asked us to be NAP (0x1116), but we are PANU client! Rejecting invalid dest UUID...\n");
                uint8_t setup_rsp[] = {
                    0x02,
                    (uint8_t)(handle & 0xFF), (uint8_t)(((handle >> 8) & 0x0F) | 0x20),
                    8, 0x00,
                    4, 0x00,
                    (uint8_t)(s_bnep_conn.remote_cid & 0xFF), (uint8_t)(s_bnep_conn.remote_cid >> 8),
                    BNEP_CONTROL,
                    BNEP_CMD_SETUP_CONNECTION_RSP_MSG,
                    0x00, 0x01 // BNEP_RSP_INVALID_DEST_UUID
                };
                hal_BtSendPacket(setup_rsp, sizeof(setup_rsp));

                // Turn around and initiate connection as PANU requesting peer's NAP!
                s_bnep_is_incoming = FALSE;
                bnep_send_setup_req_packet();
                return;
            }

            uint8_t setup_rsp[] = {
                0x02,
                (uint8_t)(handle & 0xFF), (uint8_t)(((handle >> 8) & 0x0F) | 0x20),
                8, 0x00,                                            // ACL Length = 8
                4, 0x00,                                            // L2CAP Length = 4
                (uint8_t)(s_bnep_conn.remote_cid & 0xFF), (uint8_t)(s_bnep_conn.remote_cid >> 8),
                BNEP_CONTROL,
                BNEP_CMD_SETUP_CONNECTION_RSP_MSG,
                0x00, 0x00 // Success
            };
            hal_BtSendPacket(setup_rsp, sizeof(setup_rsp));

            if (s_bnep_conn.state != BNEP_STATE_CONNECTED)
            {
                s_bnep_conn.state = BNEP_STATE_CONNECTED;
                os_log_printf("[BNEP] ===================================================\n");
                os_log_printf("[BNEP] *** BNEP PANU <-> NAP LINK ACTIVE (Incoming)! ***\n");
                os_log_printf("[BNEP] Starting lwIP DHCP...\n");
                os_log_printf("[BNEP] ===================================================\n");
                ethernetif_bnep_link_changed(TRUE);
            }
        }
        else if (ctrl_type == BNEP_CMD_SETUP_CONNECTION_RSP_MSG)
        {
            uint16_t resp_code = (uint16_t)((data[2] << 8) | data[3]);
            if (resp_code == BNEP_RSP_SUCCESS)
            {
                if (s_bnep_conn.state != BNEP_STATE_CONNECTED)
                {
                    s_bnep_conn.state = BNEP_STATE_CONNECTED;
                    os_log_printf("[BNEP] ===================================================\n");
                    os_log_printf("[BNEP] *** BNEP PANU <-> NAP CONNECTION ESTABLISHED! ***\n");
                    os_log_printf("[BNEP] Network Tethering link active! Starting lwIP DHCP...\n");
                    os_log_printf("[BNEP] ===================================================\n");

                    ethernetif_bnep_link_changed(TRUE);
                }
            }
            else
            {
                os_log_printf("[BNEP] BNEP Setup Connection Response: Code=0x%04X\n", resp_code);
                if (s_bnep_conn.state == BNEP_STATE_CONNECTED)
                {
                    os_log_printf("[BNEP] Ignoring rejection code 0x%04X - link is already CONNECTED and active!\n", resp_code);
                }
                else
                {
                    s_bnep_conn.state = BNEP_STATE_IDLE;
                }
            }
        }
        else if (ctrl_type == BNEP_CMD_FILTER_NET_TYPE_SET_MSG)
        {
            // Acknowledge network filter request
            uint8_t flt_rsp[] = {
                0x02,
                (uint8_t)(handle & 0xFF), (uint8_t)(((handle >> 8) & 0x0F) | 0x20),
                8, 0x00,                                            // ACL Length = 8
                4, 0x00,                                            // L2CAP Length = 4
                (uint8_t)(s_bnep_conn.remote_cid & 0xFF), (uint8_t)(s_bnep_conn.remote_cid >> 8),
                BNEP_CONTROL,
                BNEP_CMD_FILTER_NET_TYPE_RSP_MSG,
                0x00, 0x00 // Success
            };
            hal_BtSendPacket(flt_rsp, sizeof(flt_rsp));
        }
        else if (ctrl_type == BNEP_CMD_FILTER_MULTI_ADDR_SET_MSG)
        {
            // Acknowledge multicast filter request
            uint8_t flt_rsp[] = {
                0x02,
                (uint8_t)(handle & 0xFF), (uint8_t)(((handle >> 8) & 0x0F) | 0x20),
                8, 0x00,                                            // ACL Length = 8
                4, 0x00,                                            // L2CAP Length = 4
                (uint8_t)(s_bnep_conn.remote_cid & 0xFF), (uint8_t)(s_bnep_conn.remote_cid >> 8),
                BNEP_CONTROL,
                BNEP_CMD_FILTER_MULTI_ADDR_RSP_MSG,
                0x00, 0x00 // Success
            };
            hal_BtSendPacket(flt_rsp, sizeof(flt_rsp));
        }
        return;
    }

    // Ethernet frame reconstruction buffer
    static uint8_t eth_reconstruct_buf[1536];
    uint16_t eth_len = 0;

    // Helper: Local MAC and Remote MAC in 802.3 network byte order (MSB first)
    uint8_t local_mac[6];
    uint8_t remote_mac[6];
    for (int i = 0; i < 6; i++) {
        local_mac[i]  = g_bt_status.bd_addr[5 - i];
        remote_mac[i] = s_bnep_conn.remote_mac[5 - i];
    }

    // 2. BNEP General Ethernet Packet (Full 14-byte Ethernet Header)
    if (pkt_type == BNEP_GENERAL_ETHERNET)
    {
        if (len >= 15)
        {
            uint16_t payload_offset = 15;
            while (has_ext && payload_offset + 2 <= len) {
                has_ext = (data[payload_offset] & 0x80) != 0;
                uint8_t ext_len = data[payload_offset + 1];
                payload_offset += 2 + ext_len;
            }
            if (payload_offset <= len)
            {
                if (payload_offset == 15)
                {
                    const uint8_t *ef = &data[1];
                    eth_len = len - 1;
                    ethernetif_bnep_input(ef, eth_len);
                    return;
                }
                else
                {
                    memcpy(eth_reconstruct_buf, &data[1], 14);
                    uint16_t plen = len - payload_offset;
                    if (14 + plen <= sizeof(eth_reconstruct_buf)) {
                        memcpy(&eth_reconstruct_buf[14], &data[payload_offset], plen);
                        eth_len = 14 + plen;
                    }
                }
            }
        }
    }
    // 3. BNEP Compressed Ethernet Packet (Omitted MACs, only 2-byte EtherType + Payload)
    else if (pkt_type == BNEP_COMPRESSED_ETHERNET)
    {
        if (len >= 3)
        {
            uint16_t payload_offset = 3;
            while (has_ext && payload_offset + 2 <= len) {
                has_ext = (data[payload_offset] & 0x80) != 0;
                uint8_t ext_len = data[payload_offset + 1];
                payload_offset += 2 + ext_len;
            }
            if (payload_offset <= len)
            {
                memcpy(&eth_reconstruct_buf[0], local_mac, 6);
                memcpy(&eth_reconstruct_buf[6], remote_mac, 6);
                eth_reconstruct_buf[12] = data[1];
                eth_reconstruct_buf[13] = data[2];
                uint16_t plen = len - payload_offset;
                if (14 + plen <= sizeof(eth_reconstruct_buf)) {
                    memcpy(&eth_reconstruct_buf[14], &data[payload_offset], plen);
                    eth_len = 14 + plen;
                }
            }
        }
    }
    // 4. BNEP Compressed Ethernet Packet - Source Only (Omitted Dest MAC, Dest = Local MAC)
    else if (pkt_type == BNEP_COMPRESSED_ETHERNET_SRC_ONLY)
    {
        if (len >= 9)
        {
            uint16_t payload_offset = 9;
            while (has_ext && payload_offset + 2 <= len) {
                has_ext = (data[payload_offset] & 0x80) != 0;
                uint8_t ext_len = data[payload_offset + 1];
                payload_offset += 2 + ext_len;
            }
            if (payload_offset <= len)
            {
                memcpy(&eth_reconstruct_buf[0], local_mac, 6);
                memcpy(&eth_reconstruct_buf[6], &data[1], 6);
                eth_reconstruct_buf[12] = data[7];
                eth_reconstruct_buf[13] = data[8];
                uint16_t plen = len - payload_offset;
                if (14 + plen <= sizeof(eth_reconstruct_buf)) {
                    memcpy(&eth_reconstruct_buf[14], &data[payload_offset], plen);
                    eth_len = 14 + plen;
                }
            }
        }
    }
    // 5. BNEP Compressed Ethernet Packet - Destination Only (Omitted Src MAC, Src = Remote MAC)
    // Android NAP sends broadcast / unicast packets using this type!
    else if (pkt_type == BNEP_COMPRESSED_ETHERNET_DEST_ONLY)
    {
        if (len >= 9)
        {
            uint16_t payload_offset = 9;
            while (has_ext && payload_offset + 2 <= len) {
                has_ext = (data[payload_offset] & 0x80) != 0;
                uint8_t ext_len = data[payload_offset + 1];
                payload_offset += 2 + ext_len;
            }
            if (payload_offset <= len)
            {
                memcpy(&eth_reconstruct_buf[0], &data[1], 6);
                memcpy(&eth_reconstruct_buf[6], remote_mac, 6);
                eth_reconstruct_buf[12] = data[7];
                eth_reconstruct_buf[13] = data[8];
                uint16_t plen = len - payload_offset;
                if (14 + plen <= sizeof(eth_reconstruct_buf)) {
                    memcpy(&eth_reconstruct_buf[14], &data[payload_offset], plen);
                    eth_len = 14 + plen;
                }
            }
        }
    }

    if (eth_len >= 14)
    {
        os_log_printf("[BT_BNEP_RX] EthFrame Len %u, Type 0x%02X%02X from %02X:%02X:%02X:%02X:%02X:%02X to %02X:%02X:%02X:%02X:%02X:%02X\n",
                      eth_len, eth_reconstruct_buf[12], eth_reconstruct_buf[13],
                      eth_reconstruct_buf[6], eth_reconstruct_buf[7], eth_reconstruct_buf[8],
                      eth_reconstruct_buf[9], eth_reconstruct_buf[10], eth_reconstruct_buf[11],
                      eth_reconstruct_buf[0], eth_reconstruct_buf[1], eth_reconstruct_buf[2],
                      eth_reconstruct_buf[3], eth_reconstruct_buf[4], eth_reconstruct_buf[5]);
        if (eth_reconstruct_buf[12] == 0x08 && eth_reconstruct_buf[13] == 0x00 && eth_len >= 34)
        {
            const uint8_t *ip = &eth_reconstruct_buf[14];
            uint8_t proto = ip[9];
            uint16_t src_p = (proto == 17 && eth_len >= 38) ? ((ip[20] << 8) | ip[21]) : 0;
            uint16_t dst_p = (proto == 17 && eth_len >= 38) ? ((ip[22] << 8) | ip[23]) : 0;
            os_log_printf("[BT_BNEP_RX]   -> IPv4 Proto %u: %u.%u.%u.%u:%u -> %u.%u.%u.%u:%u\n",
                          proto, ip[12], ip[13], ip[14], ip[15], src_p,
                          ip[16], ip[17], ip[18], ip[19], dst_p);
        }
        ethernetif_bnep_input(eth_reconstruct_buf, eth_len);
    }
}

BOOL bnep_send_ethernet_packet(const uint8_t *eth_frame, uint16_t len)
{
    if (s_bnep_conn.state != BNEP_STATE_CONNECTED) return FALSE;
    if (len == 0 || len > 1514) return FALSE;

    // Buffer for: HCI ACL Header (5B) + L2CAP Header (4B) + BNEP Type (1B) + Ethernet Frame
    static uint8_t acl_tx_buf[5 + 4 + 1 + 1514];

    uint16_t bnep_len = 1 + len;
    uint16_t l2cap_len = bnep_len;
    uint16_t total_acl_len = 4 + l2cap_len;

    acl_tx_buf[0] = 0x02; // HCI ACL Data Packet
    acl_tx_buf[1] = (uint8_t)(s_bnep_conn.acl_handle & 0xFF);
    acl_tx_buf[2] = (uint8_t)(((s_bnep_conn.acl_handle >> 8) & 0x0F) | 0x20); // Handle + PB=2
    acl_tx_buf[3] = (uint8_t)(total_acl_len & 0xFF);
    acl_tx_buf[4] = (uint8_t)(total_acl_len >> 8);

    acl_tx_buf[5] = (uint8_t)(l2cap_len & 0xFF);
    acl_tx_buf[6] = (uint8_t)(l2cap_len >> 8);
    acl_tx_buf[7] = (uint8_t)(s_bnep_conn.remote_cid & 0xFF);
    acl_tx_buf[8] = (uint8_t)(s_bnep_conn.remote_cid >> 8);

    acl_tx_buf[9] = BNEP_GENERAL_ETHERNET; // Type 0x01
    memcpy(&acl_tx_buf[10], eth_frame, len);

    s_bnep_conn.tx_packets++;
    s_bnep_conn.tx_bytes += len;

    os_log_printf("[BT_BNEP_TX] EthFrame Len %u, Type 0x%02X%02X to %02X:%02X:%02X:%02X:%02X:%02X\n",
                  len, eth_frame[12], eth_frame[13],
                  eth_frame[0], eth_frame[1], eth_frame[2], eth_frame[3], eth_frame[4], eth_frame[5]);

    hal_BtSendPacket(acl_tx_buf, 5 + 4 + bnep_len);
    return TRUE;
}

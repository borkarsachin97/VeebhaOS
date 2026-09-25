/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * lwIP Network Interface Driver for Bluetooth BNEP / PANU
 */

#include "lwip/opt.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/dhcp.h"
#include "lwip/prot/dhcp.h"
#include "lwip/dns.h"
#include "netif/etharp.h"
#include "netif/ethernet.h"

#include "bnep.h"
#include "hal_bt.h"
#include "extern.h"

extern uint32_t os_log_printf(const char *fmt, ...);
extern hal_bt_status_t g_bt_status;

static struct netif s_bnep_netif;
static BOOL s_netif_initialized = FALSE;

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    (void)netif;
    static uint8_t tx_frame_buf[1514];

    if (p->tot_len > sizeof(tx_frame_buf)) {
        return ERR_BUF;
    }

    uint16_t copied = pbuf_copy_partial(p, tx_frame_buf, p->tot_len, 0);
    if (copied != p->tot_len) {
        return ERR_BUF;
    }

    if (bnep_send_ethernet_packet(tx_frame_buf, copied)) {
        return ERR_OK;
    }

    return ERR_IF;
}

err_t ethernetif_bnep_init(struct netif *netif)
{
    netif->name[0] = 'b';
    netif->name[1] = 'n';
    netif->output = etharp_output;
    netif->linkoutput = low_level_output;
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;

    // Set MAC address (using local Bluetooth address)
    netif->hwaddr_len = 6;
    for (int i = 0; i < 6; i++) {
        netif->hwaddr[i] = g_bt_status.bd_addr[5 - i];
    }

    os_log_printf("[LWIP] BNEP Netif initialized: MAC %02X:%02X:%02X:%02X:%02X:%02X, MTU %u\n",
                  netif->hwaddr[0], netif->hwaddr[1], netif->hwaddr[2],
                  netif->hwaddr[3], netif->hwaddr[4], netif->hwaddr[5],
                  netif->mtu);

    return ERR_OK;
}

void ethernetif_bnep_input(const uint8_t *frame, uint16_t len)
{
    if (!s_netif_initialized || len == 0 || len > 1514) return;

    struct pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (!p) {
        os_log_printf("[LWIP] Dropping RX frame: pbuf_alloc failed (len=%u)\n", len);
        return;
    }

    pbuf_take(p, frame, len);

    // Pass directly to lwIP ethernet input
    if (s_bnep_netif.input(p, &s_bnep_netif) != ERR_OK) {
        pbuf_free(p);
    }
}

void ethernetif_bnep_link_changed(BOOL up)
{
    if (!s_netif_initialized) return;

    if (up)
    {
        if (netif_is_link_up(&s_bnep_netif)) return;

        for (int i = 0; i < 6; i++) {
            s_bnep_netif.hwaddr[i] = g_bt_status.bd_addr[5 - i];
        }
        os_log_printf("[LWIP] Network Link UP (MAC %02X:%02X:%02X:%02X:%02X:%02X). Starting DHCP client...\n",
                      s_bnep_netif.hwaddr[0], s_bnep_netif.hwaddr[1], s_bnep_netif.hwaddr[2],
                      s_bnep_netif.hwaddr[3], s_bnep_netif.hwaddr[4], s_bnep_netif.hwaddr[5]);
        netif_set_link_up(&s_bnep_netif);
        netif_set_up(&s_bnep_netif);
        dhcp_start(&s_bnep_netif);
    }
    else
    {
        if (!netif_is_link_up(&s_bnep_netif)) return;

        os_log_printf("[LWIP] Network Link DOWN.\n");
        dhcp_stop(&s_bnep_netif);
        netif_set_link_down(&s_bnep_netif);
        netif_set_down(&s_bnep_netif);
    }
}

void net_stack_init(void)
{
    if (s_netif_initialized) return;

    lwip_init();

    ip4_addr_t ip, mask, gw;
    IP4_ADDR(&ip, 0, 0, 0, 0);
    IP4_ADDR(&mask, 0, 0, 0, 0);
    IP4_ADDR(&gw, 0, 0, 0, 0);

    netif_add(&s_bnep_netif, &ip, &mask, &gw, NULL, ethernetif_bnep_init, ethernet_input);
    netif_set_default(&s_bnep_netif);

    s_netif_initialized = TRUE;
    os_log_printf("[LWIP] lwIP TCP/IP Stack & BNEP Netif ready!\n");
}

BOOL ethernetif_is_dhcp_bound(void)
{
    if (!s_netif_initialized) return FALSE;
    struct dhcp *d = netif_dhcp_data(&s_bnep_netif);
    return (d && d->state == DHCP_STATE_BOUND);
}

const ip4_addr_t* ethernetif_get_ip(void)
{
    if (!s_netif_initialized) return NULL;
    return netif_ip4_addr(&s_bnep_netif);
}

const ip4_addr_t* ethernetif_get_gw(void)
{
    if (!s_netif_initialized) return NULL;
    return netif_ip4_gw(&s_bnep_netif);
}

const ip4_addr_t* ethernetif_get_mask(void)
{
    if (!s_netif_initialized) return NULL;
    return netif_ip4_netmask(&s_bnep_netif);
}

const ip4_addr_t* ethernetif_get_dns(void)
{
    return dns_getserver(0);
}

u32_t sys_now(void)
{
    extern uint32_t timer_get_ms(void);
    return (u32_t)timer_get_ms();
}

int atoi(const char *s)
{
    int res = 0;
    while (*s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }
    return res;
}


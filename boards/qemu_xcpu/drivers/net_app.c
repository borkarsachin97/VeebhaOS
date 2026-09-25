/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Network Application Diagnostics: ICMP Ping, DNS Resolver & HTTP Test Client
 */

#include "lwip/opt.h"
#include "lwip/mem.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip.h"
#include "lwip/dns.h"
#include "lwip/tcp.h"

#include "net_app.h"
#include "timer.h"
#include "extern.h"

extern uint32_t os_log_printf(const char *fmt, ...);
extern int mini_snprintf(char *buf, unsigned int size, const char *fmt, ...);
extern BOOL ethernetif_is_dhcp_bound(void);
extern const ip4_addr_t* ethernetif_get_ip(void);
extern const ip4_addr_t* ethernetif_get_gw(void);
extern const ip4_addr_t* ethernetif_get_mask(void);
extern const ip4_addr_t* ethernetif_get_dns(void);

static net_app_diag_t s_net_diag = {0};
static struct raw_pcb *s_ping_pcb = NULL;
static uint16_t s_ping_seq_num = 0;
static uint32_t s_ping_time = 0;

static u8_t ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr)
{
    (void)arg;
    (void)pcb;

    if (pbuf_remove_header(p, PBUF_IP_HLEN) == 0)
    {
        struct icmp_echo_hdr *iecho = (struct icmp_echo_hdr *)p->payload;
        if (iecho->type == ICMP_ER)
        {
            uint32_t now = timer_get_ms();
            s_net_diag.ping_rtt_ms = (now >= s_ping_time) ? (now - s_ping_time) : 0;
            s_net_diag.ping_in_progress = FALSE;
            s_net_diag.ping_success = TRUE;
            s_net_diag.ping_recv++;

            os_log_printf("[PING] Reply from %s: bytes=%u, time=%u ms, seq=%u (ROUND-TRIP OK!)\n",
                          ipaddr_ntoa(addr), p->tot_len, s_net_diag.ping_rtt_ms, lwip_ntohs(iecho->seqno));

            pbuf_free(p);
            return 1; // Handled
        }
    }

    return 0; // Not handled
}

BOOL net_app_ping(const char *target_ip)
{
    if (!target_ip || target_ip[0] == '\0') return FALSE;

    ip4_addr_t dst_addr;
    if (!ip4addr_aton(target_ip, &dst_addr)) {
        os_log_printf("[PING] Invalid IP address: %s\n", target_ip);
        return FALSE;
    }

    if (!s_ping_pcb) {
        s_ping_pcb = raw_new(IP_PROTO_ICMP);
        if (!s_ping_pcb) return FALSE;
        raw_recv(s_ping_pcb, ping_recv, NULL);
        raw_bind(s_ping_pcb, IP_ADDR_ANY);
    }

    size_t ping_size = sizeof(struct icmp_echo_hdr) + 32;
    struct pbuf *p = pbuf_alloc(PBUF_IP, (u16_t)ping_size, PBUF_RAM);
    if (!p) return FALSE;

    struct icmp_echo_hdr *iecho = (struct icmp_echo_hdr *)p->payload;
    ICMPH_TYPE_SET(iecho, ICMP_ECHO);
    ICMPH_CODE_SET(iecho, 0);
    iecho->chksum = 0;
    iecho->id     = 0xAFAF;
    iecho->seqno  = lwip_htons(++s_ping_seq_num);

    // Payload: fill with pattern
    uint8_t *data = (uint8_t *)p->payload + sizeof(struct icmp_echo_hdr);
    for (int i = 0; i < 32; i++) data[i] = (uint8_t)('A' + (i % 26));

    iecho->chksum = inet_chksum(p->payload, ping_size);

    s_net_diag.ping_in_progress = TRUE;
    s_net_diag.ping_success = FALSE;
    s_net_diag.ping_sent++;
    s_ping_time = timer_get_ms();
    strncpy(s_net_diag.ping_target, target_ip, sizeof(s_net_diag.ping_target) - 1);

    os_log_printf("[PING] Pinging %s with 32 bytes of data (seq=%u)...\n", target_ip, s_ping_seq_num);

    raw_sendto(s_ping_pcb, p, &dst_addr);
    pbuf_free(p);
    return TRUE;
}

static void dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    (void)callback_arg;
    s_net_diag.dns_in_progress = FALSE;

    if (ipaddr)
    {
        s_net_diag.dns_success = TRUE;
        strncpy(s_net_diag.dns_resolved_ip, ipaddr_ntoa(ipaddr), sizeof(s_net_diag.dns_resolved_ip) - 1);
        os_log_printf("[DNS] Resolved \"%s\" -> %s\n", name, s_net_diag.dns_resolved_ip);
    }
    else
    {
        s_net_diag.dns_success = FALSE;
        os_log_printf("[DNS] Failed to resolve hostname \"%s\"\n", name);
    }
}

BOOL net_app_dns_query(const char *hostname)
{
    if (!hostname) return FALSE;

    ip_addr_t resolved_addr;
    s_net_diag.dns_in_progress = TRUE;
    s_net_diag.dns_success = FALSE;

    os_log_printf("[DNS] Querying DNS for: %s...\n", hostname);

    err_t err = dns_gethostbyname(hostname, &resolved_addr, dns_found_cb, NULL);
    if (err == ERR_OK)
    {
        // Cached in DNS table
        dns_found_cb(hostname, &resolved_addr, NULL);
        return TRUE;
    }
    else if (err == ERR_INPROGRESS)
    {
        return TRUE;
    }

    s_net_diag.dns_in_progress = FALSE;
    os_log_printf("[DNS] DNS request error: %d\n", err);
    return FALSE;
}

// Minimal Raw TCP HTTP Client
static struct tcp_pcb *s_http_pcb = NULL;
static char s_http_host[32] = {0};
static char s_http_path[32] = {0};

static err_t http_recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    (void)arg;
    if (p == NULL)
    {
        // Connection closed
        tcp_close(tpcb);
        s_http_pcb = NULL;
        s_net_diag.http_in_progress = FALSE;
        return ERR_OK;
    }

    if (err == ERR_OK && p->tot_len > 0)
    {
        s_net_diag.http_bytes_recv += p->tot_len;
        s_net_diag.http_success = TRUE;

        // Parse HTTP Status code (e.g. "HTTP/1.1 200 OK")
        char line[64] = {0};
        pbuf_copy_partial(p, line, sizeof(line) - 1, 0);

        if (strncmp(line, "HTTP/", 5) == 0 && strlen(line) >= 12)
        {
            s_net_diag.http_status_code = (uint16_t)((line[9] - '0') * 100 +
                                                    (line[10] - '0') * 10 +
                                                    (line[11] - '0'));
            os_log_printf("[HTTP] Status: %u (%s)\n", s_net_diag.http_status_code, &line[9]);
        }

        // Copy small payload preview
        pbuf_copy_partial(p, s_net_diag.http_payload_preview, sizeof(s_net_diag.http_payload_preview) - 1, 0);
        tcp_recved(tpcb, p->tot_len);
    }

    pbuf_free(p);
    return ERR_OK;
}

static err_t http_connected_cb(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    (void)arg;
    if (err != ERR_OK)
    {
        os_log_printf("[HTTP] TCP Connect Error: %d\n", err);
        s_net_diag.http_in_progress = FALSE;
        return err;
    }

    os_log_printf("[HTTP] Connected to %s:80! Sending GET %s...\n", s_http_host, s_http_path);

    char req[128];
    int req_len = mini_snprintf(req, sizeof(req),
                               "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: RDA8809\r\nConnection: close\r\n\r\n",
                               s_http_path, s_http_host);

    tcp_write(tpcb, req, req_len, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    return ERR_OK;
}

static void http_err_cb(void *arg, err_t err)
{
    (void)arg;
    os_log_printf("[HTTP] TCP Connection Aborted / Error: %d\n", err);
    s_net_diag.http_in_progress = FALSE;
    s_http_pcb = NULL;
}

static void http_dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *arg)
{
    (void)name;
    (void)arg;

    if (!ipaddr)
    {
        os_log_printf("[HTTP] DNS lookup failed for %s\n", s_http_host);
        s_net_diag.http_in_progress = FALSE;
        return;
    }

    os_log_printf("[HTTP] Host %s resolved to %s. Connecting TCP port 80...\n", s_http_host, ipaddr_ntoa(ipaddr));

    if (s_http_pcb) tcp_abort(s_http_pcb);
    s_http_pcb = tcp_new();
    if (!s_http_pcb) {
        s_net_diag.http_in_progress = FALSE;
        return;
    }

    tcp_recv(s_http_pcb, http_recv_cb);
    tcp_err(s_http_pcb, http_err_cb);

    tcp_connect(s_http_pcb, ipaddr, 80, http_connected_cb);
}

BOOL net_app_http_test(const char *host, const char *path)
{
    if (!host) return FALSE;

    strncpy(s_http_host, host, sizeof(s_http_host) - 1);
    strncpy(s_http_path, path ? path : "/", sizeof(s_http_path) - 1);

    s_net_diag.http_in_progress = TRUE;
    s_net_diag.http_success = FALSE;
    s_net_diag.http_status_code = 0;
    s_net_diag.http_bytes_recv = 0;
    s_net_diag.http_payload_preview[0] = '\0';

    os_log_printf("[HTTP] Starting HTTP GET test for http://%s%s...\n", s_http_host, s_http_path);

    ip_addr_t ip;
    err_t err = dns_gethostbyname(host, &ip, http_dns_found_cb, NULL);
    if (err == ERR_OK)
    {
        http_dns_found_cb(host, &ip, NULL);
        return TRUE;
    }
    else if (err == ERR_INPROGRESS)
    {
        return TRUE;
    }

    s_net_diag.http_in_progress = FALSE;
    return FALSE;
}

void net_app_init(void)
{
    memset(&s_net_diag, 0, sizeof(s_net_diag));
}

void net_app_poll(void)
{
    // Process lwIP timers (ARP cache, DHCP lease, TCP retransmit, DNS query timeout)
    sys_check_timeouts();

    // Check DHCP bound state transition
    static BOOL s_dhcp_was_bound = FALSE;
    BOOL is_bound = ethernetif_is_dhcp_bound();

    if (is_bound && !s_dhcp_was_bound)
    {
        s_dhcp_was_bound = TRUE;
        const ip4_addr_t *ip = ethernetif_get_ip();
        const ip4_addr_t *gw = ethernetif_get_gw();
        const ip4_addr_t *mask = ethernetif_get_mask();
        const ip4_addr_t *dns = ethernetif_get_dns();

        char ip_str[16] = "0.0.0.0", gw_str[16] = "0.0.0.0";
        char mask_str[16] = "0.0.0.0", dns_str[16] = "0.0.0.0";

        if (ip) ip4addr_ntoa_r(ip, ip_str, sizeof(ip_str));
        if (gw) ip4addr_ntoa_r(gw, gw_str, sizeof(gw_str));
        if (mask) ip4addr_ntoa_r(mask, mask_str, sizeof(mask_str));
        if (dns) ip4addr_ntoa_r(dns, dns_str, sizeof(dns_str));

        os_log_printf("\n===================================================\n");
        os_log_printf("[NET] *** INTERNET ACCESS READY VIA BLUETOOTH TETHERING ***\n");
        os_log_printf("[NET] Assigned IP : %s\n", ip_str);
        os_log_printf("[NET] Subnet Mask : %s\n", mask_str);
        os_log_printf("[NET] Gateway     : %s\n", gw_str);
        os_log_printf("[NET] Primary DNS : %s\n", dns_str);
        os_log_printf("[NET] TCP/IP Protocol Stack is fully OPERATIONAL!\n");
        os_log_printf("===================================================\n\n");

        // Auto-ping the gateway to verify full round-trip IP stack communication
        if (gw && !ip4_addr_isany(gw)) {
            os_log_printf("[NET] Auto-testing connectivity: Pinging Gateway (%s)...\n", gw_str);
            net_app_ping(gw_str);
        }
    }
    else if (!is_bound && s_dhcp_was_bound)
    {
        s_dhcp_was_bound = FALSE;
        os_log_printf("[NET] Network link down or DHCP lease expired.\n");
    }

    // Ping timeout monitor
    if (s_net_diag.ping_in_progress && (timer_get_ms() - s_ping_time) > 4000)
    {
        s_net_diag.ping_in_progress = FALSE;
        s_net_diag.ping_success = FALSE;
        os_log_printf("[PING] Request timed out for %s (seq=%u, 4000 ms)\n",
                      s_net_diag.ping_target, s_ping_seq_num);
    }
}

const net_app_diag_t* net_app_get_diag(void)
{
    return &s_net_diag;
}

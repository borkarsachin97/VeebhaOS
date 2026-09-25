#ifndef _NET_APP_H_
#define _NET_APP_H_

#include "cs_types.h"

typedef struct {
    BOOL     ping_in_progress;
    BOOL     ping_success;
    uint32_t ping_rtt_ms;
    uint32_t ping_sent;
    uint32_t ping_recv;
    char     ping_target[16];

    BOOL     http_in_progress;
    BOOL     http_success;
    uint16_t http_status_code;
    uint32_t http_bytes_recv;
    char     http_payload_preview[32];

    BOOL     dns_in_progress;
    BOOL     dns_success;
    char     dns_resolved_ip[16];
} net_app_diag_t;

void net_app_init(void);
void net_app_poll(void);

BOOL net_app_ping(const char *target_ip);
BOOL net_app_dns_query(const char *hostname);
BOOL net_app_http_test(const char *host, const char *path);

const net_app_diag_t* net_app_get_diag(void);

#endif // _NET_APP_H_

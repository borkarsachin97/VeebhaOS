#ifndef _ETHERNETIF_BNEP_H_
#define _ETHERNETIF_BNEP_H_

#include "cs_types.h"
#include "lwip/ip4_addr.h"

void net_stack_init(void);
BOOL ethernetif_is_dhcp_bound(void);
const ip4_addr_t* ethernetif_get_ip(void);
const ip4_addr_t* ethernetif_get_gw(void);
const ip4_addr_t* ethernetif_get_mask(void);
const ip4_addr_t* ethernetif_get_dns(void);

#endif // _ETHERNETIF_BNEP_H_

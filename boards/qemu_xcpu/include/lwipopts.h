#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

// Operating Mode: NO_SYS = 1 (Lightweight callback / polled mode, zero OS semaphore/queue overhead)
#define NO_SYS                          1
#define SYS_LIGHTWEIGHT_PROT            0
#define LWIP_NETCONN                    0
#define LWIP_SOCKET                     0

// Memory Configuration (Optimized for 114KB available FreeRTOS heap)
#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        (16 * 1024)   // 16 KB heap for pbuf and connections
#define MEMP_NUM_PBUF                   16
#define MEMP_NUM_RAW_PCB                4
#define MEMP_NUM_UDP_PCB                4
#define MEMP_NUM_TCP_PCB                4
#define MEMP_NUM_TCP_PCB_LISTEN         2
#define MEMP_NUM_TCP_SEG                16
#define MEMP_NUM_SYS_TIMEOUT            8

// Packet Buffer Configuration
#define PBUF_POOL_SIZE                  16
#define PBUF_POOL_BUFSIZE               1536          // Full Ethernet frame MTU

// Protocols
#define LWIP_ARP                        1
#define LWIP_ETHERNET                   1
#define LWIP_IPV4                       1
#define LWIP_ICMP                       1
#define LWIP_RAW                        1
#define LWIP_DHCP                       1
#define LWIP_AUTOIP                     0
#define LWIP_SNMP                       0
#define LWIP_IGMP                       0
#define LWIP_DNS                        1
#define LWIP_UDP                        1
#define LWIP_TCP                        1

// TCP Options
#define TCP_MSS                         1460
#define TCP_WND                         (4 * TCP_MSS)
#define TCP_SND_BUF                     (4 * TCP_MSS)
#define TCP_QUEUE_OOSEQ                 0

// Checksums handled in software
#define CHECKSUM_GEN_IP                 1
#define CHECKSUM_GEN_UDP                1
#define CHECKSUM_GEN_TCP                1
#define CHECKSUM_GEN_ICMP               1
#define CHECKSUM_CHECK_IP               1
#define CHECKSUM_CHECK_UDP              1
#define CHECKSUM_CHECK_TCP              1
#define CHECKSUM_CHECK_ICMP             1

// Statistics and Debug
#define LWIP_STATS                      0
#define LWIP_DEBUG                      0

#endif /* __LWIPOPTS_H__ */

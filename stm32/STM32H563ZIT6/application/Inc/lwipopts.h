#ifndef LWIPOPTS_H
#define LWIPOPTS_H

/* =========================
 * SYSTEM
 * ========================= */

#define NO_SYS                          1

#define SYS_LIGHTWEIGHT_PROT            0

#define LWIP_NETCONN                    0
#define LWIP_SOCKET                     0


/* =========================
 * MEMORY
 * ========================= */

#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        (16U * 1024U)

#define MEMP_NUM_PBUF                   16
#define PBUF_POOL_SIZE                  16
#define PBUF_POOL_BUFSIZE               1536
#define IP_DEFAULT_TTL 64

/* =========================
 * ETHERNET / IPv4
 * ========================= */

#define LWIP_IPV4                       1
#define LWIP_ETHERNET                   1
#define LWIP_ARP                        1

#define ETH_PAD_SIZE                    0

#define ARP_TABLE_SIZE                  10
#define ARP_QUEUEING                    1


/* =========================
 * ICMP
 * ========================= */

#define LWIP_ICMP                       1


/* =========================
 * UDP / TCP
 * ========================= */

#define LWIP_UDP                        1
#define LWIP_TCP                        1


/* =========================
 * DHCP / AUTOIP
 * Na razie statyczny adres
 * ========================= */

#define LWIP_DHCP                       0
#define LWIP_AUTOIP                     0


/* =========================
 * CHECKSUM
 *
 * Na razie liczy lwIP.
 * Nie używamy jeszcze HW checksum offload.
 * ========================= */

#define CHECKSUM_GEN_IP                 1
#define CHECKSUM_GEN_UDP                1
#define CHECKSUM_GEN_TCP                1
#define CHECKSUM_GEN_ICMP               1

#define CHECKSUM_CHECK_IP               1
#define CHECKSUM_CHECK_UDP              1
#define CHECKSUM_CHECK_TCP              1
#define CHECKSUM_CHECK_ICMP             1


/* =========================
 * NETIF
 * ========================= */

#define LWIP_NETIF_STATUS_CALLBACK      0
#define LWIP_NETIF_LINK_CALLBACK        0

#define LWIP_NETIF_HOSTNAME             0


/* =========================
 * STATISTICS / DEBUG
 * Na start wyłączone
 * ========================= */

#define LWIP_STATS                      0
#define LWIP_DEBUG                      0


#endif /* LWIPOPTS_H */
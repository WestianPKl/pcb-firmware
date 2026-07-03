
#ifndef __LWIPOPTS__H__
#define __LWIPOPTS__H__

#ifdef __cplusplus
 extern "C" {
#endif

#define WITH_RTOS 1
#define CHECKSUM_BY_HARDWARE 1
#define LWIP_DHCP 1
#define NO_SYS 0
#define SYS_LIGHTWEIGHT_PROT 0
#define MEM_ALIGNMENT 4
#define LWIP_RAM_HEAP_POINTER 0x20048000
#define MEMP_NUM_SYS_TIMEOUT 8
#define LWIP_ETHERNET 1
#define LWIP_DNS_SECURE 7
#define MEM_SIZE                (10 * 1024)
#define PBUF_POOL_SIZE          8
#define TCP_MSS                 1460
#define TCP_SND_BUF             (4 * TCP_MSS)
#define TCP_WND                 (4 * TCP_MSS)
#define LWIP_ICMP               1
#define LWIP_DHCP               1
#define LWIP_NETIF_STATUS_CALLBACK 1   
#define TCP_SND_QUEUELEN 9
#define TCP_SNDLOWAT 1071
#define TCP_SNDQUEUELOWAT 5
#define TCP_WND_UPDATE_THRESHOLD 536
#define LWIP_NETIF_LINK_CALLBACK 1
#define LWIP_NETCONN 1
#define LWIP_SOCKET 1
#define LWIP_ERRNO_STDINCLUDE 1
#define RECV_BUFSIZE_DEFAULT 2000000000
#define LWIP_STATS 0
#define CHECKSUM_GEN_IP 0
#define CHECKSUM_GEN_UDP 0
#define CHECKSUM_GEN_TCP 0
#define CHECKSUM_GEN_ICMP 0
#define CHECKSUM_GEN_ICMP6 0
#define CHECKSUM_CHECK_IP 0
#define CHECKSUM_CHECK_UDP 0
#define CHECKSUM_CHECK_TCP 0
#define CHECKSUM_CHECK_ICMP 0
#define CHECKSUM_CHECK_ICMP6 0
#define TCPIP_THREAD_NAME          "tcpip"
#define TCPIP_THREAD_STACKSIZE     1024
#define TCPIP_THREAD_PRIO          5
#define TCPIP_MBOX_SIZE            8
#define DEFAULT_THREAD_STACKSIZE   1024
#define DEFAULT_RAW_RECVMBOX_SIZE  8
#define DEFAULT_UDP_RECVMBOX_SIZE  8
#define DEFAULT_TCP_RECVMBOX_SIZE  8
#define DEFAULT_ACCEPTMBOX_SIZE    8
#define LWIP_ARP                       1
#define ETHARP_SUPPORT_STATIC_ENTRIES 1

#ifdef __cplusplus
}
#endif
#endif /*__LWIPOPTS__H__ */

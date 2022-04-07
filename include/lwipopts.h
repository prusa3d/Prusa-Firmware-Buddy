#ifndef __LWIPOPTS__H__
#define __LWIPOPTS__H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define WITH_RTOS            1
#define CHECKSUM_BY_HARDWARE 0

#define LWIP_DHCP            1
#define MEM_ALIGNMENT        4
#define MEMP_NUM_SYS_TIMEOUT 7
#define LWIP_ETHERNET        1
#define LWIP_DNS_SECURE      7

#define TCP_MSS                536
#define TCP_WND                (2 * TCP_MSS)
#define TCPIP_THREAD_STACKSIZE 2048

#define TCPIP_MBOX_SIZE 6

#define DEFAULT_UDP_RECVMBOX_SIZE TCPIP_MBOX_SIZE
#define DEFAULT_TCP_RECVMBOX_SIZE TCPIP_MBOX_SIZE
#define DEFAULT_ACCEPTMBOX_SIZE   TCPIP_MBOX_SIZE
#define RECV_BUFSIZE_DEFAULT      2000000000
#define LWIP_HTTPD                0
#define LWIP_STATS                0
#define CHECKSUM_GEN_IP           1
#define CHECKSUM_GEN_UDP          1
#define CHECKSUM_GEN_TCP          1
#define CHECKSUM_GEN_ICMP         1
#define CHECKSUM_GEN_ICMP6        1
#define CHECKSUM_CHECK_IP         1
#define CHECKSUM_CHECK_UDP        1
#define CHECKSUM_CHECK_TCP        1
#define CHECKSUM_CHECK_ICMP       1
#define CHECKSUM_CHECK_ICMP6      1

#define HTTPD_USE_CUSTOM_FSDATA 0

/*
 * FIXME:
 * Workaround:
 *
 * We observed a very weird bug where, while sending a "file", the packets
 * either got reordered (1, 3, 2), which makes a mess of all the TCP
 * congestion algorithms or, when the other side ACKs twice the first
 * packet (to hint that the second one is missing while seeing the third)
 * before the second one goes out, we never actually send the second one.
 * Finding the real cause in either LwIP or our integration of that will be
 * *fun*.
 *
 * As a temporary workaround to deliver the damn file and not require F5 3
 * times to get the pages working, we make sure to send only a single
 * packet at a time. That way it won't reorder. This gives us slight
 * performance degradation, but that doesn't matter with the small web page
 * and makes it actually usable.
 *
 * #BFW-2357
 */
#define LWIP_NETIF_API               1 // enable LWIP_NETIF_API==1: Support netif api (in netifapi.c)
#define LWIP_NETIF_LINK_CALLBACK     1 // Support a callback function from an interface whenever the link changes (i.e., link down)
#define LWIP_NETIF_STATUS_CALLBACK   1 // Support a callback function whenever an interface changes its up/down status (i.e., due to DHCP IP acquisition)
#define LWIP_HTTPD_DYNAMIC_HEADERS   0
#define LWIP_SINGLE_NETIF            0
#define LWIP_NETIF_HOSTNAME          1
#define LWIP_HTTPD_SUPPORT_POST      0
#define LWIP_COMPAT_SOCKETS          1
#define LWIP_ALTCP                   0
#define LWIP_HTTPD_DYNAMIC_FILE_READ 0
#define LWIP_TIMERS                  1

#define LWIP_DNS 1

/*
 * We have a HTTP server (PrusaLink). The browsers tend to keep few
 * connections at ready and take up the slots. In general it works better
 * if we have more slots available - if there are too few of them, it
 * sometimes tends to refuse connections, which leads to user pressing the
 * F5, ...
 *
 * We pay about 200B per slot, so this can be tuned as needed.
 *
 * Note, these are shared among ethernet and wifi connectins.
 */
#define MEMP_NUM_TCP_PCB 12
#define SO_REUSE         1 // Allow SOF_REUSEADDR to do something useful.

#ifdef __cplusplus
}
#endif
#endif /*__LWIPOPTS__H__ */

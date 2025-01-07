#ifndef __LWIPOPTS__H__
#define __LWIPOPTS__H__

#include "printers.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <option/mdns.h>

#define WITH_RTOS            1
#define MEM_LIBC_MALLOC      0
#define CHECKSUM_BY_HARDWARE 0
#define LWIP_DHCP            1
#define MEM_ALIGNMENT        4
#define LWIP_ETHERNET        1
#define LWIP_DNS_SECURE      7
#define DNS_MAX_NAME_LENGTH  128
#define LWIP_RAW             1

#if MDNS()
    #define MDNS_MAX_STORED_PKTS 1
    // Each interface takes up to 6 timeouts (in addition to the packets).
    // One is the "main" one, the other 5 deal with some delayed answering (and
    // probably can be lowered, a lot of these could come in sequence instead
    // of starting them in parallel).
    //
    // We limit ourselves to only one active interface and don't do mdns on the
    // other.
    #define MDNS_EXTRA_TIMEOUTS 6 + MDNS_MAX_STORED_PKTS
    #define LWIP_MDNS_RESPONDER 1
    // For MDNS
    #define LWIP_IGMP                      1
    #define LWIP_NUM_NETIF_CLIENT_DATA     1
    #define LWIP_NETIF_EXT_STATUS_CALLBACK 1
#else
    // No extra timeouts if no MDNS
    #define MDNS_EXTRA_TIMEOUTS 0
#endif
#define MEMP_NUM_SYS_TIMEOUT 9 + MDNS_EXTRA_TIMEOUTS

#define TCP_MSS        1024
#define TCP_WND        (8 * TCP_MSS)
#define TCP_SND_BUF    (2 * TCP_MSS)
#define LWIP_WND_SCALE 0
#define TCP_RCV_SCALE  0
// Note: We replaced the use of this pool by our own mechanism.
#define PBUF_POOL_SIZE                 0
#define LWIP_DISABLE_TCP_SANITY_CHECKS 1
#define IP_REASS_MAX_PBUFS             15
#define TCPIP_THREAD_STACKSIZE         1248
// Before moving to our own allocator, we had 10 "big" pbufs and 12 "small"
// ones and this summed up into this. Now we are a bit more flexible and under
// some circumstances can produce more pbufs, so we add a little bit of
// additional headroom.
#define TCPIP_MBOX_SIZE 30

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
#define LWIP_CHECKSUM_ON_COPY     1
#define HTTPD_USE_CUSTOM_FSDATA   0

#include "buddy/priorities_config.h"
#define TCPIP_THREAD_PRIO TASK_PRIORITY_TCPIP_THREAD

#define MEM_USE_POOLS         1
#define MEMP_USE_CUSTOM_POOLS 1

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
#define LWIP_COMPAT_SOCKETS          0
#define LWIP_ALTCP                   0
#define LWIP_HTTPD_DYNAMIC_FILE_READ 0
#define LWIP_TIMERS                  1
#define LWIP_SO_RCVTIMEO             1
#define LWIP_SO_SNDTIMEO             1

// Some attempts to "tune" it to use less memory in unstable network environment with many retries of new connections.
#define LWIP_TCP_CLOSE_TIMEOUT_MS_DEFAULT 5000 /* 5s for closing a connection must be enough... or let the other side time out */
#define TCP_OVERSIZE                      128
#define LWIP_TCP_SACK_OUT                 1
#define TCP_OOSEQ_MAX_PBUFS               3

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

#define MEMP_NUM_UDP_PCB 5

#define MEMP_NUM_TCPIP_MSG_INPKT TCPIP_MBOX_SIZE

#define LWIP_ASSERT_CORE_LOCKED()                  \
    do {                                           \
        extern void wui_lwip_assert_core_locked(); \
        wui_lwip_assert_core_locked();             \
    } while (0)

#ifdef __cplusplus
}
#endif
#endif /*__LWIPOPTS__H__ */

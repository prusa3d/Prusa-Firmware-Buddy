#ifndef __LWIPOPTS__H__
#define __LWIPOPTS__H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define WITH_RTOS 0

#define CHECKSUM_BY_HARDWARE 0
#define LWIP_DHCP            0
#define MEM_ALIGNMENT        8
#define MEMP_NUM_SYS_TIMEOUT 6
#define LWIP_ETHERNET        0
#define LWIP_DNS_SECURE      7

#define TCP_MSS                536
#define TCP_WND                (2 * TCP_MSS)
#define TCPIP_THREAD_STACKSIZE 2048

#define TCPIP_MBOX_SIZE 6

#define DEFAULT_UDP_RECVMBOX_SIZE    TCPIP_MBOX_SIZE
#define DEFAULT_TCP_RECVMBOX_SIZE    TCPIP_MBOX_SIZE
#define DEFAULT_ACCEPTMBOX_SIZE      TCPIP_MBOX_SIZE
#define RECV_BUFSIZE_DEFAULT         2000000000
#define LWIP_HTTPD                   1
#define LWIP_STATS                   0
#define CHECKSUM_GEN_IP              0
#define CHECKSUM_GEN_UDP             0
#define CHECKSUM_GEN_TCP             0
#define CHECKSUM_GEN_ICMP            0
#define CHECKSUM_GEN_ICMP6           0
#define CHECKSUM_CHECK_IP            0
#define CHECKSUM_CHECK_UDP           0
#define CHECKSUM_CHECK_TCP           0
#define CHECKSUM_CHECK_ICMP          0
#define CHECKSUM_CHECK_ICMP6         0
#define HTTPD_USE_CUSTOM_FSDATA      1 // uses the web resources from fsdata_custom.c (buddy web pages)
#define LWIP_NETIF_API               1 // enable LWIP_NETIF_API==1: Support netif api (in netifapi.c)
#define LWIP_NETIF_LINK_CALLBACK     1 // Support a callback function from an interface whenever the link changes (i.e., link down)
#define LWIP_NETIF_STATUS_CALLBACK   1 // Support a callback function whenever an interface changes its up/down status (i.e., due to DHCP IP acquisition)
#define LWIP_HTTPD_DYNAMIC_HEADERS   1
#define LWIP_SINGLE_NETIF            1
#define LWIP_NETIF_HOSTNAME          1
#define LWIP_HTTPD_SUPPORT_POST      1
#define LWIP_COMPAT_SOCKETS          0
#define LWIP_ALTCP                   1
#define LWIP_HTTPD_DYNAMIC_FILE_READ 1

#define HTTPD_SERVER_AGENT "Prusa Mini"
#define LWIP_DNS           1
#define MEMP_NUM_TCP_PCB   6
#define SO_REUSE           1 // Allow SOF_REUSEADDR to do something useful.

// Specifics for tests.
#define MEM_LIBC_MALLOC     1
#define MEMP_MEM_MALLOC     1
#define LWIP_MPU_COMPATIBLE 1

#define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS

#ifdef __cplusplus
}
#endif
#endif /*__LWIPOPTS__H__ */

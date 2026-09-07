#include "sntp.h"
#include "sntp_client.h"
#include "netdev.h"
#include <lwip/tcpip.h>

#include <option/has_esp.h>

static uint32_t sntp_running = 0; // describes if sntp is currently running or not
void sntp_client_init(void) {
    sntp_setoperatingmode(SNTP_OPMODE_POLL);

    sntp_init();
}

void sntp_client_step(void) {
    bool netif_up = netdev_get_status(NETDEV_ETH_ID) == NETDEV_NETIF_UP;
#if HAS_ESP()
    netif_up |= netdev_get_status(NETDEV_ESP_ID) == NETDEV_NETIF_UP;
#endif

    if (!sntp_running && netif_up) {
        LOCK_TCPIP_CORE();
        sntp_client_init();
        UNLOCK_TCPIP_CORE();
        sntp_running = 1;
    } else if (sntp_running && !netif_up) {
        LOCK_TCPIP_CORE();
        sntp_stop();
        UNLOCK_TCPIP_CORE();
        sntp_running = 0;
    }
}

#include "sntp.h"
#include "sntp_client.h"
#include "netdev.h"
#include <lwip/tcpip.h>

#include <option/has_esp.h>

static uint32_t sntp_running = 0; // describes if sntp is currently running or not
void sntp_client_init(const char *ntp_address, bool dhcp) {
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_servermode_dhcp(dhcp);
    sntp_setservername(dhcp, ntp_address);
    sntp_init();
}

void sntp_client_step(bool ntp_via_dhcp, const char *ntp_address) {
    bool netif_up = netdev_get_status(NETDEV_ETH_ID) == NETDEV_NETIF_UP;
#if HAS_ESP()
    netif_up |= netdev_get_status(NETDEV_ESP_ID) == NETDEV_NETIF_UP;
#endif
    if (!sntp_running && netif_up) {
        LOCK_TCPIP_CORE();
        sntp_client_init(ntp_address, ntp_via_dhcp);
        UNLOCK_TCPIP_CORE();
        sntp_running = 1;
    } else if (sntp_running && !netif_up) {
        LOCK_TCPIP_CORE();
        sntp_stop();
        UNLOCK_TCPIP_CORE();
        sntp_running = 0;
    }
}

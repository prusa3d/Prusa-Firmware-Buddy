#include "sntp.h"
#include "sntp_client.h"
#include "netdev.h"
#include "tcpip.h"
#include "netif.h"
#include "netdb.h"

static uint32_t sntp_running = 0; // describes if sntp is currently running or not
void sntp_client_static_init(const char *ntp_address) {
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_servermode_dhcp(0);
    sntp_setservername(0, ntp_address);
    sntp_init();
}

static void sntp_client_dhcp_init(const char *ntp_address) {
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_servermode_dhcp(1);
    sntp_setservername(1, ntp_address);
    sntp_init();
}

void sntp_client_step(bool ntp_via_dhcp, const char *ntp_address) {
    netdev_status_t eth = netdev_get_status(NETDEV_ETH_ID);
    netdev_status_t wifi = netdev_get_status(NETDEV_ESP_ID);

    if (!sntp_running && (eth == NETDEV_NETIF_UP || wifi == NETDEV_NETIF_UP)) {
        LOCK_TCPIP_CORE();
        if (ntp_via_dhcp) {
            sntp_client_dhcp_init(ntp_address);
        } else {
            sntp_client_static_init(ntp_address);
        }

        UNLOCK_TCPIP_CORE();
        sntp_running = 1;
    } else if (sntp_running && eth != NETDEV_NETIF_UP && wifi != NETDEV_NETIF_UP) {
        LOCK_TCPIP_CORE();
        sntp_stop();
        UNLOCK_TCPIP_CORE();
        sntp_running = 0;
    }
}

void sntp_client_stop() {
    if (sntp_running) {
        sntp_stop();
        sntp_running = 0;
    }
}

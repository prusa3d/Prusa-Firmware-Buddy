#include "sntp.h"
#include "sntp_client.h"
#include "netdev.h"

static ip_addr_t ntp_server;      // testing ntp server located in Prague
static uint32_t sntp_running = 0; // describes if sntp is currently running or not
void sntp_client_init(void) {
    sntp_setoperatingmode(SNTP_OPMODE_POLL);

    /* TODO: enable DNS for ntp.pool.org as default sntp server*/

    // TMP: ip of Czech CESNET NTP server tak.cesnet.cz
    if (ipaddr_aton("195.113.144.238", &ntp_server)) {
        sntp_setserver(0, &ntp_server);
    }
    sntp_init();
}

void sntp_client_step(void) {
    if (!sntp_running && netdev_get_status(NETDEV_ETH_ID) == NETDEV_NETIF_UP) {
        sntp_client_init();
        sntp_running = 1;
    } else if (sntp_running && netdev_get_status(NETDEV_ETH_ID) != NETDEV_NETIF_UP) {
        sntp_stop();
        sntp_running = 0;
    }
}

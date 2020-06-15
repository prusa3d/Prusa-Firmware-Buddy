#include "sntp.h"
#include "sntp_client.h"
#include "wui_api.h"

static ip_addr_t ntp_server;      // testing ntp server located in Prague
static bool sntp_running = false; // describes if sntp is currently running or not
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
    if (!sntp_running && get_eth_status() == ETH_NETIF_UP) {
        sntp_client_init();
        sntp_running = true;
    } else if (sntp_running && get_eth_status() != ETH_NETIF_UP) {
        sntp_stop();
        sntp_running = false;
    }
}

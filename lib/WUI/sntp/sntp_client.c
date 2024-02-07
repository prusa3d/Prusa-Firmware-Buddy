#include "sntp.h"
#include "sntp_client.h"
#include "netdev.h"

static uint32_t sntp_running = 0; // describes if sntp is currently running or not
void sntp_client_init(void) {
    sntp_setoperatingmode(SNTP_OPMODE_POLL);

    sntp_init();
}

void sntp_client_step(void) {
    netdev_status_t eth = netdev_get_status(NETDEV_ETH_ID);
    netdev_status_t wifi = netdev_get_status(NETDEV_ESP_ID);

    if (!sntp_running && (eth == NETDEV_NETIF_UP || wifi == NETDEV_NETIF_UP)) {
        sntp_client_init();
        sntp_running = 1;
    } else if (sntp_running && eth != NETDEV_NETIF_UP && wifi != NETDEV_NETIF_UP) {
        sntp_stop();
        sntp_running = 0;
    }
}

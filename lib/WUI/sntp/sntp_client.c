#include "sntp.h"
#include "sntp_client.h"
#include "netdev.h"
#include "wui_api.h"
#include <lwip/tcpip.h>

#include <string.h>

#include <option/has_esp.h>

// The server name registered with lwIP -- a private copy, not a pointer into
// the config store. sntp_setservername() stores the pointer it is given, and
// the config store rewrites its buffer in place on a settings reload, which
// would both hand lwIP a name that changes under an in-flight query and make
// the comparison in sntp_client_step() compare the buffer against itself,
// never seeing a change. Empty means the compiled-in default.
static char sntp_applied_server[NTP_SERVER_LEN + 1] = "";
static bool sntp_running = false;

// To be called with the tcpip core lock held.
static void sntp_client_start(void) {
    // Stop first, and before sntp_setoperatingmode(), which asserts that the
    // client is not running. This is a no-op on the first call, when lwIP has
    // not allocated the UDP control block yet, and on a restart it is what
    // makes the restart real: sntp_init() short-circuits when that block
    // already exists, so without the stop the server name would be re-applied
    // but no request would ever be scheduled.
    sntp_stop();

    sntp_setoperatingmode(SNTP_OPMODE_POLL);

    sntp_init();

    // sntp_init() resets server 0 to the compiled-in default
    // (SNTP_SERVER_ADDRESS), so the override must be applied after it.
    if (sntp_applied_server[0] != '\0') {
        sntp_setservername(0, sntp_applied_server);
    }
}

void sntp_client_step(void) {
    bool netif_up = netdev_get_status(NETDEV_ETH_ID) == NETDEV_NETIF_UP;
#if HAS_ESP()
    netif_up |= netdev_get_status(NETDEV_ESP_ID) == NETDEV_NETIF_UP;
#endif

    if (!netif_up) {
        if (sntp_running) {
            LOCK_TCPIP_CORE();
            sntp_stop();
            UNLOCK_TCPIP_CORE();
            sntp_running = false;
        }
        return;
    }

    const char *configured = wui_get_ntp_server();
    if (configured == NULL) {
        configured = "";
    }

    if (sntp_running && strcmp(configured, sntp_applied_server) == 0) {
        return;
    }

    LOCK_TCPIP_CORE();
    // The copy itself must happen under the lock too: lwIP still holds a
    // pointer to this same buffer from the previous start, and the tcpip
    // thread may be reading it in an in-flight request. The lock excludes
    // that reader while the buffer changes; the restart below re-registers it.
    strlcpy(sntp_applied_server, configured, sizeof(sntp_applied_server));
    sntp_client_start();
    UNLOCK_TCPIP_CORE();
    sntp_running = true;
}

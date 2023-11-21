#include "../gcode.h"
#include "wui_api.h"
#include "netdev.h"
#include "netif_settings.h"

/** \addtogroup G-Codes
 * @{
 */

/**
 * M46: Reports the assigned IP address to serial port
 *
 * ## Parameters
 *
 * - `M` - Also print out MAC address to serial port
 */

void GcodeSuite::M46() {
    lan_t ethconfig = {};
    netdev_get_ipv4_addresses(netdev_get_active_id(), &ethconfig);
    uint8_t *ipp = (uint8_t *)&ethconfig.addr_ip4;
    char ip4[17] = { 0 };
    sprintf(ip4, "%u.%u.%u.%u\n", ipp[0], ipp[1], ipp[2], ipp[3]);
    serialprintPGM(ip4);

    if (parser.seen('M')) {
        mac_address_t mac;
        get_MAC_address(&mac, netdev_get_active_id());
        serialprintPGM(mac);
    }
}

/** @}*/

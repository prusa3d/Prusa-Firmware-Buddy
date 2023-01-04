/**
 * @file selftest_netstatus_interface.cpp
 */
#include "selftest_netstatus_interface.hpp"
#include "netdev.h"
#include "eeprom.h"
#include "selftest_eeprom.hpp"
#include "selftest_log.hpp"

LOG_COMPONENT_REF(Selftest);

static TestResultNet_t convert(netdev_status_t status) {
    switch (status) {
    case NETDEV_UNLINKED:
        return TestResultNet_t::Unlinked;
    case NETDEV_NETIF_DOWN:
        return TestResultNet_t::Down;
    case NETDEV_NETIF_NOADDR:
        return TestResultNet_t::NoAddress;
    case NETDEV_NETIF_UP:
        return TestResultNet_t::Up;
    default:
        break;
    }
    return TestResultNet_t::Unlinked; // did not use Unknown, because it means test did not run
}

static const char *to_string(netdev_status_t status) {
    switch (status) {
    case NETDEV_UNLINKED:
        return "Unlinked";
    case NETDEV_NETIF_DOWN:
        return "Down";
    case NETDEV_NETIF_NOADDR:
        return "NoAddress";
    case NETDEV_NETIF_UP:
        return "Up";
    default:
        break;
    }
    return "ERROR";
}

namespace selftest {
void phaseNetStatus() {
    netdev_status_t eth = netdev_get_status(NETDEV_ETH_ID);
    netdev_status_t wifi = netdev_get_status(NETDEV_ESP_ID);

    SelftestResultEEprom_t eeres;
    eeres.ui32 = variant8_get_ui32(eeprom_get_var(EEVAR_SELFTEST_RESULT));
    eeres.eth = uint8_t(convert(eth));
    eeres.wifi = uint8_t(convert(wifi));
    eeprom_set_var(EEVAR_SELFTEST_RESULT, variant8_ui32(eeres.ui32));

    log_info(Selftest, "Eth %s", to_string(eth));
    log_info(Selftest, "Wifi %s", to_string(wifi));
}

} // namespace selftest

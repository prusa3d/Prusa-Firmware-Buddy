#pragma once

#include <wifi_credentials.hpp>
#include <selftest_result.hpp>

namespace network_wizard {

enum class WizardMode {
    /// Full wizard, opened from the network menu
    from_network_menu,

    /// Full wizard, opened as part of selftest
    from_selftest,

#if HAS_NFC()
    /// NFC-only subset; raised NFC autoscan
    nfc_only,
#endif
};

/// To be called from the marlin thread, this is basically a gcode
void network_setup_wizard();

/// To be called from the marlin thread, this is basically a gcode
void network_selftest_wizard();

#if HAS_NFC()
/// Subpart of the wi-fi wizard, raised when receiving wi-fi credentials over NFC without prompt
void network_nfc_wizard(const WifiCredentials &creds);
#endif

TestResult network_selftest_result();

}; // namespace network_wizard

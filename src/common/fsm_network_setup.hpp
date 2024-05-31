#pragma once

#include <option/has_nfc.h>
#include <wifi_credentials.hpp>

namespace network_wizard {

enum class WizardMode {
    /// Full wizard, opened from the network menu
    from_network_menu,

    /// Full wizard, opened on the first run of the printer
    initial_setup,

    /// Ini file autodetected, user asked to load it
    ini_load_only,

#if HAS_NFC()
    /// NFC-only subset; raised NFC autoscan
    nfc_only,
#endif
};

/// To be called from the marlin thread, this is basically a gcode
void network_setup_wizard();

/// To be called from the marlin thread, this is basically a gcode
void network_ini_wizard();

/// To be called from the marlin thread, this is basically a gcode
void network_initial_setup_wizard();

#if HAS_NFC()
/// Subpart of the wi-fi wizard, raised when receiving wi-fi credentials over NFC without prompt
void network_nfc_wizard(const WifiCredentials &creds);
#endif

}; // namespace network_wizard

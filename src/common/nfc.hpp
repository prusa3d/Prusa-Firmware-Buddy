#pragma once

#include <option/has_nfc.h>
#if HAS_NFC()

    #include <wifi_credentials.hpp>

    #include <optional>

namespace nfc {

void init();

void turn_on();
void turn_off();

bool has_activity();

std::optional<WifiCredentials> consume_data();

}; // namespace nfc

#endif

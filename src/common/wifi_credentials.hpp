#pragma once

#include <config_store/constants.hpp>

#include <array>

struct WifiCredentials {
    std::array<char, config_store_ns::wifi_max_ssid_len + 1> ssid;
    std::array<char, config_store_ns::wifi_max_passwd_len + 1> password;
};

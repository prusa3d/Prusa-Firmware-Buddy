#pragma once

#include <stddef.h>

namespace config_store_ns::old_eeprom {

inline constexpr size_t EEPROM_MAX_TOOL_COUNT { 6 };
inline constexpr size_t LAN_HOSTNAME_MAX_LEN { 20 };
inline constexpr size_t CONNECT_HOST_SIZE { 20 };
inline constexpr size_t CONNECT_TOKEN_SIZE { 20 };
inline constexpr size_t LAN_EEFLG_ONOFF { 1 }; // EEPROM flag for user-defined settings (SW turn OFF/ON of the LAN)
inline constexpr size_t LAN_EEFLG_TYPE { 2 }; // EEPROM flag for user-defined settings (Switch between dhcp and static)
inline constexpr size_t PL_PASSWORD_SIZE { 16 };
inline constexpr size_t WIFI_EEFLG_SEC { 0b100 }; // reserved, previously Wifi security (ap_sec_t).
inline constexpr size_t WIFI_MAX_SSID_LEN { 32 };
inline constexpr size_t WIFI_MAX_PASSWD_LEN { 64 };

} // namespace config_store_ns::old_eeprom

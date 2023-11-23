#pragma once

#include <module/temperature.h>
#include "old_eeprom/constants.hpp"

// TODO: Find a better home for this
inline bool operator==(PID_t lhs, PID_t rhs) {
    return lhs.Kd == rhs.Kd && lhs.Ki == rhs.Ki && lhs.Kp == rhs.Kp;
}

// TODO: Find a better home for this
enum class HWCheckSeverity : uint8_t {
    Ignore = 0,
    Warning = 1,
    Abort = 2
};

/**
 * @brief Allow metrics.
 * @note You can add, but never reorder items.
 */
enum class MetricsAllow : uint8_t {
    None = 0, ///< Metrics are not allowed
    One = 1, ///< Metrics can be enabled only to one selected host
    All = 2, ///< Metrics can be enabled to any host
};

namespace config_store_ns {
// place for constants relevant to config_store
inline constexpr size_t sheets_num { 8 };
inline constexpr float z_offset_uncalibrated { std::numeric_limits<float>::max() };

inline constexpr size_t max_tool_count { old_eeprom::EEPROM_MAX_TOOL_COUNT };
inline constexpr size_t lan_hostname_max_len { old_eeprom::LAN_HOSTNAME_MAX_LEN };
inline constexpr size_t connect_host_size { old_eeprom::CONNECT_HOST_SIZE };
inline constexpr size_t connect_token_size { old_eeprom::CONNECT_TOKEN_SIZE };
inline constexpr size_t pl_password_size { old_eeprom::PL_PASSWORD_SIZE };
inline constexpr size_t wifi_max_ssid_len { old_eeprom::WIFI_MAX_SSID_LEN };
inline constexpr size_t wifi_max_passwd_len { old_eeprom::WIFI_MAX_PASSWD_LEN };

inline constexpr size_t metrics_host_size { connect_host_size }; ///< Size of metrics host string
inline constexpr int16_t stallguard_sensitivity_unset { std::numeric_limits<int16_t>::max() };
} // namespace config_store_ns

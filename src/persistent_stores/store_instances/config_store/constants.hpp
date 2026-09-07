#pragma once

#include <module/temperature.h>
#include <option/has_auto_retract.h>

#include <limits>
#include <cstdint>

// TODO: Find a better home for this
inline bool operator==(PID_t lhs, PID_t rhs) {
    return lhs.Kd == rhs.Kd && lhs.Ki == rhs.Ki && lhs.Kp == rhs.Kp;
}

namespace config_store_ns {
// place for constants relevant to config_store
inline constexpr size_t sheets_num { 8 };
inline constexpr float z_offset_uncalibrated { std::numeric_limits<float>::max() };

inline constexpr size_t lan_hostname_max_len { 20 };
inline constexpr size_t connect_host_size { 20 };
inline constexpr size_t connect_proxy_size { 30 };
inline constexpr size_t connect_token_size { 20 };
inline constexpr size_t pl_password_size { 16 };
inline constexpr size_t wifi_max_ssid_len { 32 };
inline constexpr size_t wifi_max_passwd_len { 64 };

inline constexpr size_t metrics_host_size { connect_host_size }; ///< Size of metrics host string
inline constexpr int16_t stallguard_sensitivity_unset { std::numeric_limits<int16_t>::max() };

// 0 = no override -> derive the X/Y steps/mm for this unit's hardware at runtime.
inline constexpr float steps_per_unit_unset { 0.0f };

#if HAS_AUTO_RETRACT()
inline constexpr uint8_t invalid_retracted_distance = 255;
#endif
} // namespace config_store_ns

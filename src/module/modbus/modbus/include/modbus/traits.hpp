/// @file
#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utils/byte_utils.hpp>
#include <type_traits>

namespace modbus {

/// Maximum size of a register file in bytes.
/// MODBUS PDU is 253 bytes, 6 are used for function header, leaving 247 bytes.
/// Rounded down to 246 to fit whole 16-bit registers.
inline constexpr size_t max_register_file_size_bytes = 246;

/// Concept for a modbus register file struct
template <typename T>
concept RegisterFile = std::is_standard_layout_v<T> && requires { { T::address } -> std::convertible_to<uint16_t>; } && (sizeof(T) <= max_register_file_size_bytes) && (sizeof(T) % sizeof(uint16_t) == 0) && (alignof(T) == alignof(uint16_t));

/// Concept for register files that carry a payload with size and data fields.
template <typename T>
concept RegisterFileWithPayload = RegisterFile<T> && requires(const T &reg) {
    { reg.size } -> std::convertible_to<size_t>;
    { std::span { reg.data } };
};

/// Convert register file payload to a bounded byte span.
/// The size field is clamped to the actual buffer size for safety.
template <RegisterFileWithPayload T>
Bytes payload(const T &reg) {
    const auto bytes = std::as_bytes(std::span { reg.data });
    return bytes.subspan(0, std::min<size_t>(reg.size, bytes.size()));
}

} // namespace modbus

#pragma once

#include <expected>
#include <cstdint>

#include <bsod/bsod.h>

namespace nfcv {
/// Common error type for all the implementations and interface
/// Feel free to extended the values
enum class Error : uint8_t {
    timeout,
    invalid_chip,
    bad_oscilator,
    buffer_overflow,
    invalid_crc,
    device_hard_framing_error,
    device_soft_framing_error,
    device_parity_error,
    device_crc_error,
    no_response,
    response_invalid_size,
    response_format_invalid,
    response_is_error,
    bad_request,
    not_implemented,

    /// RF collision avoidance found a foreign field already radiating
    field_collision,

    other,
    unknown,
};

template <typename T>
using Result = std::expected<T, Error>;

/// Name of the error, for log messages
constexpr const char *to_str(Error error) {
    // No default case: a new Error value should not silently become "?"
    switch (error) {
    case Error::timeout:
        return "timeout";
    case Error::invalid_chip:
        return "invalid_chip";
    case Error::bad_oscilator:
        return "bad_oscilator";
    case Error::buffer_overflow:
        return "buffer_overflow";
    case Error::invalid_crc:
        return "invalid_crc";
    case Error::device_hard_framing_error:
        return "device_hard_framing_error";
    case Error::device_soft_framing_error:
        return "device_soft_framing_error";
    case Error::device_parity_error:
        return "device_parity_error";
    case Error::device_crc_error:
        return "device_crc_error";
    case Error::no_response:
        return "no_response";
    case Error::response_invalid_size:
        return "response_invalid_size";
    case Error::response_format_invalid:
        return "response_format_invalid";
    case Error::response_is_error:
        return "response_is_error";
    case Error::bad_request:
        return "bad_request";
    case Error::not_implemented:
        return "not_implemented";
    case Error::field_collision:
        return "field_collision";
    case Error::other:
        return "other";
    case Error::unknown:
        return "unknown";
    }
    bsod_unreachable();
}
} // namespace nfcv

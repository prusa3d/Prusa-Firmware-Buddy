/// @file
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utils/byte_utils.hpp>

namespace cyphal {

/// Abstract component health information.
enum class Health : uint8_t {
    /// The component is functioning properly (nominal).
    nominal = 0,

    /// A critical parameter went out of range or the component encountered
    /// a minor failure that does not prevent the subsystem from performing
    /// any of its real-time functions.
    advisory = 1,

    /// The component encountered a major failure and is performing
    /// in a degraded mode or outside of its designed limitations.
    caution = 2,

    /// The component suffered a fatal malfunction and is unable to perform its intended function.
    warning = 3,
};

/// The operating mode of a node.
enum class Mode : uint8_t {
    /// Normal operating mode.
    operational = 0,

    /// Initialization is in progress; this mode is entered immediately after startup.
    initialization = 1,

    /// E.g., calibration, self-test, etc.
    maintenance = 2,

    /// New software/firmware is being loaded or the bootloader is running.
    software_update = 3,
};

/// Generic message severity representation.
enum class Severity : uint8_t {
    /// Messages of this severity can be used only during development.
    /// They shall not be used in a fielded operational system.
    trace = 0,

    /// Messages that can aid in troubleshooting.
    /// Messages of this severity and lower should be disabled by default.
    debug = 1,

    /// General informational messages of low importance.
    /// Messages of this severity and lower should be disabled by default.
    info = 2,

    /// General informational messages of high importance.
    /// Messages of this severity and lower should be disabled by default.
    notice = 3,

    /// Messages reporting abnormalities and warning conditions.
    /// Messages of this severity and higher should be enabled by default.
    warning = 4,

    /// Messages reporting problems and error conditions.
    /// Messages of this severity and higher should be enabled by default.
    error = 5,

    /// Messages reporting serious problems and critical conditions.
    /// Messages of this severity and higher should be always enabled.
    critical = 6,

    /// Notifications of dangerous circumstances that demand immediate attention.
    /// Messages of this severity should be always enabled.
    alert = 7,
};

/// Strong type representing node ID on cyphal network.
class NodeId {
private:
    uint8_t value;

public:
    /// Construct invalid/unset NodeId.
    constexpr NodeId()
        : value { 255 } {}

    constexpr explicit NodeId(uint8_t value)
        : value { value } {}

    constexpr explicit operator uint8_t() const { return value; }

    constexpr bool operator==(const NodeId &) const = default;
    constexpr bool operator!=(const NodeId &) const = default;
};

/// Strong type representing transfer ID (5 bit unsigned integer) on cyphal network.
class TransferId {
private:
    uint8_t value;

public:
    static constexpr uint8_t mask = 0b0001'1111;

    constexpr TransferId()
        : value { 0 } {}

    constexpr explicit TransferId(uint8_t value)
        : value { static_cast<uint8_t>(value & mask) } {}

    constexpr explicit operator uint8_t() const { return value; }

    constexpr bool operator==(const TransferId &) const = default;
    constexpr bool operator!=(const TransferId &) const = default;
};

/// Strong type representing command to be executed on the node.
// Note: We use bijection to simplify implementation. If needed, this can be
//       changed to uint8_t in order to trade RAM for FLASH.
// Note: OpenCyphal specification v1.0-beta 6.4.1.1
//       Vendors can define arbitrary, vendor-specific commands in the bottom
//       part of the range (starting from zero). Vendor-specific commands shall
//       not use identifiers above 32767.
enum class Command : uint16_t {
    start_app = 0,
    get_app_salted_hash = 1,
    software_update = 65533,
    restart = 65535,
};

/// Strong type representing unique ID of the node on cyphal network.
class UniqueId {
private:
    uint8_t unique_id[16];

public:
    constexpr UniqueId() {
        std::memset(unique_id, 0, 16);
    }

    constexpr explicit UniqueId(const uint8_t unique_id_[16]) {
        std::memcpy(unique_id, unique_id_, 16);
    }

    constexpr const uint8_t *data() const { return &unique_id[0]; }
    static constexpr size_t size() { return 16; }

    constexpr auto operator<=>(const UniqueId &) const = default;
};

struct Heartbeat {
    Health health;
    Mode mode;
    uint8_t vendor_specific_status_code;
};

} // namespace cyphal

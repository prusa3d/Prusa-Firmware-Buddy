/// @file
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <xbuddy_extension/shared_enums.hpp>

/// This file defines MODBUS register files, to be shared between master and slave.
/// Resist the temptation to make this type-safe in any way! This is only used for
/// memory layout and should consist of 16-bit values, arrays and structures of such.
/// To ensure proper synchronization, you must always read/write entire register files.

namespace xbuddy_extension::modbus {

/// Helper struct to group chunk request parameters together.
struct ChunkRequest {
    uint16_t file_id; ///< request to receive a chunk of this file (FileId)
    uint16_t offset_lo; ///< request to receive a chunk with this offset (lower 16 bits)
    uint16_t offset_hi; ///< request to receive a chunk with this offset (upper 16 bits)

    bool operator==(const ChunkRequest &) const = default;
};

/// Helper struct to group digest request parameters together.
struct DigestRequest {
    uint16_t file_id; ///< request to compute digest of this file (FileId)
    uint16_t salt_lo; ///< request to compute digest with this salt (lower 16 bits)
    uint16_t salt_hi; ///< request to compute digest with this salt (upper 16 bits)

    bool operator==(const DigestRequest &) const = default;
};

/// The XL-CAN bridge has a single fan connector (the Modular Bed cooling fan)
/// wired to the xBE "fan2" pins (PA7 PWM / PA9 tach), so it populates only
/// this slot of the fan_pwm / fan_rpm arrays.
///
/// The fan1 and fan2 slots share one PWM line (TIM3_CH2), so the last write
/// wins: the bridge applies fan_pwm[0] before fan_pwm[1], and the master
/// leaves fan_pwm[0] at zero. Driving the fan from slot 0, or applying the
/// slots in the other order, would silently zero it.
static constexpr size_t XL_CAN_FAN_IDX = 1;

/// MODBUS register file for reporting current status of xBuddyExtension to motherboard.
struct Status {
    static constexpr uint16_t address = 0x8000;

    std::array<uint16_t, fan_count> fan_rpm; /// RPM of the fan
    uint16_t temperature; /// decidegree Celsius (eg. 23.5°C = 235 in the register)
    uint16_t gpio_filament_sensor; ///< Single GPIO sensor (PA5 on standard, PA9 on iX)

    /// 8x TMP1826 sensors on EXT connector (PC14)
    /// 2 bits per state, so all states fit into a single register
    uint16_t ext_filament_sensors;

    ChunkRequest chunk_request; ///< request to receive a chunk

    DigestRequest digest_request; ///< request to compute digest

    uint16_t log_message_sequence; ///< increments when new log message available

    /// Fan 5 V power switch fault (boolean). XL-CAN bridge only, 0 on xBE
    /// (the xBE fan rail has no dedicated switch). 1 = the TPS2041C reports
    /// overcurrent/overtemperature on the fan rail.
    uint16_t fan_power_fault;
};

/// MODBUS register file for setting desired config of xBuddyExtension from motherboard.
struct Config {
    static constexpr uint16_t address = 0x9000;

    std::array<uint16_t, fan_count> fan_pwm; /// PWM of the fan (0-255)

    /// white led strip intensity (0-255)
    /// DEAD on XL-CAN, the pin is used for something else
    uint16_t w_led_pwm;

    uint16_t rgbw_led_r_pwm; /// RGBW led strip, red component (0-255)
    uint16_t rgbw_led_g_pwm; /// RGBW led strip, green component (0-255)
    uint16_t rgbw_led_b_pwm; /// RGBW led strip, blue component (0-255)
    uint16_t rgbw_led_w_pwm; /// RGBW led strip, white component (0-255)
    uint16_t usb_power; /// enable power for usb port (boolean)
    uint16_t mmu_power; /// enable power for MMU port (boolean)

    /// MMU port inverted reset pin value (boolean)
    /// !!! DISCLAIMER: Used for other devices than just MMU (XLCAN -> ModularBed, INDX -> INDX_HEAD)
    uint16_t mmu_nreset;

    /// Frequency of the white led PWM.
    ///
    /// 0 = default left to discretion of the extension board.
    /// Is the frequency of the full cycle, in Hz.
    ///
    /// Can be used to implement a "strobe"
    ///
    /// Warning: PWM timer shared with some fans.
    /// DEAD on XL-CAN, the pin is used for something else
    uint16_t w_led_frequency;

    /// A value that's changing regularly, to signal to the device that the
    /// master is alive. If it doesn't change, it can assume the master is
    /// dead in some way and act accordingly.
    uint16_t activity;
};

/// MODBUS register file for transferring chunk from motherboard.
struct Chunk {
    static constexpr uint16_t address = 0x9100;

    ChunkRequest request; ///< echoed back to prevent mixup
    uint16_t size; ///< how many valid bytes are in data; must be full unless it's the last chunk
    std::array<uint16_t, 119> data; ///< actual bytes of the chunk (little endian)
};

/// MODBUS register file for transferring digest from motherboard.
struct Digest {
    static constexpr uint16_t address = 0x9200;

    DigestRequest request; ///< echoed back to prevent mixup
    uint16_t status; ///< DigestStatus encoded as uint16_t
    std::array<uint16_t, 16> data; ///< actual bytes of the digest (little endian); only valid when status == ok
};

/// MODBUS register file for reporting log messages from xBuddyExtension to motherboard.
struct LogMessage {
    static constexpr uint16_t address = 0x9300;

    uint16_t sequence; ///< sequence number when this record was written, see Status::log_message_sequence
    uint16_t text_size; ///< length of valid text_data in bytes
    std::array<uint16_t, 121> text_data; ///< actual bytes of log message (little endian)
};

/// MODBUS register file for draining the CyphalBridgeQueue.
/// Messages are packed as [1B length | 2B port_id LE | length bytes payload]...
/// Byte content is packed little-endian into uint16 registers.
struct CyphalBridge {
    static constexpr uint16_t address = 0x8100;

    uint16_t bytes_available; ///< bytes remaining in queue after this read
    uint16_t size; ///< valid bytes in data[]
    std::array<uint16_t, 120> data; ///< packed bridge messages (little endian)
};

/// Parse FileId from modbus structure.
FileId parse_file_id(uint16_t);

/// Serialize FileId to modbus structure.
uint16_t serialize_file_id(FileId);

/// Parse DigestStatus from modbus structure.
DigestStatus parse_digest_status(uint16_t);

/// Serialize DigestStatus to modbus structure.
uint16_t serialize_digest_status(DigestStatus);

} // namespace xbuddy_extension::modbus

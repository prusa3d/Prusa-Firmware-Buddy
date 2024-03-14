#pragma once

#include <cstdint>
#include <array>
#include <functional>

namespace common::puppies::fifo {

/**
 * FIFO coded stream
 *
 * Instead of passing data as raw Modbus registers in FIFO stream this defines messages
 * on top of FIFO stream data.
 *
 * A message looks like as follows:
 * - 1 byte message type (MessageType)
 * - static (per message type) payload
 *
 * Important notes:
 * - Messages do not cross FIFO transfer boundaries - each transfer can be decoded into
 *   message upon receival without additional buffering.
 * - Message type 0 means no message - padding. This allows to pad data with 0 if necessary.
 */

inline constexpr auto MODBUS_FIFO_LEN = 31;

enum class MessageType : uint8_t {
    no_data = 0, // Padding
    log = 1,
    loadcell = 2,
    accelerometer = 3, ///< Single sample with timestamp
    accelerometer_fast = 4, ///< Multiple samples without timestamp
    // ...
};

typedef uint32_t TimeStamp_us;

typedef struct __attribute__((packed)) {
    MessageType type;
} Header;

// Payload types
typedef uint32_t AccelerometerXyzSample;
typedef uint32_t LoadcellSample_t;

typedef struct __attribute__((packed)) {
    TimeStamp_us timestamp;
    LoadcellSample_t loadcell_raw_value;
} LoadcellRecord;

typedef std::array<char, 8> LogData;
typedef struct __attribute__((packed)) {
    TimeStamp_us timestamp_us;
    AccelerometerXyzSample sample;
} AccelerometerData;
typedef std::array<AccelerometerXyzSample, 2> AccelerometerFastData;

// Payload to message type mapping using template specialization
template <typename T>
inline constexpr MessageType message_type() {
    return MessageType::no_data;
}

template <>
inline constexpr MessageType message_type<LogData>() {
    return MessageType::log;
}

template <>
inline constexpr MessageType message_type<LoadcellRecord>() {
    return MessageType::loadcell;
}

template <>
inline constexpr MessageType message_type<AccelerometerData>() {
    return MessageType::accelerometer;
}

template <>
inline constexpr MessageType message_type<AccelerometerFastData>() {
    return MessageType::accelerometer_fast;
}
} // namespace common::puppies::fifo

namespace dwarf::accelerometer {
struct AccelerometerRecord {
    common::puppies::fifo::TimeStamp_us timestamp;
    union {
        struct {
            int16_t x, y, z;
        };
        int16_t raw[3];
    };
    bool buffer_overflow = false;
    bool sample_overrun = false;
};
} // namespace dwarf::accelerometer

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
 * - 2 bytes timestamp (TimeStamp_us_t)
 * - 1 byte message type (MessageType)
 * - static (per message type) payload
 *
 * Important notes:
 * - Messages do not cross FIFO transfer boundaries - each transfer can be decoded into
 *   message upon receival without additional buffering.
 * - Message type 0 means no message - padding. This allows to pad data with 0 if necessary.
 */

static constexpr auto MODBUS_FIFO_LEN = 31;

enum class MessageType : uint8_t {
    no_data = 0, // Padding
    log = 1,
    loadcell = 2,
    // ...
};

typedef uint32_t TimeStamp_us_t;

typedef struct __attribute__((packed)) {
    TimeStamp_us_t timestamp_us;
    MessageType type;
} Header_t;

// Payload types
typedef int32_t LoadCellData_t;
typedef std::array<char, 8> LogData_t;

// Payload to message type mapping using template specialization
template <typename T>
inline constexpr MessageType message_type() {
    return MessageType::no_data;
}

template <>
inline constexpr MessageType message_type<LogData_t>() {
    return MessageType::log;
}

template <>
inline constexpr MessageType message_type<LoadCellData_t>() {
    return MessageType::loadcell;
}

}

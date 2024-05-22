#pragma once

#include <cstdint>
#include <cstring>
#include <array>

#include "log.h"
#include "fifo_coder.hpp"

namespace common::puppies::fifo {

LOG_COMPONENT_REF(ModbusFIFOEncoder);

/**
 * Encoder
 *
 * Encoder is constructed on top of a std::array hoding data to be send as Modbus FIFO transfer.
 *
 * Calls to can_encode are used to check available space and encode are used to actually put the message in.
 * Finally padd is used to fill possibly half-filled last Modbus register.
 */
class Encoder {
public:
    Encoder(std::array<uint16_t, MODBUS_FIFO_LEN> &fifo);

    /**
     * Test whenever message fits in
     */
    template <typename T>
    bool can_encode() {
        return sizeof(Header) + sizeof(T) <= available_bytes();
    }

    /**
     * Actually encode message in the stream
     */
    template <typename T>
    bool encode(const T data) {
        log_debug(
            ModbusFIFOEncoder,
            "Encoding message type: %u, size: %zu+%zu at byte offset: %u",
            static_cast<unsigned>(message_type<T>()),
            sizeof(Header),
            sizeof(T),
            fifo_bytes_pos);
        if (!can_encode<T>()) {
            return false;
        }

        Header header = {
            .type = message_type<T>(),
        };

        put(header);
        put(data);

        return true;
    }

    /**
     * Get number of Modbus registers used by encoded messages
     */
    size_t position() const;

    /**
     * Make sure all Modbus registers used are completely defined
     */
    void padd();

private:
    std::array<uint16_t, MODBUS_FIFO_LEN> &fifo; // TODO: In C++20 this can be a span - we might save some casts
    uint8_t fifo_bytes_pos;

    /**
     * Store data in the stream
     */
    template <typename T>
    void put(const T data) {
        static_assert(std::is_standard_layout<T>());
        uint8_t *pos = reinterpret_cast<uint8_t *>(fifo.data()) + fifo_bytes_pos;
        memcpy(pos, &data, sizeof(data));
        fifo_bytes_pos += sizeof(T);
    }

    uint8_t available_bytes() const;
};

} // namespace common::puppies::fifo

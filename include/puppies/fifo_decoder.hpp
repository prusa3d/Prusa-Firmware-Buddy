#pragma once

#include <cstdint>
#include <cstring>
#include <array>

#include "log.h"
#include "fifo_coder.hpp"

namespace common::puppies::fifo {

LOG_COMPONENT_REF(ModbusFIFODecoder);

/**
 * Decoder
 *
 * Decoder is instantiated over received FIFO transfer
 *
 * decode method is used to decode messages and call callbacks defined as fuctions stored in a dedicated structure.
 */
class Decoder {
public:
    typedef struct {
        std::function<void(TimeStamp_us_t, LogData_t)> log_handler;
        std::function<void(TimeStamp_us_t, LoadCellData_t)> loadcell_handler;
    } Callbacks_t;

    Decoder(std::array<uint16_t, MODBUS_FIFO_LEN> &fifo, size_t len);
    void decode(const Callbacks_t callbacks);

private:
    std::array<uint16_t, MODBUS_FIFO_LEN> &fifo;
    uint8_t *data;
    size_t len;

    template <typename T>
    bool can_get() {
        return sizeof(T) <= available_bytes();
    }

    template <typename T>
    T get() {
        static_assert(std::is_standard_layout<T>());
        T ret;
        memcpy(&ret, data, sizeof(ret));
        data += sizeof(T);
        return ret;
    }

    template <typename R, typename TIME_T, typename PAYLOAD_T>
    void make_call(TimeStamp_us_t const &timestamp_us, std::function<R(TIME_T, PAYLOAD_T)> const &function) {
        if (function && can_get<PAYLOAD_T>()) {
            function(timestamp_us, get<PAYLOAD_T>());
        }
    }

    uint8_t available_bytes() const;
};

}

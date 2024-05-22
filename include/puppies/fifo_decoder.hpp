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
        std::function<void(LogData)> log_handler;
        std::function<void(LoadcellRecord)> loadcell_handler;
        std::function<void(AccelerometerFastData)> accelerometer_fast_handler;
        std::function<void(AccelerometerSamplingRate)> accelerometer_freq_handler;
    } Callbacks_t;

    Decoder(std::array<uint16_t, MODBUS_FIFO_LEN> &fifo, size_t len);
    void decode(const Callbacks_t callbacks);

    /**
     * @brief Guess if there is more data in FIFO.
     * @return true if there is
     */
    bool more() const;

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

    template <typename R, typename PAYLOAD_T>
    void make_call(std::function<R(PAYLOAD_T)> const &function) {
        if (can_get<PAYLOAD_T>()) {
            // Extract payload even when no callback function is defined to
            // advance the data pointer to the next header.
            PAYLOAD_T payload = get<PAYLOAD_T>();
            if (function) {
                function(payload);
            }
        }
    }

    uint8_t available_bytes() const;
};

} // namespace common::puppies::fifo

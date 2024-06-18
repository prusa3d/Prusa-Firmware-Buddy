#pragma once

#include <cstdint>
#include <cstring>
#include <array>

#include <logging/log.hpp>
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
    class Callbacks {
    public:
        virtual void decode_log(const LogData &) {}
        virtual void decode_loadcell(const LoadcellRecord &) {}
        virtual void decode_accelerometer_fast(const AccelerometerFastData &) {}
        virtual void decode_accelerometer_freq(const AccelerometerSamplingRate &) {}

    protected:
        ~Callbacks() = default;
    };

    Decoder(std::array<uint16_t, MODBUS_FIFO_LEN> &fifo, size_t len);
    void decode(Callbacks &callbacks);

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
    void make_call(Callbacks &callbacks, R (Callbacks::*function)(const PAYLOAD_T &)) {
        if (can_get<PAYLOAD_T>()) {
            // Extract payload even when no callback function is defined to
            // advance the data pointer to the next header.
            PAYLOAD_T payload = get<PAYLOAD_T>();
            (callbacks.*function)(payload);
        }
    }

    uint8_t available_bytes() const;
};

} // namespace common::puppies::fifo

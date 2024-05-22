#include "puppies/fifo_decoder.hpp"

#include <cassert>

namespace common::puppies::fifo {

LOG_COMPONENT_DEF(ModbusFIFODecoder, LOG_SEVERITY_INFO);

Decoder::Decoder(std::array<uint16_t, MODBUS_FIFO_LEN> &fifo, size_t len)
    : fifo(fifo)
    , data(reinterpret_cast<uint8_t *>(fifo.data()))
    , len(len * sizeof(uint16_t)) {
    assert(len <= fifo.size() * sizeof(uint16_t));
}

void Decoder::decode(const Callbacks_t callbacks) {
    // As FIFO data come in 2 byte "register" chunks the last byte may be padding.
    // To avoid interpreting padding and random subsequent bytes as messages this
    // skips decoding when there is not enough data to read header.
    while (sizeof(Header) <= available_bytes()) {
        Header header = get<Header>();
        switch (header.type) {
        case MessageType::no_data:
            break;
        case MessageType::log:
            make_call(callbacks.log_handler);
            break;
        case MessageType::loadcell:
            make_call(callbacks.loadcell_handler);
            break;
        case MessageType::accelerometer_fast:
            make_call(callbacks.accelerometer_fast_handler);
            break;
        case MessageType::accelerometer_sampling_rate:
            make_call(callbacks.accelerometer_freq_handler);
            break;
        default:
            assert(false);
        }
    }
}

bool Decoder::more() const {
    // TODO: FIFO needs info about how many bytes are in it and clear it by another channel.
    // Until fixed: If another item wouldn't fit the message, we assume there is more in FIFO.
    return (sizeof(std::array<uint16_t, MODBUS_FIFO_LEN>) - len) // Remainder of the packet
        < sizeof(Header) + std::max(sizeof(LogData), sizeof(LoadcellRecord)); // Size of the biggest item
}

uint8_t Decoder::available_bytes() const {
    return (reinterpret_cast<uint8_t *>(fifo.data()) + len) - data;
}

} // namespace common::puppies::fifo

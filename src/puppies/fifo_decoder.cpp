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
    // To avoid interpretting padding and random subsequant bytes as messages this
    // skips deocding when there is not enough data to read header.
    while (sizeof(Header_t) <= available_bytes()) {
        Header_t header = get<Header_t>();
        switch (header.type) {
        case MessageType::no_data:
            break;
        case MessageType::log:
            make_call(header.timestamp_us, callbacks.log_handler);
            break;
        case MessageType::loadcell:
            make_call(header.timestamp_us, callbacks.loadcell_handler);
            break;
        default:
            log_warning(ModbusFIFODecoder, "Unknown message type: %d", header.type);
        }
    }
}

uint8_t Decoder::available_bytes() const {
    return (reinterpret_cast<uint8_t *>(fifo.data()) + len) - data;
}

}

#include "puppies/fifo_encoder.hpp"

namespace common::puppies::fifo {

LOG_COMPONENT_DEF(ModbusFIFOEncoder, LOG_SEVERITY_INFO);

Encoder::Encoder(std::array<uint16_t, MODBUS_FIFO_LEN> &fifo)
    : fifo(fifo)
    , fifo_bytes_pos(0) {};

size_t Encoder::position() const {
    return (fifo_bytes_pos + sizeof(uint16_t) - 1) / sizeof(uint16_t);
}

void Encoder::padd() {
    if (fifo_bytes_pos % sizeof(uint16_t)) {
        put(static_cast<std::byte>(0));
    }
}

uint8_t Encoder::available_bytes() const {
    return fifo.size() * sizeof(uint16_t) - fifo_bytes_pos;
}

} // namespace common::puppies::fifo

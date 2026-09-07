/// @file
#include <puppies/cyphal_bridge.hpp>

#include <logging/log.hpp>

LOG_COMPONENT_REF(Buddy);

using Lock = std::unique_lock<freertos::Mutex>;

namespace buddy::puppies {

CyphalBridge cyphal_bridge;

CommunicationStatus CyphalBridge::refresh(PuppyModbus &bus, modbus::ServerAddress server) {
    Lock lock(mutex);

    // Drain the Cyphal bridge queue (up to 5 reads per cycle)
    CommunicationStatus status = CommunicationStatus::SKIPPED;
    if (stream_callback_ || bridge_has_stale_data_) {
        for (int i = 0; i < 5; ++i) {
            if (bus.read_input_registers(server, register_file.value)) {
                dispatch_bridge_messages();
                status = CommunicationStatus::OK;
                if (register_file.value.bytes_available == 0) {
                    bridge_has_stale_data_ = false;
                    break;
                }
            } else {
                return CommunicationStatus::ERROR;
            }
        }
    }
    return status;
}

void CyphalBridge::set_stream_callback(StreamCallback cb, void *ctx) {
    Lock lock(mutex);
    stream_callback_ = cb;
    stream_callback_ctx_ = ctx;
    bridge_has_stale_data_ = true; // There may be old data we dont want to send to callback
}

void CyphalBridge::dispatch_bridge_messages() {
    if (!stream_callback_ || bridge_has_stale_data_) {
        return;
    }

    static_assert(std::endian::native == std::endian::little);
    const auto bytes = std::as_bytes(std::span { register_file.value.data });
    const uint16_t size = std::min<uint16_t>(register_file.value.size, static_cast<uint16_t>(bytes.size()));
    size_t offset = 0;

    while (offset + 3 <= size) {
        const uint8_t payload_len = static_cast<uint8_t>(bytes[offset]);
        const uint16_t port_id = static_cast<uint16_t>(bytes[offset + 1])
            | (static_cast<uint16_t>(bytes[offset + 2]) << 8);
        offset += 3;

        if (offset + payload_len > size) {
            log_warning(Buddy, "Cyphal: bridge msg truncated len=%u offset=%zu size=%u", payload_len, offset, size);
            break;
        }

        stream_callback_(port_id, bytes.subspan(offset, payload_len), stream_callback_ctx_);
        offset += payload_len;
    }
}

} // namespace buddy::puppies

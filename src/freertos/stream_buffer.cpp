/// @file
#include <freertos/stream_buffer.hpp>

#include <cassert>
#include <cstdlib>

// FreeRTOS.h must be included before stream_buffer.h
#include <FreeRTOS.h>
#include <stream_buffer.h>

namespace freertos {

StreamBufferBase::StreamBufferBase(std::span<std::byte> data_storage) {
    // If these asserts start failing, go fix the constants.
    static_assert(stream_buffer_storage_size == sizeof(StaticStreamBuffer_t));
    static_assert(stream_buffer_storage_align == alignof(StaticStreamBuffer_t));

    handle = xStreamBufferCreateStatic(
        data_storage.size() - 1,
        0,
        (uint8_t *)data_storage.data(),
        reinterpret_cast<StaticStreamBuffer_t *>(&stream_buffer_storage));
}

StreamBufferBase::~StreamBufferBase() {
    vStreamBufferDelete(StreamBufferHandle_t(handle));
}

std::span<std::byte> StreamBufferBase::receive(std::span<std::byte> buffer) {
    assert(!xPortIsInsideInterrupt());
    size_t count = xStreamBufferReceive(StreamBufferHandle_t(handle),
        buffer.data(),
        buffer.size(),
        0);
    return { buffer.data(), count };
}

std::span<std::byte> StreamBufferBase::receive_from_isr(std::span<std::byte> buffer) {
    assert(xPortIsInsideInterrupt());
    size_t count = xStreamBufferReceiveFromISR(StreamBufferHandle_t(handle),
        buffer.data(),
        buffer.size(),
        nullptr);
    return { buffer.data(), count };
}

std::span<const std::byte> StreamBufferBase::send(std::span<const std::byte> buffer) {
    assert(!xPortIsInsideInterrupt());
    const size_t count = xStreamBufferSend(StreamBufferHandle_t(handle),
        buffer.data(),
        buffer.size(),
        0);
    return buffer.subspan(count);
}

std::span<const std::byte> StreamBufferBase::send_from_isr(std::span<const std::byte> buffer) {
    assert(xPortIsInsideInterrupt());
    const size_t count = xStreamBufferSendFromISR(StreamBufferHandle_t(handle),
        buffer.data(),
        buffer.size(),
        nullptr);
    return buffer.subspan(count);
}

} // namespace freertos

/// @file
#include <freertos/stream_buffer.hpp>

#include <cstdlib>

// FreeRTOS.h must be included before stream_buffer.h
#include <FreeRTOS.h>
#include <stream_buffer.h>
#include <bsod/bsod.h>
#include <utils/byte_utils.hpp>

namespace freertos {

StreamBufferBase::StreamBufferBase(WritableBytes data_storage) {
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

WritableBytes StreamBufferBase::receive(WritableBytes buffer) {
    debug_assert(!xPortIsInsideInterrupt());
    size_t count = xStreamBufferReceive(StreamBufferHandle_t(handle),
        buffer.data(),
        buffer.size(),
        0);
    return { buffer.data(), count };
}

WritableBytes StreamBufferBase::receive_from_isr(WritableBytes buffer) {
    debug_assert(xPortIsInsideInterrupt());
    BaseType_t higher_priority_task_woken = pdFALSE;
    size_t count = xStreamBufferReceiveFromISR(StreamBufferHandle_t(handle),
        buffer.data(),
        buffer.size(),
        &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
    return { buffer.data(), count };
}

Bytes StreamBufferBase::send(Bytes buffer) {
    debug_assert(!xPortIsInsideInterrupt());
    const size_t count = xStreamBufferSend(StreamBufferHandle_t(handle),
        buffer.data(),
        buffer.size(),
        0);
    return buffer.subspan(count);
}

Bytes StreamBufferBase::send_from_isr(Bytes buffer) {
    debug_assert(xPortIsInsideInterrupt());
    BaseType_t higher_priority_task_woken = pdFALSE;
    const size_t count = xStreamBufferSendFromISR(StreamBufferHandle_t(handle),
        buffer.data(),
        buffer.size(),
        &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
    return buffer.subspan(count);
}

} // namespace freertos

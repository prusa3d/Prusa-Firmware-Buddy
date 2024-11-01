/// @file
#pragma once

#include <span>
#include <cstddef>
#include <freertos/config.hpp>

namespace freertos {

/**
 * C++ wrapper for FreeRTOS stream buffer.
 */
class StreamBufferBase {
public:
    // We use erased storage in order to not pollute the scope with FreeRTOS internals.
    // The actual size and alignment are statically asserted in implementation file.
    using Storage = std::array<std::byte, stream_buffer_storage_size>;

private:
    void *handle;
    alignas(stream_buffer_storage_align) Storage stream_buffer_storage;

protected:
    StreamBufferBase(std::span<std::byte> data_storage);
    ~StreamBufferBase();
    StreamBufferBase(const StreamBufferBase &) = delete;
    StreamBufferBase &operator=(const StreamBufferBase &) = delete;
    StreamBufferBase(StreamBufferBase &&) = delete;
    StreamBufferBase &operator=(StreamBufferBase &&) = delete;

public:
    /**
     * Try receiving data from this StreamBuffer into provided buffer.
     * Return view into provided buffer containing data that was received so far.
     * Must not be called from interrupt!
     */
    [[nodiscard]] std::span<std::byte> receive(std::span<std::byte> buffer);

    /**
     * Try receiving data from this StreamBuffer into provided buffer.
     * Return view into provided buffer containing data that was received so far.
     * May only be called from interrupt!
     */
    [[nodiscard]] std::span<std::byte> receive_from_isr(std::span<std::byte> buffer);

    /**
     * Try sending data into this StreamBuffer from provided buffer.
     * Return view into provided buffer containing data that was not yet sent.
     * Must not be called from interrupt!
     */
    [[nodiscard]] std::span<const std::byte> send(std::span<const std::byte> buffer);

    /**
     * Try sending data into this StreamBuffer from provided buffer.
     * Return view into provided buffer containing data that was not yet sent.
     * May only be called from interrupt!
     */
    [[nodiscard]] std::span<const std::byte> send_from_isr(std::span<const std::byte> buffer);
};

/**
 * C++ wrapper for FreeRTOS stream buffer and its storage.
 */
template <size_t N>
class StreamBuffer : public StreamBufferBase {
public:
    StreamBuffer()
        : StreamBufferBase(data_storage) {}

private:
    std::array<std::byte, N> data_storage;
};

} // namespace freertos

#include "serial_log.h"

#include <USBSerial.h>
#include <freertos/mutex.hpp>

#include <array>
#include <atomic>
#include <cstring>
#include <mutex>

namespace nhttp::printer {

namespace {
    // Storage and synchronization for the ring buffer. Kept in an anon namespace
    // so internal state is hidden from headers; one global SerialLog instance
    // shares this state.
    std::array<uint8_t, SerialLog::BUFFER_SIZE> ring {};
    size_t head { 0 };  ///< write index (next slot to overwrite)
    bool wrapped { false };  ///< true once we've written ≥ BUFFER_SIZE bytes
    std::atomic<uint32_t> total_written { 0 };

    // Lazy-initialized mutex. Constructing freertos::Mutex at C++ static-init
    // time runs BEFORE the FreeRTOS scheduler is up, which yields a broken
    // handle. A function-local static is initialized on first call (which is
    // after the WUI task starts), so the scheduler is already running.
    freertos::Mutex &ring_mutex() {
        static freertos::Mutex m;
        return m;
    }

    void append_locked(const uint8_t *data, size_t len) {
        // Memcpy in up to two segments, wrapping at the ring end.
        while (len > 0) {
            const size_t space_to_end = SerialLog::BUFFER_SIZE - head;
            const size_t chunk = (len < space_to_end) ? len : space_to_end;
            std::memcpy(&ring[head], data, chunk);
            head += chunk;
            if (head >= SerialLog::BUFFER_SIZE) {
                head = 0;
                wrapped = true;
            }
            data += chunk;
            len -= chunk;
        }
    }

    void on_serial_line(const uint8_t *buf, int len) {
        if (len <= 0 || !buf) {
            return;
        }
        std::lock_guard lock { ring_mutex() };
        append_locked(buf, static_cast<size_t>(len));
        total_written.fetch_add(static_cast<uint32_t>(len), std::memory_order_relaxed);
    }
} // namespace

SerialLog::SerialLog() = default;

void SerialLog::append(const uint8_t *data, size_t len) {
    if (len == 0 || !data) {
        return;
    }
    std::lock_guard lock { ring_mutex() };
    append_locked(data, len);
    total_written.fetch_add(static_cast<uint32_t>(len), std::memory_order_relaxed);
}

size_t SerialLog::snapshot_into(char *dst, size_t dst_size) const {
    if (!dst || dst_size == 0) {
        return 0;
    }

    std::lock_guard lock { ring_mutex() };

    // Compute current size and oldest-first layout
    const size_t cur_size = wrapped ? BUFFER_SIZE : head;
    if (cur_size == 0) {
        dst[0] = '\0';
        return 0;
    }

    static constexpr char kTruncMarker[] = "... (truncated)\n";
    static constexpr size_t kTruncLen = sizeof(kTruncMarker) - 1;

    size_t out = 0;
    size_t src_skip = 0;
    if (cur_size > dst_size - 1) {
        // Reserve room for marker + null terminator. Skip the oldest bytes.
        const size_t reserve = kTruncLen;
        if (dst_size > reserve + 1) {
            std::memcpy(dst, kTruncMarker, kTruncLen);
            out = kTruncLen;
            src_skip = cur_size - (dst_size - 1 - kTruncLen);
        } else {
            // dst too small for marker; just take the tail
            src_skip = cur_size - (dst_size - 1);
        }
    }

    // The ring starts at `head` when wrapped (oldest there), at 0 when not.
    const size_t start = wrapped ? head : 0;
    for (size_t i = src_skip; i < cur_size && out < dst_size - 1; ++i) {
        dst[out++] = ring[(start + i) % BUFFER_SIZE];
    }
    dst[out] = '\0';
    return out;
}

uint32_t SerialLog::bytes_written() const {
    return total_written.load(std::memory_order_relaxed);
}

void SerialLog::clear() {
    std::lock_guard lock { ring_mutex() };
    head = 0;
    wrapped = false;
}

SerialLog serial_log;

void serial_log_install_hook() {
    SerialUSB.lineBufferHook = &on_serial_line;
}

} // namespace nhttp::printer

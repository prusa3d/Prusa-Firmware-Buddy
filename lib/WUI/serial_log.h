#pragma once

#include <cstdint>
#include <cstddef>

namespace nhttp::printer {

/// Lock-protected ring buffer that captures Marlin's USB serial output.
/// Hooked into Arduino USBSerial::lineBufferHook so every complete line
/// emitted on the USB CDC also lands here.
///
/// Reads via snapshot_into() are safe from any task; writes happen from
/// whichever task drains the USB CDC line buffer.
class SerialLog {
public:
    static constexpr size_t BUFFER_SIZE = 8192;

    SerialLog();

    /// Append raw bytes to the ring. Safe to call from the USB drain task.
    /// Bytes overwrite the oldest data when the ring fills.
    void append(const uint8_t *data, size_t len);

    /// Copy out a snapshot of the current ring (oldest first), up to dst_size
    /// bytes. Returns the number of bytes copied. If the ring contains more
    /// than dst_size, the oldest entries are truncated and a marker
    /// "... (truncated)\n" is prepended.
    size_t snapshot_into(char *dst, size_t dst_size) const;

    /// Total bytes appended since boot (modular). Useful for incremental polling.
    uint32_t bytes_written() const;

    /// Reset the ring (does not reset bytes_written counter).
    void clear();
};

extern SerialLog serial_log;

/// Wire up the lineBufferHook on SerialUSB so output starts being captured.
/// Idempotent; safe to call multiple times.
void serial_log_install_hook();

} // namespace nhttp::printer

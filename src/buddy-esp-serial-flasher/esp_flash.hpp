#pragma once

namespace buddy_esp_serial_flasher {

enum class [[nodiscard]] Result {
    success = 0,
    not_connected,
    protocol_error,
    filesystem_error,
    checksum_mismatch,
    hal_error,
};

Result flash();

} // namespace buddy_esp_serial_flasher

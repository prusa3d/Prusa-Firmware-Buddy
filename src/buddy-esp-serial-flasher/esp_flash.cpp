#include <esp_flash.hpp>

extern "C" {
#include <esp_loader_io.h>
#include <esp_loader.h>
#include <slip.h>
}
#include <algorithm>
#include <cstring>
#include <gui/gui_bootstrap_screen.hpp>
#include <logging/log.hpp>
#include <option/has_embedded_esp32.h>
#include <option/has_gui.h>
#include <unique_file_ptr.hpp>
#include <tasks.hpp>
#include <charconv>
#include <array>
#include <str_utils.hpp>

LOG_COMPONENT_REF(Buddy);

namespace buddy_esp_serial_flasher {

constexpr size_t REFLASH_MAX_TRIES = 5;

struct Part {
    const char *filename;
    uintptr_t address;
    size_t size;
    size_t offset;
    uint8_t md5[16];
};

#include <esp_parts.gen>

using generic_start_function = esp_loader_error_t(uint32_t, uint32_t, uint32_t);
using generic_write_function = esp_loader_error_t(const void *, uint32_t);

class ProgressHookInterface {
public:
    virtual void report_progress(size_t increment) = 0;
};

class NullProgressHook final : public ProgressHookInterface {
public:
    void report_progress(size_t increment) final {};
};

class BootstrapProgressHook final : public ProgressHookInterface {
private:
    size_t total;
    size_t current;
    bool retry;

public:
    BootstrapProgressHook(size_t total, bool retry)
        : total { total }
        , current { 0 }
        , retry { retry } {
        report_progress(0);
    }
    void report_progress(size_t increment) final {
        current += increment;
        gui_bootstrap_screen_set_state(100 * current / total, retry ? "[ESP] Reflashing broken sectors" : "Flashing ESP");
    }
};

static Result generic_upload(const Part &part, generic_start_function start, generic_write_function write, ProgressHookInterface &progress_hook) {
    uint8_t buffer[esp::flash::buffer_size];

    unique_file_ptr file { fopen(part.filename, "rb") };
    if (file.get() == nullptr) {
        return Result::filesystem_error;
    }

    if (fseek(file.get(), part.offset, SEEK_SET)) {
        return Result::filesystem_error;
    }

    if (start(part.address, part.size, sizeof(buffer)) != ESP_LOADER_SUCCESS) {
        return Result::protocol_error;
    }

    size_t read_total = 0;

    while (read_total < part.size) {
        const size_t read_bytes = fread(buffer, 1, sizeof(buffer), file.get());

        if (ferror(file.get())) {
            return Result::filesystem_error;
        }

        if (write(buffer, read_bytes) != ESP_LOADER_SUCCESS) {
            return Result::protocol_error;
        }

        progress_hook.report_progress(read_bytes);
        read_total += read_bytes;
    }

    return Result::success;
}

[[maybe_unused]] // printers with embedded ESP32 don't need this
static Result
memory_upload(const Part &part) {
    log_info(Buddy, "memory_upload %s", part.filename);
    NullProgressHook progress_hook;
    return generic_upload(part, esp_loader_mem_start, esp_loader_mem_write, progress_hook);
}

static Result flash_upload(const Part &part, ProgressHookInterface &progress_hook) {
    log_info(Buddy, "flash_upload %s", part.filename);
    if (const Result result = generic_upload(part, esp_loader_flash_start, esp_loader_flash_write, progress_hook); result != Result::success) {
        return result;
    }
    return Result::success;
}

static Result verify_checksum(const Part &part) {
    log_info(Buddy, "verify %s", part.filename);

    // Obtain ESP region MD5 checksum
    std::array<char, 32> checksum_string;
    if (const auto result = esp_loader_md5_region(part.address, part.size, std::bit_cast<uint8_t *>(checksum_string.data())); result != ESP_LOADER_SUCCESS) {
        return Result::protocol_error;
    }
    // Convert hex string to MD5 data
    std::array<uint8_t, 16> checksum;
    for (uint8_t i = 0; i < 16; ++i) {
        const auto result = from_chars_light(checksum_string.data() + 2 * i, checksum_string.data() + 2 * i + 2, checksum.at(i), 16);
        if (result.ec != std::errc {}) {
            return Result::hal_error;
        }
    }

    if (memcmp(part.md5, checksum.data(), checksum.size()) != 0) {
        return Result::checksum_mismatch;
    }
    return Result::success;
}

static Result run_stub() {
#if HAS_EMBEDDED_ESP32()
    // embedded ESP32 already knows how to change baudrate and compute checksum
    // so we do nothing
#else
    // Upload stub to memory
    for (const Part &part : esp::flash::memory_parts) {
        if (const Result state = memory_upload(part); state != Result::success) {
            return state;
        }
    }

    // Jump to uploaded code
    if (esp_loader_mem_finish(esp::flash::memory_entry) != ESP_LOADER_SUCCESS) {
        return Result::protocol_error;
    }

    // Wait for the greeting from the stub
    uint8_t buffer[4];
    if (SLIP_receive_packet(buffer, 4) != ESP_LOADER_SUCCESS) {
        return Result::protocol_error;
    }
    if (memcmp(buffer, "OHAI", 4) != 0) {
        return Result::protocol_error;
    }
#endif
    return Result::success;
}

static Result run_rom() {
    // Esp32 doesn't require reset after flashing, but we should reset anyway to prevent connection problems over uart
    esp_loader_connect_args_t config = ESP_LOADER_CONNECT_DEFAULT();
    if (esp_loader_connect(&config) != ESP_LOADER_SUCCESS) {
        return Result::not_connected;
    }
    return Result::success;
}

static const uint32_t FLASH_UART_BAUDRATE = 115200;
static const uint32_t NIC_UART_BAUDRATE = 4600000;

[[maybe_unused]] // for now, this is not used because we do not need it, but it can be enabled
static Result
change_baudrate() {
    if (esp_loader_change_baudrate(FLASH_UART_BAUDRATE) != ESP_LOADER_SUCCESS) {
        return Result::protocol_error;
    }
    if (loader_port_change_transmission_rate(FLASH_UART_BAUDRATE) != ESP_LOADER_SUCCESS) {
        return Result::hal_error;
    }
    loader_port_delay_ms(30);
    return Result::success;
}

using FlashPartsResults = Result[sizeof(esp::flash::flash_parts) / sizeof(esp::flash::flash_parts[0])];

static bool all_parts_success(const FlashPartsResults &results) {
    return std::all_of(
        std::begin(results),
        std::end(results),
        [](const Result result) { return result == Result::success; });
}

static void foreach_flash_parts(FlashPartsResults &results, auto callback) {
    size_t index = 0;
    for (const Part &part : esp::flash::flash_parts) {
        callback(results[index++], part);
    }
}

static Result verify_all_flash_parts(FlashPartsResults &results) {
    if (const Result result = run_stub(); result != Result::success) {
        return result;
    }

    foreach_flash_parts(results, [](Result &result, const Part &part) {
        result = verify_checksum(part);
    });
    return Result::success;
}

Result flash() {
    esp_loader_connect_args_t config = ESP_LOADER_CONNECT_DEFAULT();
#if HAS_EMBEDDED_ESP32()
    config.trials = 40;
#endif
    if (esp_loader_connect(&config) != ESP_LOADER_SUCCESS) {
        return Result::not_connected;
    }

#if !HAS_EMBEDDED_ESP32()
    // With ESP8266 we need to wait for resources early to be able to flush the stub
    // TODO: Add more resources flags for esp and do a more granular dependency check
    TaskDeps::wait(TaskDeps::make(TaskDeps::Dependency::resources_ready));
#endif

    FlashPartsResults results;
    if (const Result result = verify_all_flash_parts(results); result != Result::success) {
        return result;
    }

    int tries = REFLASH_MAX_TRIES;
    while (tries > 0 && !all_parts_success(results)) {
        size_t total = 0;
        foreach_flash_parts(results, [&](Result &result, const Part &part) {
            if (result != Result::success) {
                total += part.size;
            }
        });

        // Flashing doesn't work in stub (wtf?) so let's just enter the original bootloader
        if (const Result result = run_rom(); result != Result::success) {
            return result;
        }

#if HAS_EMBEDDED_ESP32()
        // ESP32 doesn't need the stub so we can wait for resources here.
        // This makes the verification happend during the Install phase so we don't need to wait at the end for it
        TaskDeps::wait(TaskDeps::make(TaskDeps::Dependency::resources_ready));
#endif

        // Flash everything that needs to be flashed
        BootstrapProgressHook progress_hook { total, tries != REFLASH_MAX_TRIES };
        foreach_flash_parts(results, [&](Result &result, const Part &part) {
            if (result != Result::success) {
                result = flash_upload(part, progress_hook);
            }
        });

        if (auto response = esp_loader_flash_finish(false); response != ESP_LOADER_SUCCESS) {
            log_warning(Buddy, "flash finish (noreboot) failed with %d", response);
            if (response = esp_loader_flash_finish(true); response != ESP_LOADER_SUCCESS) {
                log_warning(Buddy, "flash finish (reboot) failed with %d", response);
            }
        }

        // For some reason we need to run ROM again in order to upload stub and verify (wtf?)
        if (const Result result = run_rom(); result != Result::success) {
            return result;
        }

        // Verify all parts again
        if (const Result result = verify_all_flash_parts(results); result != Result::success) {
            return result;
        }
        --tries;
    }

    if (tries == 0 && !all_parts_success(results)) {
        return Result::protocol_error;
    }

    if (loader_port_change_transmission_rate(NIC_UART_BAUDRATE) != ESP_LOADER_SUCCESS) {
        return Result::hal_error;
    }
    return Result::success;
}

} // namespace buddy_esp_serial_flasher

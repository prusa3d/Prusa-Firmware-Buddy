#include <sys/stat.h>
#include "error_codes.hpp"
#include "mbedtls/md5.h"
#include "bsod_gui.hpp"

#include <esp_flash.hpp>
#include <esp_loader.h>
#include <espif.h>
#include <unique_file_ptr.hpp>
#include <log.h>
#include <cstring>
#include <bsod.h>
#include "gui_bootstrap_screen.hpp"

LOG_COMPONENT_DEF(EspFlash, LOG_SEVERITY_DEBUG);

ESPFlash::ESPFlash()
    : firmware_set(FIRMWARE_SET)
    , state(State::Init)
    , total_size(0)
    , total_read(0) {
}

ESPFlash::State ESPFlash::flash() {
    // Get file sizes
    for (esp_fw_entry &fwpart : firmware_set) {
        struct stat fs;
        if (stat(fwpart.filename, &fs) == 0) {
            fwpart.size = fs.st_size;
            total_size += fwpart.size;
        }
    }
    update_progress();

    // Connect ESP
    for (int tries = retries;; tries--) {
        espif_flash_initialize(false);
        esp_loader_connect_args_t config = ESP_LOADER_CONNECT_DEFAULT();
        if (ESP_LOADER_SUCCESS == esp_loader_connect(&config)) {
            log_info(EspFlash, "ESP boot connect OK");
            state = State::Connected;
            break;
        }

        if (!tries) {
            log_debug(EspFlash, "ESP boot failed");
            state = State::NotConnected;
            goto end;
        }

        espif_flash_deinitialize();
    }

    // Flash all files
    for (esp_fw_entry &fwpart : firmware_set) {
        state = flash_part(fwpart);
        if (state != State::DataWritten) {
            goto end;
        }
    }

    // Finish flash
    log_info(EspFlash, "ESP finished flashing");
    state = State::Done;

end:
    esp_loader_flash_finish(true);
    espif_flash_deinitialize();
    update_progress();
    return state;
}

ESPFlash::State ESPFlash::flash_part(esp_fw_entry &fwpart) {
    // Generic data buffer used for flashing and checksum verification
    uint8_t buffer[buffer_length];

    // Open firmware file
    unique_file_ptr file;
    file.reset(fopen(fwpart.filename, "rb"));
    if (file.get() == nullptr) {
        log_error(EspFlash, "ESP flash: Unable to open file %s", fwpart.filename);
        return State::ReadError;
    }

    state = State::Checking;

    // Obtain ESP region MD5 checksum
    uint8_t checksum_string[32];
    auto err = esp_loader_md5_region(fwpart.address, fwpart.size, checksum_string);
    if (err != ESP_LOADER_SUCCESS) {
        log_error(EspFlash, "Failed to checksum ESP flash region");
    }
    // Convert hex string to MD5 data
    uint8_t checksum[16];
    for (uint i = 0; i < 16; ++i) {
        char byte[3] = { checksum_string[2 * i], checksum_string[2 * i + 1], 0 };
        checksum[i] = strtoul(byte, NULL, 16);
    }

    // Compute firmware file checksum
    mbedtls_md5_context md5;
    mbedtls_md5_init(&md5);
    mbedtls_md5_starts_ret(&md5);
    while (!feof(file.get())) {
        int read = fread(buffer, 1, std::min(fwpart.size, sizeof(buffer)), file.get());
        if (read > 0) {
            mbedtls_md5_update_ret(&md5, buffer, read);
        }

        if (ferror(file.get())) {
            log_error(EspFlash, "ESP flash: Unable to read file %s", fwpart.filename);
            mbedtls_md5_free(&md5);
            return State::ReadError;
        }
    }
    uint8_t source_checksum[16];
    mbedtls_md5_finish_ret(&md5, source_checksum);
    mbedtls_md5_free(&md5);

    // Skip flashing if checksum matches
    if (!memcmp(source_checksum, checksum, sizeof(checksum))) {
        state = State::DataWritten;
        return state;
    }

    // Start flash file
    rewind(file.get());
    log_info(EspFlash, "ESP Start flash %s", fwpart.filename);
    if (esp_loader_flash_start(fwpart.address, fwpart.size, buffer_length) != ESP_LOADER_SUCCESS) {
        log_error(EspFlash, "ESP flash: Unable to start flash on address %0xld", fwpart.address);
        return State::FlashError;
    }

    state = State::WriteData;

    // Flash file
    while (!feof(file.get())) {
        const size_t read_bytes = fread(buffer, 1, sizeof(buffer), file.get());
        total_read += read_bytes;
        log_debug(EspFlash, "ESP read data %ld", read_bytes);
        update_progress();

        if (ferror(file.get())) {
            log_error(EspFlash, "ESP flash: Unable to read file %s", fwpart.filename);
            return State::ReadError;
        }

        if (esp_loader_flash_write(buffer, read_bytes) != ESP_LOADER_SUCCESS) {
            log_error(EspFlash, "ESP flash write FAIL");
            return State::WriteError;
        }
    }

    state = State::DataWritten;

    return state;
}

void ESPFlash::update_progress() {
    uint8_t percent = total_size ? 100 * total_read / total_size : 0;
    const char *stage_description;
    switch (state) {
    case ESPFlash::State::Init:
        stage_description = "Connecting ESP";
        break;
    case ESPFlash::State::WriteData:
        stage_description = "Flashing ESP";
        break;
    case ESPFlash::State::Checking:
        stage_description = "Checking ESP";
        break;
    default:
        stage_description = "Unknown ESP state";
    }

    gui_bootstrap_screen_set_state(percent, stage_description);
}

void ESPFlash::fatal_err(const State state) {
    switch (state) {
    case State::ReadError:
        fatal_error(ErrCode::ERR_SYSTEM_ESP_FW_READ);
        break;
    case State::NotConnected:
        fatal_error(ErrCode::ERR_SYSTEM_ESP_NOT_CONNECTED);
        break;
    case State::WriteError:
    case State::FlashError:
        fatal_error(ErrCode::ERR_SYSTEM_ESP_COMMAND_ERR);
        break;
    default:
        fatal_error(ErrCode::ERR_SYSTEM_ESP_UNKNOWN_ERR);
    }
}

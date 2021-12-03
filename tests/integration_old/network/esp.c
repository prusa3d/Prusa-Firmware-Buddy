#include "lwesp_ll_buddy.h"

/**
 * This is not an automatic test, but rather this code can be used to debug ESP flashing. The real flash code is
 * interlevaed with GUI code and cannot be used for manual testing of flash process easily.
 */

typedef struct {
    size_t address;
    const char *filename;
    size_t size;
} esp_firmware_part;

#define BUFFER_LENGTH 512

esp_firmware_part firmware_set[] = {
    { .address = 0x00000ul, .filename = "/boot_v1.7_with_flash_params.bin", .size = 4080 },
    { .address = 0x01000ul, .filename = "/user1.1024.new.2.bin", .size = 413444 },
    { .address = 0x7e000ul, .filename = "/blank.bin", .size = 4096 },
    { .address = 0xfb000ul, .filename = "/blank.bin", .size = 4096 },
    { .address = 0xfc000ul, .filename = "/esp_init_data_default_v08.bin", .size = 128 },
    { .address = 0xfe000ul, .filename = "/blank.bin", .size = 4096 }
};

/**
 * This is a bare minimum ESP flash method. It is kept here for reference and
 * future use as the actually used implementation in screen_menu_esp_update.cpp
 * is hard to understand and not suitable for reuse.
 */
espr_t esp_flash() {
    espr_t flash_init_res = esp_flash_initialize();
    if (flash_init_res != espOK) {
        return flash_init_res;
    }
    esp_loader_connect_args_t config = ESP_LOADER_CONNECT_DEFAULT();
    _dbg("ESP boot connect");
    if (ESP_LOADER_SUCCESS != esp_loader_connect(&config)) {
        _dbg("ESP boot connect failed");
        return espERR;
    }
    _dbg("ESP connected");

    for (esp_firmware_part *current_part = &firmware_set[0]; current_part < &firmware_set[sizeof(firmware_set) / sizeof(esp_firmware_part)]; current_part++) {
        _dbg("ESP Start flash %s", current_part->filename);
        FIL file_descriptor;
        if (f_open(&file_descriptor, current_part->filename, FA_READ) != FR_OK) {
            _dbg("ESP flash: Unable to open file %s", current_part->filename);
            break;
        }

        if (esp_loader_flash_start(current_part->address, current_part->size, BUFFER_LENGTH) != ESP_LOADER_SUCCESS) {
            _dbg("ESP flash: Unable to start flash on address %0xld", current_part->address);
            f_close(&file_descriptor);
            break;
        }

        UINT readBytes = 0;
        uint8_t buffer[BUFFER_LENGTH];
        uint32_t readCount = 0;

        vTaskDelay(500 / portTICK_PERIOD_MS);

        do {
            FRESULT res = f_read(&file_descriptor, buffer, sizeof(buffer), &readBytes);
            readCount += readBytes;
            if (res != FR_OK) {
                _dbg("ESP flash: Unable to read file %s", current_part->filename);
                readBytes = 0;
            }
            esp_loader_error_t ret = ESP_LOADER_ERROR_FAIL;
            if (readBytes > 0) {
                ret = esp_loader_flash_write(buffer, readBytes);
                if (ret == ESP_LOADER_SUCCESS) {
                    _dbg("ESP flashed data %ld ending at %ld, flash ending at: %ld", readBytes, readCount, readCount + current_part->address);
                } else {
                    _dbg("ESP flash write FAIL: %d", ret);
                }
            }
        } while (readBytes > 0);

        _dbg("File finished");
        f_close(&file_descriptor);
    }

    return espOK;
}

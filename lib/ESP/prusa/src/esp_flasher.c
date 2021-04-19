#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include "serial_io.h"
#include "esp_loader.h"
#include "esp_flasher.h"
#include "dbg.h"

#ifndef SINGLE_TARGET_SUPPORT

    #define BOOTLOADER_ADDRESS_8266 0x1000
    #define BOOTLOADER_ADDRESS      0x1000
    #define PARTITION_ADDRESS       0x8000
    #define APPLICATION_ADDRESS     0x10000

extern const uint8_t ESP8266_bootloader_bin[];
extern const uint32_t ESP8266_bootloader_bin_size;
extern const uint8_t ESP8266_hello_world_bin[];
extern const uint32_t ESP8266_hello_world_bin_size;
extern const uint8_t ESP8266_partition_table_bin[];
extern const uint32_t ESP8266_partition_table_bin_size;

void get_esp8266_binaries(target_chip_t target, esp8266_binaries_t *bins) {
    bins->boot.data = ESP8266_bootloader_bin;
    bins->boot.size = ESP8266_bootloader_bin_size;
    bins->boot.addr = BOOTLOADER_ADDRESS_8266;
    bins->part.data = ESP8266_partition_table_bin;
    bins->part.size = ESP8266_partition_table_bin_size;
    bins->part.addr = PARTITION_ADDRESS;
    bins->app.data = ESP8266_hello_world_bin;
    bins->app.size = ESP8266_hello_world_bin_size;
    bins->app.addr = APPLICATION_ADDRESS;
}

#endif

esp_loader_error_t connect_to_target() {
    esp_loader_connect_args_t connect_config = ESP_LOADER_CONNECT_DEFAULT();

    // connect + sync
    esp_loader_error_t err = esp_loader_connect(&connect_config);
    if (err != ESP_LOADER_SUCCESS) {
        _dbg0("Cannot connect to target. Error: %u\n", err);
        return err;
    }
    _dbg0("Connected to target\n");

    // esp8266 chip only (ESP-01)
    if (esp_loader_get_target() != ESP8266_CHIP) {
      return ESP_LOADER_ERROR_UNSUPPORTED_CHIP;
    }

    return ESP_LOADER_SUCCESS;
}

esp_loader_error_t flash_binary(const uint8_t *bin, size_t size, size_t address) {
    esp_loader_error_t err;
    static uint8_t payload[1024];
    const uint8_t *bin_addr = bin;

    _dbg0("Erasing flash (this may take a while)...\n");
    err = esp_loader_flash_start(address, size, sizeof(payload));
    if (err != ESP_LOADER_SUCCESS) {
        _dbg0("Erasing flash failed with error %d.\n", err);
        return err;
    }
    _dbg0("Start programming\n");

    size_t binary_size = size;
    size_t written = 0;

    while (size > 0) {
        size_t to_read = MIN(size, sizeof(payload));
        memcpy(payload, bin_addr, to_read);

        err = esp_loader_flash_write(payload, to_read);
        if (err != ESP_LOADER_SUCCESS) {
            _dbg0("\nPacket could not be written! Error %d.\n", err);
            return err;
        }

        size -= to_read;
        bin_addr += to_read;
        written += to_read;

        int progress = (int)(((float)written / binary_size) * 100);
        _dbg0("\rProgress: %d %%", progress);
        fflush(stdout);
    };

    _dbg0("\nFinished programming\n");

#if MD5_ENABLED
    err = esp_loader_flash_verify();
    if (err == ESP_LOADER_ERROR_UNSUPPORTED_FUNC) {
        _dbg0("ESP8266 does not support flash verify command.");
        return err;
    } else if (err != ESP_LOADER_SUCCESS) {
        _dbg0("MD5 does not match. err: %d\n", err);
        return err;
    }
    _dbg0("Flash verified\n");
#endif

    return ESP_LOADER_SUCCESS;
}

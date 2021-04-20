#pragma once

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    const uint8_t *data;
    uint32_t size;
    uint32_t addr;
} partition_attr_t;

typedef struct {
    partition_attr_t boot;
    partition_attr_t user;
    partition_attr_t blank1;
    partition_attr_t init_data;
    partition_attr_t blank2;
    partition_attr_t blank3;
} esp8266_binaries_t;

void get_esp8266_binaries(esp8266_binaries_t *binaries);
esp_loader_error_t connect_to_target();
esp_loader_error_t flash_binary(const uint8_t *bin, size_t size, size_t address);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#pragma once

#include "lwesp/lwesp.h"
#include "lwesp/lwesp_private.h"
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// # Commands supported by ESP8266 ROM bootloader
#define ESP_FLASH_BEGIN      0x02  // LWESP_CMD_TCPIP_CIPSTART
#define ESP_FLASH_DATA       0x03  // LWESP_CMD_TCPIP_CIPSEND
#define ESP_FLASH_END        0x04  //
#define ESP_SYNC             0x08  // LWESP_CMD_TCPIP_CIPSTATUS
#define ESP_WRITE_REG        0x09  //
#define ESP_READ_REG         0x0a  // LWESP_CMD_TCPIP_CIFSR

#define ESP_CHECKSUM_MAGIC   0xfe  // initial state for checksum
#define ESP_FLASH_WRITE_SIZE 0x400 // write data packet size

void esp_upload_start();
lwespr_t lwespi_upload_cmd(lwesp_msg_t *msg);
lwespr_t lwespi_upload_process(const void *data, size_t data_len);

// -- logic sequence
// void esp_upload_sync();
// void esp_upload_get_chip_desc();
// void esp_upload_flash_begin();
// void esp_upload_flash_data();

typedef enum {
  ESP_BOOT_SYNC = 0,
  ESP_BOOT_
} lwesp_upload_state;

#ifdef __cplusplus
}
#endif /* __cplusplus */

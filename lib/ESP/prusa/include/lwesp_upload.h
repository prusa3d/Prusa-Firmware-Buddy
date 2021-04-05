#pragma once

#include "lwesp/lwesp.h"
#include "lwesp/lwesp_private.h"
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// # Commands supported by ESP8266 ROM bootloader
#define ESP_FLASH_BEGIN 0x02 // LWESP_CMD_TCPIP_CIPSTART
#define ESP_FLASH_DATA  0x03 // LWESP_CMD_TCPIP_CIPSEND
#define ESP_FLASH_END   0x04 //
#define ESP_SYNC        0x08 // LWESP_CMD_TCPIP_CIPSTATUS
#define ESP_WRITE_REG   0x09 //
#define ESP_READ_REG    0x0a // LWESP_CMD_TCPIP_CIFSR

void esp_upload_start();
lwespr_t lwespi_upload_cmd(lwesp_msg_t *msg);

// -- logic sequence
void esp_upload_sync();
void esp_upload_get_chip_desc();
void esp_upload_flash_begin();
void esp_upload_flash_data();

#ifdef __cplusplus
}
#endif /* __cplusplus */

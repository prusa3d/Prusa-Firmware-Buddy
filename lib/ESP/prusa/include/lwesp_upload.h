
#pragma once

#include "lwesp/lwesp.h"
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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

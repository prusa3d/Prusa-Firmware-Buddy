#ifndef ESP_HDR_CONFIG_H
#define ESP_HDR_CONFIG_H

/* Rename this file to "esp_config.h" for your application */

/* Increase default receive buffer length */
#define ESP_RCV_BUFF_SIZE                   0x800
 
/* After user configuration, call default config to merge config together */
#include "esp/esp_config_default.h"

#endif /* ESP_HDR_CONFIG_H */
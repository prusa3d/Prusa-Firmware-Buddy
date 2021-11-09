#include "esp/esp_debug.h"

/*
 * Print debug message to the screen
 * Trace message will be printed as it is enabled in types 
 * while state message will not be printed.
 */
ESP_DEBUGF(MY_DBG_MODULE | ESP_DBG_TYPE_TRACE, "This is trace message on my program\r\n");
ESP_DEBUGF(MY_DBG_MODULE | ESP_DBG_TYPE_STATE, "This is state message on my program\r\n");

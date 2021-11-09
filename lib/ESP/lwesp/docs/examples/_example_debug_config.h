/* Modifications of esp_config.h file for configuration */

/* Enable global debug */
#define ESP_CFG_DBG                             ESP_DBG_ON

/*
 * Enable debug types. 
 * You may use OR | to use multiple types: ESP_DBG_TYPE_TRACE | ESP_DBG_TYPE_STATE
 */
#define ESP_CFG_DBG_TYPES_ON                    ESP_DBG_TYPE_TRACE

/* Enable debug on custom module */
#define MY_DBG_MODULE                           ESP_DBG_ON

#include "esp/esp_includes.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup        LWESP Lightweight ESP-AT parser library
 * \brief           Lightweight ESP-AT parser
 * \{
 */

espr_t esp_init(esp_evt_fn cb_func, const uint32_t blocking);
espr_t esp_reset(const esp_api_cmd_evt_fn evt_fn, void *const evt_arg, const uint32_t blocking);
espr_t esp_reset_with_delay(uint32_t delay, const esp_api_cmd_evt_fn evt_fn, void *const evt_arg, const uint32_t blocking);

espr_t esp_restore(const esp_api_cmd_evt_fn evt_fn, void *const evt_arg, const uint32_t blocking);
espr_t esp_set_at_baudrate(uint32_t baud, const esp_api_cmd_evt_fn evt_fn, void *const evt_arg, const uint32_t blocking);
espr_t esp_set_wifi_mode(esp_mode_t mode, const esp_api_cmd_evt_fn evt_fn, void *const evt_arg, const uint32_t blocking);
espr_t esp_get_wifi_mode(esp_mode_t *mode, const esp_api_cmd_evt_fn evt_fn, void *const evt_arg, const uint32_t blocking);

espr_t esp_set_server(uint8_t en, esp_port_t port, uint16_t max_conn, uint16_t timeout, esp_evt_fn cb, const esp_api_cmd_evt_fn evt_fn, void *const evt_arg, const uint32_t blocking);

espr_t esp_update_sw(const esp_api_cmd_evt_fn evt_fn, void *const evt_arg, const uint32_t blocking);

espr_t esp_core_lock(void);
espr_t esp_core_unlock(void);

espr_t esp_device_set_present(uint8_t present, const esp_api_cmd_evt_fn evt_fn, void *const evt_arg, const uint32_t blocking);
uint8_t esp_device_is_present(void);

uint8_t esp_device_is_esp8266(void);

uint8_t esp_delay(const uint32_t ms);

uint8_t esp_get_current_at_fw_version(esp_sw_version_t *const version);

/**
 * \brief           Set and format major, minor and patch values to firmware version
 * \param[in]       v: Version output, pointer to \ref esp_sw_version_t structure
 * \param[in]       major_: Major version
 * \param[in]       minor_: Minor version
 * \param[in]       patch_: Patch version
 * \hideinitializer
 */
#define esp_set_fw_version(v, major_, minor_, patch_) \
    do {                                                \
        (v)->major = (major_);                          \
        (v)->minor = (minor_);                          \
        (v)->patch = (patch_);                          \
    } while (0)

/**
 * \brief           Get minimal AT version supported by library
 * \param[out]      v: Version output, pointer to \ref esp_sw_version_t structure
 * \hideinitializer
 */
#define esp_get_min_at_fw_version(v) esp_set_fw_version(v, LWESP_MIN_AT_VERSION_MAJOR_ESP8266, LWESP_MIN_AT_VERSION_MINOR_ESP8266, LWESP_MIN_AT_VERSION_PATCH_ESP8266)

#ifdef __cplusplus
}
#endif /* __cplusplus */

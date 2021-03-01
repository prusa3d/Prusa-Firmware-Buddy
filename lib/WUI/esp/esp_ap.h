#include "esp/esp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \ingroup         ESP
 * \defgroup        ESP_AP Access point
 * \brief           Access point
 * \{
 *
 * Functions to manage access point (AP) on ESP device.
 *
 * In order to be able to use AP feature, \ref ESP_CFG_MODE_ACCESS_POINT must be enabled.
 */

esp_res_t    esp_ap_getip(esp_ip_t* ip, esp_ip_t* gw, esp_ip_t* nm, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
esp_res_t    esp_ap_setip(const esp_ip_t* ip, const esp_ip_t* gw, const esp_ip_t* nm, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
esp_res_t    esp_ap_getmac(esp_mac_t* mac, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
esp_res_t    esp_ap_setmac(const esp_mac_t* mac, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);

esp_res_t    esp_ap_get_config(esp_ap_conf_t* ap_conf, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
esp_res_t    esp_ap_set_config(const char* ssid, const char* pwd, uint8_t ch, esp_ecn_t ecn, uint8_t max_sta, uint8_t hid, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);

esp_res_t    esp_ap_list_sta(esp_sta_t* sta, size_t stal, size_t* staf, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
esp_res_t    esp_ap_disconn_sta(const esp_mac_t* mac, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);

#ifdef __cplusplus
}
#endif /* __cplusplus */


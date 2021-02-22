#include "esp/esp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \ingroup         ESP
 * \defgroup        ESP_STA Station API
 * \brief           Station API
 * \{
 */

esp_res_t    esp_sta_join(const char* name, const char* pass, const esp_mac_t* mac, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
esp_res_t    esp_sta_quit(const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
esp_res_t    esp_sta_autojoin(uint8_t en, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
esp_res_t    esp_sta_reconnect_set_config(uint16_t interval, uint16_t rep_cnt, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
esp_res_t    esp_sta_getip(esp_ip_t* ip, esp_ip_t* gw, esp_ip_t* nm, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
esp_res_t    esp_sta_setip(const esp_ip_t* ip, const esp_ip_t* gw, const esp_ip_t* nm, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
esp_res_t    esp_sta_getmac(esp_mac_t* mac, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
esp_res_t    esp_sta_setmac(const esp_mac_t* mac, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);

uint8_t     esp_sta_has_ip(void);
uint8_t     esp_sta_is_joined(void);

esp_res_t    esp_sta_copy_ip(esp_ip_t* ip, esp_ip_t* gw, esp_ip_t* nm, uint8_t* is_dhcp);
esp_res_t    esp_sta_list_ap(const char* ssid, esp_ap_t* aps, size_t apsl, size_t* apf, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);
esp_res_t    esp_sta_get_ap_info(esp_sta_info_ap_t* info, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking);

uint8_t     esp_sta_is_ap_802_11b(esp_ap_t* ap);
uint8_t     esp_sta_is_ap_802_11g(esp_ap_t* ap);
uint8_t     esp_sta_is_ap_802_11n(esp_ap_t* ap);

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */


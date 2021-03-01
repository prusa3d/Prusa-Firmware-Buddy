#include "esp/esp_private.h"
#include "esp/esp_ap.h"
// #include "esp/esp_mem.h"
#include "esp_utils.h"

#if ESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__

/**
 * \brief           Get IP of access point
 * \param[out]      ip: Pointer to variable to write IP address
 * \param[out]      gw: Pointer to variable to write gateway address
 * \param[out]      nm: Pointer to variable to write netmask address
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref esp_res_t enumeration otherwise
 */
esp_res_t
esp_ap_getip(esp_ip_t* ip, esp_ip_t* gw, esp_ip_t* nm,
             const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CIPAP_GET;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_getip.ip = ip;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_getip.gw = gw;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_getip.nm = nm;

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 1000);
}

/**
 * \brief           Set IP of access point
 *
 * Configuration changes will be saved in the NVS area of ESP device.
 *
 * \param[in]       ip: Pointer to IP address
 * \param[in]       gw: Pointer to gateway address. Set to `NULL` to use default gateway
 * \param[in]       nm: Pointer to netmask address. Set to `NULL` to use default netmask
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref esp_res_t enumeration otherwise
 */
esp_res_t
esp_ap_setip(const esp_ip_t* ip, const esp_ip_t* gw, const esp_ip_t* nm,
             const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_ASSERT("ip != NULL", ip != NULL);

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CIPAP_SET;
    ESP_MEMCPY(&ESP_MSG_VAR_REF(msg).msg.sta_ap_setip.ip, ip, sizeof(*ip));
    ESP_MEMCPY(&ESP_MSG_VAR_REF(msg).msg.sta_ap_setip.gw, gw, sizeof(*gw));
    ESP_MEMCPY(&ESP_MSG_VAR_REF(msg).msg.sta_ap_setip.nm, nm, sizeof(*nm));

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 1000);
}

/**
 * \brief           Get MAC of access point
 * \param[out]      mac: Pointer to output variable to save MAC address
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref esp_res_t enumeration otherwise
 */
esp_res_t
esp_ap_getmac(esp_mac_t* mac,
              const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CIPAPMAC_GET;
    ESP_MSG_VAR_REF(msg).msg.sta_ap_getmac.mac = mac;

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 1000);
}

/**
 * \brief           Set MAC of access point
 *
 * Configuration changes will be saved in the NVS area of ESP device.
 *
 * \param[in]       mac: Pointer to variable with MAC address. Memory of at least 6 bytes is required
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref esp_res_t enumeration otherwise
 */
esp_res_t
esp_ap_setmac(const esp_mac_t* mac,
              const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_ASSERT("mac != NULL", mac != NULL);
    ESP_ASSERT("Bit 0 of byte 0 in AP MAC must be 0!", !(((uint8_t*)mac)[0] & 0x01));

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CIPAPMAC_SET;
    ESP_MEMCPY(&ESP_MSG_VAR_REF(msg).msg.sta_ap_setmac.mac, mac, sizeof(*mac));

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 1000);
}

/**
 * \brief           Configure access point
 *
 * Configuration changes will be saved in the NVS area of ESP device.
 *
 * \note            Before you can configure access point, ESP device must be in AP mode. Check \ref esp_set_wifi_mode for more information
 * \param[in]       ssid: SSID name of access point
 * \param[in]       pwd: Password for network. Either set it to `NULL` or less than `64` characters
 * \param[in]       ch: Wifi RF channel
 * \param[in]       ecn: Encryption type. Valid options are `OPEN`, `WPA_PSK`, `WPA2_PSK` and `WPA_WPA2_PSK`
 * \param[in]       max_sta: Maximal number of stations access point can accept. Valid between 1 and 10 stations
 * \param[in]       hid: Set to `1` to hide access point from public access
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref esp_res_t enumeration otherwise
 */
esp_res_t
esp_ap_set_config(const char* ssid, const char* pwd, uint8_t ch, esp_ecn_t ecn, uint8_t max_sta, uint8_t hid,
                 const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_ASSERT("ssid != NULL", ssid != NULL);
    ESP_ASSERT("pwd == NULL || (pwd && strlen(pwd) <= 64)", pwd == NULL || (pwd != NULL && strlen(pwd) <= 64));
    ESP_ASSERT("ecn == open || ecn == WPA_PSK || ecn == WPA2_PSK || ecn == WPA_WPA2_PSK",
               ecn == ESP_ECN_OPEN || ecn == ESP_ECN_WPA_PSK || ecn == ESP_ECN_WPA2_PSK || ecn == ESP_ECN_WPA_WPA2_PSK);
    ESP_ASSERT("ch <= 128", ch <= 128);
    ESP_ASSERT("1 <= max_sta <= 10", max_sta > 0 && max_sta <= 10);

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CWSAP_SET;
    ESP_MSG_VAR_REF(msg).msg.ap_conf.ssid = ssid;
    ESP_MSG_VAR_REF(msg).msg.ap_conf.pwd = pwd;
    ESP_MSG_VAR_REF(msg).msg.ap_conf.ch = ch;
    ESP_MSG_VAR_REF(msg).msg.ap_conf.ecn = ecn;
    ESP_MSG_VAR_REF(msg).msg.ap_conf.max_sta = max_sta;
    ESP_MSG_VAR_REF(msg).msg.ap_conf.hid = hid;

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 10000);
}

/**
 * \brief           Get configuration of Soft Access Point
 *
 * \note            Before you can get configuration access point, ESP device must be in AP mode. Check \ref esp_set_wifi_mode for more information
 * \param[out]      ap_conf: soft access point configuration
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref esp_res_t enumeration otherwise
 */
esp_res_t
esp_ap_get_config(esp_ap_conf_t* ap_conf, const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_ASSERT("ap_conf != NULL", ap_conf != NULL);

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CWSAP_GET;
    ESP_MSG_VAR_REF(msg).msg.ap_conf_get.ap_conf = ap_conf;

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 10000);
}

/**
 * \brief           List stations connected to access point
 * \param[in]       sta: Pointer to array of \ref esp_sta_t structure to fill with stations
 * \param[in]       stal: Number of array entries of sta parameter
 * \param[out]      staf: Number of stations connected to access point
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref esp_res_t enumeration otherwise
 */
esp_res_t
esp_ap_list_sta(esp_sta_t* sta, size_t stal, size_t* staf,
                const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_ASSERT("sta != NULL", sta != NULL);
    ESP_ASSERT("stal > 0", stal > 0);

    if (staf != NULL) {
        *staf = 0;
    }

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CWLIF;
    ESP_MSG_VAR_REF(msg).msg.sta_list.stas = sta;
    ESP_MSG_VAR_REF(msg).msg.sta_list.stal = stal;
    ESP_MSG_VAR_REF(msg).msg.sta_list.staf = staf;

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 1000);
}

/**
 * \brief           Disconnects connected station from SoftAP access point
 * \param[in]       mac: Device MAC address to disconnect.
 *                      Application may use \ref esp_ap_list_sta to obtain list of connected stations to SoftAP.
 * \param[in]       evt_fn: Callback function called when command has finished. Set to `NULL` when not used
 * \param[in]       evt_arg: Custom argument for event callback function
 * \param[in]       blocking: Status whether command should be blocking or not
 * \return          \ref espOK on success, member of \ref esp_res_t enumeration otherwise
 */
esp_res_t
esp_ap_disconn_sta(const esp_mac_t* mac,
                   const esp_api_cmd_evt_fn evt_fn, void* const evt_arg, const uint32_t blocking) {
    ESP_MSG_VAR_DEFINE(msg);

    ESP_ASSERT("mac != NULL", mac != NULL);

    ESP_MSG_VAR_ALLOC(msg, blocking);
    ESP_MSG_VAR_SET_EVT(msg, evt_fn, evt_arg);
    ESP_MSG_VAR_REF(msg).cmd_def = ESP_CMD_WIFI_CWQIF;
    ESP_MEMCPY(&ESP_MSG_VAR_REF(msg).msg.ap_disconn_sta.mac, mac, sizeof(*mac));

    return espi_send_msg_to_producer_mbox(&ESP_MSG_VAR_REF(msg), espi_initiate_cmd, 1000);
}

#endif /* ESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__ */

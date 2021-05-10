#include "esp.h"
#include "esp/esp_includes.h"
#include "dbg.h"

#define MAX_TIMEOUT_ATTEMPTS (2UL)

/**
 * \brief           List of access points found by ESP device
 */
static esp_ap_t aps[32];

/**
 * \brief           Number of valid access points in \ref aps array
 */
static size_t apf;

//static esp_sta_info_ap_t connected_ap_info;

/**
 * \brief           Event callback function for ESP stack
 * \param[in]       evt: Event information with data
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
// static espr_t
// esp_callback_func(esp_evt_t *evt) {
//     switch (esp_evt_get_type(evt)) {
//     case ESP_EVT_AT_VERSION_NOT_SUPPORTED: {
//         esp_sw_version_t v_min, v_curr;

//         esp_get_min_at_fw_version(&v_min);
//         esp_get_current_at_fw_version(&v_curr);

//         _dbg("Current ESP8266 AT version is not supported by library!");
//         _dbg("Minimum required AT version is: %d.%d.%d", (int)v_min.major, (int)v_min.minor, (int)v_min.patch);
//         _dbg("Current AT version is: %d.%d.%d", (int)v_curr.major, (int)v_curr.minor, (int)v_curr.patch);
//         break;
//     }
//     case ESP_EVT_INIT_FINISH: {
//         _dbg("ESP_EVT_INIT_FINISH");
//         break;
//     }
//     case ESP_EVT_RESET: {
//         _dbg("ESP_EVT_RESET");
//         break;
//     }
//     case ESP_EVT_RESET_DETECTED: {
//         _dbg("ESP_EVT_RESET_DETECTED");
//         break;
//     }
//     case ESP_EVT_WIFI_GOT_IP: {
//         _dbg("ESP_EVT_WIFI_GOT_IP");
//         esp_set_wifi_mode(ESP_MODE_STA, NULL, NULL, 0);
//         break;
//     }
//     case ESP_EVT_WIFI_CONNECTED: {
//         _dbg("ESP_EVT_WIFI_CONNECTED");
//         esp_sta_get_ap_info(&connected_ap_info, NULL, NULL, 0);
//         break;
//     }
//     case ESP_EVT_WIFI_DISCONNECTED: {
//         _dbg("ESP_EVT_WIFI_DISCONNECTED");
//         break;
//     }
//     default:
//         break;
//     }
//     return espOK;
// }

uint32_t esp_present(uint32_t on) {
    // lwespr_t eres;
    // uint32_t tried = MAX_TIMEOUT_ATTEMPTS;
    // lwesp_mode_t mode = ESP_MODE_STA_AP;

    // eres = lwesp_device_set_present(on, NULL, NULL, 1);
    // while (on && mode != ESP_MODE_STA && tried) {
    //     eres = lwesp_set_wifi_mode(ESP_MODE_STA, NULL, NULL, 1);
    //     if (eres != lwespOK) {
    //         _dbg("Unable to set wifi mode : %d", eres);
    //         --tried;
    //     }
    //     lwesp_get_wifi_mode(&mode, NULL, NULL, 1);
    // }

    // if(!tried || !on) {
    //     eres = esp_device_set_present(0, NULL, NULL, 1);
    //     return 0UL;
    // }
    return 1UL;
}

uint32_t esp_is_device_presented() {
    return esp_device_is_present();
}

uint32_t esp_initialize() {
    espr_t eres;
    eres = esp_init(NULL, 1);
    if (eres != espOK) {
        return eres;
    }
    //    eres = esp_device_set_present(0, NULL, NULL, 1);
    return eres;
}

uint32_t esp_connect_to_AP(const ap_entry_t *preferead_ap) {
    espr_t eres;
    uint32_t tried = MAX_TIMEOUT_ATTEMPTS;

    /*
     * Scan for network access points
     * In case we have access point,
     * try to connect to known AP
     */
    do {

        /* Scan for access points visible to ESP device */
        if ((eres = esp_sta_list_ap(NULL, aps, ESP_ARRAYSIZE(aps), &apf, NULL, NULL, 1)) == espOK) {
            tried = 0;
            /* Process array of preferred access points with array of found points */
            for (size_t i = 0; i < ESP_ARRAYSIZE(aps); i++) {
                if (!strcmp(aps[i].ssid, preferead_ap->ssid)) {
                    tried = 1;
                    _dbg("Connecting to \"%s\" network...", preferead_ap->ssid);
                    /* Try to join to access point */
                    if ((eres = esp_sta_join(preferead_ap->ssid, preferead_ap->pass, NULL, 0, NULL, NULL, 1)) == espOK) {
                        esp_ip_t ip, gw, mask;
                        esp_sta_getip(&ip, &gw, &mask, 0, NULL, NULL, 1);
                        esp_sta_copy_ip(&ip, NULL, NULL);
                        _dbg("Connected to %s network!", preferead_ap->ssid);
                        _dbg("Station IP address: %d.%d.%d.%d",
                            (int)ip.ip[0], (int)ip.ip[1], (int)ip.ip[2], (int)ip.ip[3]);
                        _dbg("Station gateway address: %d.%d.%d.%d",
                            (int)gw.ip[0], (int)gw.ip[1], (int)gw.ip[2], (int)gw.ip[3]);
                        _dbg("Station mask address: %d.%d.%d.%d",
                            (int)mask.ip[0], (int)mask.ip[1], (int)mask.ip[2], (int)mask.ip[3]);
                        return (uint32_t)espOK;
                    } else {
                        _dbg("Connection error: %d", (int)eres);
                    }
                }
            }
            if (!tried) {
                _dbg("No access points available with preferred SSID: %s!", preferead_ap->ssid);
            }
        } else if (eres == espERRNODEVICE) {
            _dbg("Device is not present!");
            break;
        } else {
            _dbg("Error on WIFI scan procedure!");
        }
    } while (esp_is_device_presented());
    return (uint32_t)espERR;
}

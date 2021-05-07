#include "esp.h"
#include "lwesp/lwesp_includes.h"
#include "dbg.h"

#define MAX_TIMEOUT_ATTEMPTS (2UL)

/**
 * \brief           List of access points found by ESP device
 */
static lwesp_ap_t aps[32];

/**
 * \brief           Number of valid access points in \ref aps array
 */
static size_t apf;

static lwesp_sta_info_ap_t connected_ap_info;

/**
 * \brief           Event callback function for ESP stack
 * \param[in]       evt: Event information with data
 * \return          \ref lwespOK on success, member of \ref lwespr_t otherwise
 */
static lwespr_t
lwesp_callback_func(lwesp_evt_t *evt) {
    switch (lwesp_evt_get_type(evt)) {
    case LWESP_EVT_AT_VERSION_NOT_SUPPORTED: {
        lwesp_sw_version_t v_min, v_curr;

        lwesp_get_min_at_fw_version(&v_min);
        lwesp_get_current_at_fw_version(&v_curr);

        _dbg("Current ESP8266 AT version is not supported by library!");
        _dbg("Minimum required AT version is: %d.%d.%d", (int)v_min.major, (int)v_min.minor, (int)v_min.patch);
        _dbg("Current AT version is: %d.%d.%d", (int)v_curr.major, (int)v_curr.minor, (int)v_curr.patch);
        break;
    }
    case LWESP_EVT_INIT_FINISH: {
        _dbg("LWESP_EVT_INIT_FINISH");
        break;
    }
    case LWESP_EVT_RESET: {
        _dbg("LWESP_EVT_RESET");
        break;
    }
    case LWESP_EVT_RESET_DETECTED: {
        _dbg("LWESP_EVT_RESET_DETECTED");
        break;
    }
    case LWESP_EVT_WIFI_GOT_IP: {
        _dbg("LWESP_EVT_WIFI_GOT_IP");
        lwesp_set_wifi_mode(LWESP_MODE_STA, NULL, NULL, 0);
        break;
    }
    case LWESP_EVT_WIFI_CONNECTED: {
        _dbg("LWESP_EVT_WIFI_CONNECTED");
        lwesp_sta_get_ap_info(&connected_ap_info, NULL, NULL, 0);
        break;
    }
    case LWESP_EVT_WIFI_DISCONNECTED: {
        _dbg("LWESP_EVT_WIFI_DISCONNECTED");
        break;
    }
    case LWESP_EVT_CMD_TIMEOUT: {
        _dbg("LWESP_EVT_CMD_TIMEOUT");
        break;
    }
    default:
        break;
    }
    return lwespOK;
}

uint32_t esp_present(uint32_t on) {
    // lwespr_t eres;
    // uint32_t tried = MAX_TIMEOUT_ATTEMPTS;
    // lwesp_mode_t mode = LWESP_MODE_STA_AP;

    // eres = lwesp_device_set_present(on, NULL, NULL, 1);
    // while (on && mode != LWESP_MODE_STA && tried) {
    //     eres = lwesp_set_wifi_mode(LWESP_MODE_STA, NULL, NULL, 1);
    //     if (eres != lwespOK) {
    //         _dbg("Unable to set wifi mode : %d", eres);
    //         --tried;
    //     }
    //     lwesp_get_wifi_mode(&mode, NULL, NULL, 1);
    // }

    // if(!tried || !on) {
    //     eres = lwesp_device_set_present(0, NULL, NULL, 1);
    //     return 0UL;
    // }
    return 1UL;
}

uint32_t esp_is_device_presented() {
    return lwesp_device_is_present();
}

uint32_t esp_get_device_model() {
    return LWESP_DEVICE_ESP8266;
}

uint32_t esp_initialize() {
    lwespr_t eres;
    eres = lwesp_init(lwesp_callback_func, 0);
    if (eres != lwespOK) {
        return eres;
    }
    //    eres = lwesp_device_set_present(0, NULL, NULL, 1);
    return eres;
}

uint32_t esp_connect_to_AP(const ap_entry_t *preferead_ap) {
    lwespr_t eres;
    uint32_t tried = MAX_TIMEOUT_ATTEMPTS;

    /*
     * Scan for network access points
     * In case we have access point,
     * try to connect to known AP
     */
    do {

        /* Scan for access points visible to ESP device */
        if ((eres = lwesp_sta_list_ap(NULL, aps, LWESP_ARRAYSIZE(aps), &apf, NULL, NULL, 1)) == lwespOK) {
            tried = 0;
            /* Process array of preferred access points with array of found points */
            for (size_t i = 0; i < LWESP_ARRAYSIZE(aps); i++) {
                if (!strcmp(aps[i].ssid, preferead_ap->ssid)) {
                    tried = 1;
                    _dbg("Connecting to \"%s\" network...", preferead_ap->ssid);
                    /* Try to join to access point */
                    if ((eres = lwesp_sta_join(preferead_ap->ssid, preferead_ap->pass, NULL, NULL, NULL, 1)) == lwespOK) {
                        lwesp_ip_t ip, gw, mask;
                        uint8_t is_dhcp;
                        lwesp_sta_getip(&ip, &gw, &mask, NULL, NULL, 1);
                        lwesp_sta_copy_ip(&ip, NULL, NULL, &is_dhcp);
                        _dbg("Connected to %s network!", preferead_ap->ssid);
                        _dbg("Station IP address: %d.%d.%d.%d",
                            (int)ip.ip[0], (int)ip.ip[1], (int)ip.ip[2], (int)ip.ip[3]);
                        _dbg("Station gateway address: %d.%d.%d.%d",
                            (int)gw.ip[0], (int)gw.ip[1], (int)gw.ip[2], (int)gw.ip[3]);
                        _dbg("Station mask address: %d.%d.%d.%d",
                            (int)mask.ip[0], (int)mask.ip[1], (int)mask.ip[2], (int)mask.ip[3]);
                        return (uint32_t)lwespOK;
                    } else {
                        _dbg("Connection error: %d", (int)eres);
                    }
                }
            }
            if (!tried) {
                _dbg("No access points available with preferred SSID: %s!", preferead_ap->ssid);
            }
        } else if (eres == lwespERRNODEVICE) {
            _dbg("Device is not present!");
            break;
        } else {
            _dbg("Error on WIFI scan procedure!");
        }
    } while (esp_is_device_presented());
    return (uint32_t)lwespERR;
}

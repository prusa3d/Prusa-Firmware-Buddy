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

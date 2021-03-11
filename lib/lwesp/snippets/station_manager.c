#include "station_manager.h"
#include "lwesp/lwesp.h"

/*
 * List of preferred access points for ESP device
 * SSID and password
 *
 * ESP will try to scan for access points
 * and then compare them with the one on the list below
 */
ap_entry_t
ap_list[] = {
    //{ "SSID name", "SSID password" },
    { "TilenM_ST", "its private" },
    { "Majerle WIFI", "majerle_internet_private" },
    { "Majerle AMIS", "majerle_internet_private" },
};

/**
 * \brief           List of access points found by ESP device
 */
static
lwesp_ap_t aps[100];

/**
 * \brief           Number of valid access points in \ref aps array
 */
static
size_t apf;

/**
 * \brief           Connect to preferred access point
 *
 * \note            List of access points should be set by user in \ref ap_list structure
 * \param[in]       unlimited: When set to 1, function will block until SSID is found and connected
 * \return          \ref lwespOK on success, member of \ref lwespr_t enumeration otherwise
 */
lwespr_t
connect_to_preferred_access_point(uint8_t unlimited) {
    lwespr_t eres;
    uint8_t tried;

    /*
     * Scan for network access points
     * In case we have access point,
     * try to connect to known AP
     */
    do {
        if (lwesp_sta_has_ip()) {
            return lwespOK;
        }

        /* Scan for access points visible to ESP device */
        printf("Scanning access points...\r\n");
        if ((eres = lwesp_sta_list_ap(NULL, aps, LWESP_ARRAYSIZE(aps), &apf, NULL, NULL, 1)) == lwespOK) {
            tried = 0;
            /* Print all access points found by ESP */
            for (size_t i = 0; i < apf; i++) {
                printf("AP found: %s, CH: %d, RSSI: %d\r\n", aps[i].ssid, aps[i].ch, aps[i].rssi);
            }

            /* Process array of preferred access points with array of found points */
            for (size_t j = 0; j < LWESP_ARRAYSIZE(ap_list); j++) {
                for (size_t i = 0; i < apf; i++) {
                    if (!strcmp(aps[i].ssid, ap_list[j].ssid)) {
                        tried = 1;
                        printf("Connecting to \"%s\" network...\r\n", ap_list[j].ssid);
                        /* Try to join to access point */
                        if ((eres = lwesp_sta_join(ap_list[j].ssid, ap_list[j].pass, NULL, NULL, NULL, 1)) == lwespOK) {
                            lwesp_ip_t ip;
                            uint8_t is_dhcp;
                            lwesp_sta_copy_ip(&ip, NULL, NULL, &is_dhcp);

                            printf("Connected to %s network!\r\n", ap_list[j].ssid);
                            printf("Station IP address: %d.%d.%d.%d; Is DHCP: %d\r\n",
                                   (int)ip.ip[0], (int)ip.ip[1], (int)ip.ip[2], (int)ip.ip[3], (int)is_dhcp);
                            return lwespOK;
                        } else {
                            printf("Connection error: %d\r\n", (int)eres);
                        }
                    }
                }
            }
            if (!tried) {
                printf("No access points available with preferred SSID!\r\nPlease check station_manager.c file and edit preferred SSID access points!\r\n");
            }
        } else if (eres == lwespERRNODEVICE) {
            printf("Device is not present!\r\n");
            break;
        } else {
            printf("Error on WIFI scan procedure!\r\n");
        }
        if (!unlimited) {
            break;
        }
    } while (1);
    return lwespERR;
}

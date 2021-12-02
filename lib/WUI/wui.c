/*
 * wui.h
 * \brief main interface functions for Web User Interface (WUI) thread
 *
 *  Created on: Dec 12, 2019
 *      Author: joshy
  *  Modify on 09/17/2021
 *      Author: Marek Mosna <marek.mosna[at]prusa3d.cz>
*/

#include "wui.h"

#include "marlin_client.h"
#include "wui_api.h"
#include "ethernetif.h"
#include "stm32f4xx_hal.h"

#include "sntp_client.h"
#include "dbg.h"

#include <lwip/ip.h>
#include <lwip/tcp.h>
#include <lwip/altcp_tcp.h>
#include "esp_tcp.h"
#include "http/httpd.h"
#include "http_handler_default.h"
#include "main.h"

#include "netdev.h"

#include <string.h>
#include "eeprom.h"
#include "variant8.h"

#define LOOP_EVT_TIMEOUT           500UL
#define IS_TIME_TO_CHECK_ESP(time) (((time) % 1000) == 0)

extern RNG_HandleTypeDef hrng;

osMessageQDef(networkMbox, 16, NULL);
osMessageQId networkMbox_id;

static variant8_t prusa_link_api_key;

const char *wui_generate_api_key(char *api_key, uint32_t length) {
    // Avoid confusing character pairs ‒ 1/l/I, 0/O.
    static char charset[] = "abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    // One less, as the above contains '\0' at the end which we _do not_ want to generate.
    const uint32_t charset_length = sizeof(charset) / sizeof(char) - 1;
    uint32_t i = 0;

    while (i < length - 1) {
        uint32_t random = 0;
        HAL_StatusTypeDef status = HAL_RNG_GenerateRandomNumber(&hrng, &random);
        if (HAL_OK == status) {
            api_key[i++] = charset[random % charset_length];
        }
    }
    api_key[i] = 0;
    return api_key;
}

void wui_store_api_key(char *api_key, uint32_t length) {
    variant8_t *p_prusa_link_api_key = &prusa_link_api_key;
    variant8_done(&p_prusa_link_api_key);
    prusa_link_api_key = variant8_init(VARIANT8_PCHAR, length, api_key);
    eeprom_set_var(EEVAR_PL_API_KEY, prusa_link_api_key);
}

void StartWebServerTask(void const *argument) {
    uint32_t esp_check_counter = 1;
    _dbg("wui starts");

    networkMbox_id = osMessageCreate(osMessageQ(networkMbox), NULL);
    if (networkMbox_id == NULL) {
        _dbg("networkMbox was not created");
        return;
    }

    netdev_init();

    prusa_link_api_key = eeprom_get_var(EEVAR_PL_API_KEY);
    if (!strcmp(variant8_get_pch(prusa_link_api_key), "")) {
        char api_key[PL_API_KEY_SIZE] = { 0 };
        wui_generate_api_key(api_key, PL_API_KEY_SIZE);
        wui_store_api_key(api_key, PL_API_KEY_SIZE);
    }

    if (variant8_get_ui8(eeprom_get_var(EEVAR_PL_RUN)) == 1) {
        httpd_init(&default_http_handlers);
    }

    for (;;) {
        osEvent evt = osMessageGet(networkMbox_id, LOOP_EVT_TIMEOUT);
        if (evt.status == osEventMessage) {
            switch (evt.value.v) {
            case EVT_TCPIP_INIT_FINISHED:
                netdev_set_up(NETDEV_ETH_ID);
                break;
            case EVT_LWESP_INIT_FINISHED:
                netdev_set_up(NETDEV_ESP_ID);
                break;
            case EVT_NETDEV_INIT_FINISHED(NETDEV_ETH_ID, 0):
                if (netdev_get_ip_obtained_type(NETDEV_ETH_ID) == NETDEV_DHCP) {
                    netdev_set_dhcp(NETDEV_ETH_ID);
                } else {
                    netdev_set_static(NETDEV_ETH_ID);
                }
                break;
            case EVT_NETDEV_INIT_FINISHED(NETDEV_ESP_ID, 0):
                if (netdev_get_ip_obtained_type(NETDEV_ESP_ID) == NETDEV_STATIC) {
                    netdev_set_static(NETDEV_ESP_ID);
                }
                break;
            default:
                break;
            }
        }

        if (netdev_get_status(NETDEV_ESP_ID) == NETDEV_NETIF_DOWN && IS_TIME_TO_CHECK_ESP(esp_check_counter * LOOP_EVT_TIMEOUT)) {
            netdev_check_link(NETDEV_ESP_ID);
            esp_check_counter = 0;
        } else {
            ++esp_check_counter;
        }
    }
}

const char *wui_get_api_key() {
    return variant8_get_pch(prusa_link_api_key);
}

struct altcp_pcb *prusa_alloc(void *arg, uint8_t ip_type) {
    uint32_t active_device_id = netdev_get_active_id();
    if (active_device_id == NETDEV_ETH_ID) {
        struct altcp_pcb *result = altcp_tcp_new_ip_type(ip_type);
        /*
         * Hack:
         * We need the option for listening sockets, because it otherwise
         * breaks if we turn the interface down and up again (and therefore
         * close and open a listening socket on the same port). Apparently,
         * setting interface down does not kill/wipe all its connections/puts
         * them into TCP_WAIT state.
         *
         * This should do nothing for connecting sockets, so it's fine to set.
         *
         * This is the wrong place to do it, but there's no other as other
         * places can't know for sure the actide device didn't change.
         */
        if (result != NULL) {
            ip_set_option((struct tcp_pcb *)result->state, SOF_REUSEADDR);
        }
        return result;
    } else if (active_device_id == NETDEV_ESP_ID) {
        return altcp_esp_new_ip_type(ip_type);
    } else {
        return NULL;
    }
}

/*
 * wui.h
 * \brief main interface functions for Web User Interface (WUI) thread
 *
 *  Created on: Dec 12, 2019
 *      Author: joshy
 */

#include "wui.h"
#include "wui_vars.h"
#include "marlin_client.h"
#include "wui_api.h"
#include "ethernetif.h"

#include "sntp_client.h"
#include "dbg.h"

#include "lwip/altcp_tcp.h"
#include "esp_tcp.h"
#include "httpd.h"

#include "netdev.h"

#include <string.h>

#define LOOP_EVT_TIMEOUT           500UL
#define IS_TIME_TO_CHECK_ESP(time) (((time) % 1000) == 0)

osMessageQDef(networkMbox, 16, NULL);
osMessageQId networkMbox_id;

// WUI thread mutex for updating marlin vars
osMutexDef(wui_thread_mutex);
osMutexId(wui_thread_mutex_id);

static marlin_vars_t *wui_marlin_vars;
wui_vars_t wui_vars;                              // global vriable for data relevant to WUI
static char wui_media_LFN[FILE_NAME_MAX_LEN + 1]; // static buffer for gcode file name
static char *api_key = "miniPL-apikey";

static void wui_marlin_client_init(void) {
    wui_marlin_vars = marlin_client_init(); // init the client
    // force update variables when starts
    marlin_client_set_event_notify(MARLIN_EVT_MSK_DEF - MARLIN_EVT_MSK_FSM, NULL);
    marlin_client_set_change_notify(MARLIN_VAR_MSK_DEF | MARLIN_VAR_MSK_WUI, NULL);
    if (wui_marlin_vars) {
        wui_marlin_vars->media_LFN = wui_media_LFN;
    }
}

static void sync_with_marlin_server(void) {
    if (wui_marlin_vars) {
        marlin_client_loop();
    } else {
        return;
    }
    osMutexWait(wui_thread_mutex_id, osWaitForever);
    for (int i = 0; i < 4; i++) {
        wui_vars.pos[i] = wui_marlin_vars->pos[i];
    }
    wui_vars.temp_nozzle = wui_marlin_vars->temp_nozzle;
    wui_vars.temp_bed = wui_marlin_vars->temp_bed;
    wui_vars.target_nozzle = wui_marlin_vars->target_nozzle;
    wui_vars.target_bed = wui_marlin_vars->target_bed;
    wui_vars.fan_speed = wui_marlin_vars->fan_speed;
    wui_vars.print_speed = wui_marlin_vars->print_speed;
    wui_vars.flow_factor = wui_marlin_vars->flow_factor;
    wui_vars.print_dur = wui_marlin_vars->print_duration;
    wui_vars.sd_precent_done = wui_marlin_vars->sd_percent_done;
    wui_vars.sd_printing = wui_marlin_vars->sd_printing;
    wui_vars.time_to_end = wui_marlin_vars->time_to_end;
    wui_vars.print_state = wui_marlin_vars->print_state;
    if (marlin_change_clr(MARLIN_VAR_FILENAME)) {
        strlcpy(wui_vars.gcode_name, wui_marlin_vars->media_LFN, FILE_NAME_MAX_LEN);
    }

    osMutexRelease(wui_thread_mutex_id);
}

void StartWebServerTask(void const *argument) {
    uint32_t esp_check_counter = 1;
    _dbg("wui starts");
    networkMbox_id = osMessageCreate(osMessageQ(networkMbox), NULL);
    if (networkMbox_id == NULL) {
        _dbg("networkMbox was not created");
        return;
    }

    // mutex for passing marlin variables to tcp thread
    wui_thread_mutex_id = osMutexCreate(osMutex(wui_thread_mutex));
    if (wui_thread_mutex_id == NULL) {
        _dbg("wui_thread_mutex was not created");
        return;
    }

    wui_marlin_client_init();
    netdev_init();
    httpd_init();

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
                if (netdev_get_ip_obtained_type() == NETDEV_DHCP) {
                    netdev_set_dhcp(NETDEV_ETH_ID);
                } else {
                    netdev_set_static(NETDEV_ETH_ID);
                }
                break;
            case EVT_NETDEV_INIT_FINISHED(NETDEV_ESP_ID, 0):
                if (netdev_get_ip_obtained_type() == NETDEV_DHCP) {
                    netdev_set_dhcp(NETDEV_ESP_ID);
                } else {
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

        sync_with_marlin_server();
    }
}

const char *wui_get_api_key() {
    return api_key;
}

struct altcp_pcb *prusa_alloc(void *arg, uint8_t ip_type) {
    uint32_t active_device_id = netdev_get_active_id();
    if (active_device_id == NETDEV_ETH_ID)
        return altcp_tcp_new_ip_type(ip_type);
    else if (active_device_id == NETDEV_ESP_ID) {
        return altcp_esp_new_ip_type(ip_type);
    } else {
        return NULL;
    }
}

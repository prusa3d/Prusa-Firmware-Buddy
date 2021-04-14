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
#include "lwip.h"
#include "ethernetif.h"
#include <string.h>
#include "sntp_client.h"
#include "dbg.h"
#include "lwesp/lwesp.h"

#define WUI_NETIF_SETUP_DELAY  1000
#define WUI_COMMAND_QUEUE_SIZE WUI_WUI_MQ_CNT // maximal number of messages at once in WUI command messageQ

// WUI thread mutex for updating marlin vars
osMutexDef(wui_thread_mutex);
osMutexId(wui_thread_mutex_id);

static marlin_vars_t *wui_marlin_vars;
wui_vars_t wui_vars;                              // global vriable for data relevant to WUI
static char wui_media_LFN[FILE_NAME_MAX_LEN + 1]; // static buffer for gcode file name

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

static void update_eth_changes(void) {
    wui_lwip_link_status(); // checks plug/unplug status and take action
    wui_lwip_sync_gui_lan_settings();
}

static lwespr_t conn_upload_callback_func(lwesp_evt_t *evt);

void StartWebServerTask(void const *argument) {
    // get settings from ini file
    osDelay(1000);
    printf("wui starts");
    if (load_ini_file(&wui_eth_config)) {
        save_eth_params(&wui_eth_config);
    }
    wui_eth_config.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
    load_eth_params(&wui_eth_config);
    // mutex for passing marlin variables to tcp thread
    wui_thread_mutex_id = osMutexCreate(osMutex(wui_thread_mutex));
    // marlin client initialization for WUI
    wui_marlin_client_init();
    // LwIP related initalizations
    MX_LWIP_Init(&wui_eth_config);
    http_server_init();
    sntp_client_init();
    osDelay(WUI_NETIF_SETUP_DELAY); // wait for all settings to take effect
    // lwesp stuffs
    if (lwesp_init(NULL, 1) != lwespOK) {
        printf("Cannot initialize LwESP!\r\n");
    } else {
        printf("LwESP initialized!\r\n");
    }

    lwesp_mode_t mode = LWESP_MODE_STA_AP;
    // -- flash uploader conn start
    // lwesp_conn_upload_start(NULL, NULL, conn_upload_callback_func, 0);

    for (;;) {
        update_eth_changes();
        sync_with_marlin_server();
        // lwesp_get_wifi_mode(&mode, NULL, NULL, 0);
        // if (mode == LWESP_MODE_STA) {
        // printf("test ok");
        // }
        lwesp_conn_upload_start(NULL, NULL, conn_upload_callback_func, 0);
        osDelay(2000);
    }
}

static lwespr_t conn_upload_callback_func(lwesp_evt_t *evt) {
    lwesp_conn_p conn;
    lwespr_t res = lwespOK;
    uint8_t conn_num;

    conn = lwesp_conn_get_from_evt(evt);
    if (conn == NULL) {
        return lwespERR;
    }
    conn_num = lwesp_conn_getnum(conn); /* Get connection number for identification */
    switch (lwesp_evt_get_type(evt)) {
    case LWESP_EVT_CONN_SEND: { /* Data send event */
        lwespr_t res = lwesp_evt_conn_send_get_result(evt);
        if (res == lwespOK) {
            _dbg0("Data sent successfully");
        } else {
            _dbg0("Data sent ERROR");
        }
        break;
    }
    case LWESP_EVT_CONN_RECV: { /* Data received from remote side */
        lwesp_pbuf_p pbuf = lwesp_evt_conn_recv_get_buff(evt);
        lwesp_conn_recved(conn, pbuf); /* Notify stack about received pbuf */
        _dbg0("Received %d bytes..", (int)lwesp_pbuf_length(pbuf, 1));
        break;
    }
    default:
        break;
    }
    return res;
}

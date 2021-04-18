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

#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "stm32_port.h"
#include "esp_loader.h"
#include "example_common.h"
#define HIGHER_BAUDRATE 115520

#define WUI_NETIF_SETUP_DELAY  1000
#define WUI_COMMAND_QUEUE_SIZE WUI_WUI_MQ_CNT // maximal number of messages at once in WUI command messageQ

// WUI thread mutex for updating marlin vars
osMutexDef(wui_thread_mutex);
osMutexId(wui_thread_mutex_id);

static marlin_vars_t *wui_marlin_vars;
wui_vars_t wui_vars;                              // global vriable for data relevant to WUI
static char wui_media_LFN[FILE_NAME_MAX_LEN + 1]; // static buffer for gcode file name

// -- test serial flashing ESP
static uint8_t serial_flashing = 1;
extern UART_HandleTypeDef huart6;

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

    lwesp_mode_t mode = LWESP_MODE_STA_AP;
    if (!serial_flashing) {
        // lwesp stuffs
        if (lwesp_init(NULL, 1) != lwespOK) {
            printf("Cannot initialize LwESP!\r\n");
        } else {
            printf("LwESP initialized!\r\n");
        }

    } else {
        loader_stm32_config_t config = {
            .huart = &huart6,
            .port_io0 = GPIOE,
            .pin_num_io0 = GPIO_PIN_6,
            .port_rst = GPIOC,
            .pin_num_rst = GPIO_PIN_13,
        };
        loader_port_stm32_init(&config);
        if (connect_to_target(HIGHER_BAUDRATE) == ESP_LOADER_SUCCESS) {
            _dbg0("SYNC DONe - connected with ESP");
        } else {
          _dbg0("SYNC FAiLED");
        }
    }

    for (;;) {
        update_eth_changes();
        sync_with_marlin_server();
        if (!serial_flashing) {
            lwesp_get_wifi_mode(&mode, NULL, NULL, 0);
            if (mode == LWESP_MODE_STA) {
                printf("test ok");
            }
        }
        osDelay(1000);
    }
}

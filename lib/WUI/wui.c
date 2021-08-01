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
#include <string.h>
#include "sntp_client.h"
#include "dbg.h"
#include "esp/esp.h"
#include "stm32_port.h"
#include "lwip/altcp_tcp.h"
#include "esp_tcp.h"
#include "netifapi.h"
#include "dns.h"
#include "httpd.h"

typedef enum {
    WUI_IP4_DHCP,
    WUI_IP4_STATIC
} WUI_IP4_TYPE;

/**
 * \brief           Lookup table for preferred SSIDs with password for auto connect feature
 */
typedef struct {
    const char *ssid;
    const char *pass;
} ap_entry_t;

#define DNS_1 0
#define DNS_2 1

#define create_evt_eth(dev, flag, value) (((dev) << 28 | (flag) << 16) | (value))

#define EVT_TCPIP_INIT_FINISHED              (0xFFFFFFFFUL)
#define EVT_NETDEV_INIT_FINISHED(dev, value) create_evt_eth((dev), 0x0FFF, (value))

#define LOOP_EVT_TIMEOUT           500UL
#define IS_TIME_TO_CHECK_ESP(time) (((time) % 1000) == 0)

osMessageQDef(networkMbox, 16, NULL);
static osMessageQId networkMbox_id;

// WUI thread mutex for updating marlin vars
osMutexDef(wui_thread_mutex);
osMutexId(wui_thread_mutex_id);

static marlin_vars_t *wui_marlin_vars;
wui_vars_t wui_vars;                              // global vriable for data relevant to WUI
static char wui_media_LFN[FILE_NAME_MAX_LEN + 1]; // static buffer for gcode file name

struct netif eth0; // network interface structure for ETH

static ETH_config_t wui_eth_config;  // the active WUI configuration for ethernet, connect and server
static ETH_config_t wui_wifi_config; // the active WUI configuration for ethernet, connect and server

static ap_entry_t ap = { "ssid", "password" };
static ETH_STATUS_t esp_state = ETH_UNLINKED;

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

static void netif_link_callback(struct netif *eth) {
    ethernetif_update_config(eth);
    eth->hostname = wui_eth_config.hostname;
    osMessagePut(networkMbox_id, EVT_NETDEV_INIT_FINISHED(NETDEV_ETH_ID, 0), 0);
}

static void netif_status_callback(struct netif *eth) {
    ethernetif_update_config(eth);
}

static void tcpip_init_done_callback(void *arg) {
    if (netif_add_noaddr(&eth0, NULL, ethernetif_init, tcpip_input)) {
        netif_set_link_callback(&eth0, netif_link_callback);
        netif_set_status_callback(&eth0, netif_status_callback);
    }
    osMessagePut(networkMbox_id, EVT_TCPIP_INIT_FINISHED, 0);
}

ETH_STATUS_t get_eth_status(void) {
    if (!ethernetif_link(&eth0)) {
        return ETH_UNLINKED;
    }
    return !netif_is_link_up(&eth0) ? ETH_NETIF_DOWN : ETH_NETIF_UP;
}

uint32_t netdev_set_dhcp(uint32_t netdev_id) {
    if (netdev_id == NETDEV_ETH_ID) {
        err_t res;
        res = netifapi_dhcp_start(&eth0);
        CHANGE_FLAG_TO_DHCP(wui_eth_config.lan.flag);
        wui_eth_config.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
        save_eth_params(&wui_eth_config);
        wui_eth_config.var_mask = 0;
        return res;
    } else {
        return ERR_OK;
    }
}

uint32_t netdev_set_up(uint32_t netdev_id) {
    if (netdev_id == NETDEV_ETH_ID) {
        err_t res;
        netifapi_netif_set_link_up(&eth0);
        res = netifapi_netif_set_up(&eth0);
        TURN_FLAG_ON(wui_eth_config.lan.flag);
        wui_eth_config.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
        save_eth_params(&wui_eth_config);
        wui_eth_config.var_mask = 0;
        return res;
    } else {
        return ERR_OK;
    }
}

uint32_t netdev_set_down(uint32_t netdev_id) {
    if (netdev_id == NETDEV_ETH_ID) {
        err_t res = netifapi_netif_set_link_down(&eth0);
        res = netifapi_netif_set_down(&eth0);
        TURN_FLAG_OFF(wui_eth_config.lan.flag);
        wui_eth_config.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
        save_eth_params(&wui_eth_config);
        wui_eth_config.var_mask = 0;
        return res;
    } else {
        return ERR_OK;
    }
}

uint32_t netdev_set_static(uint32_t netdev_id) {
    if (netdev_id == NETDEV_ETH_ID) {
        err_t res;
        dns_setserver(DNS_1, &wui_eth_config.dns1_ip4);
        dns_setserver(DNS_2, &wui_eth_config.dns2_ip4);
        res = netifapi_netif_set_addr(&eth0, &wui_eth_config.lan.addr_ip4, &wui_eth_config.lan.msk_ip4, &wui_eth_config.lan.gw_ip4);
        res = netifapi_dhcp_inform(&eth0);
        CHANGE_FLAG_TO_STATIC(wui_eth_config.lan.flag);
        wui_eth_config.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
        save_eth_params(&wui_eth_config);
        wui_eth_config.var_mask = 0;
        return res;
    } else {
        return ERR_OK;
    }
}

ETH_STATUS_t get_wifi_status(void) {
    return esp_state;
}

uint8_t get_lan_flag(void) {
    return wui_eth_config.lan.flag;
}

void get_eth_address(ETH_config_t *config) {
    config->lan.addr_ip4.addr = netif_ip4_addr(&eth0)->addr;
    config->lan.msk_ip4.addr = netif_ip4_netmask(&eth0)->addr;
    config->lan.gw_ip4.addr = netif_ip4_gw(&eth0)->addr;
}

/**
 * \brief           Event callback function for ESP stack
 * \param[in]       evt: Event information with data
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
static espr_t
esp_callback_func(esp_evt_t *evt) {
    switch (esp_evt_get_type(evt)) {
    case ESP_EVT_AT_VERSION_NOT_SUPPORTED: {
        esp_sw_version_t v_min, v_curr;

        esp_get_min_at_fw_version(&v_min);
        esp_get_current_at_fw_version(&v_curr);

        _dbg("Current ESP8266 AT version is not supported by library!");
        _dbg("Minimum required AT version is: %d.%d.%d", (int)v_min.major, (int)v_min.minor, (int)v_min.patch);
        _dbg("Current AT version is: %d.%d.%d", (int)v_curr.major, (int)v_curr.minor, (int)v_curr.patch);
        break;
    }
    case ESP_EVT_INIT_FINISH: {
        esp_set_wifi_mode(ESP_MODE_STA, NULL, NULL, 1);
        esp_sta_join(ap.ssid, ap.pass, NULL, 0, NULL, NULL, 0);
        _dbg("ESP_EVT_INIT_FINISH");
        break;
    }
    case ESP_EVT_RESET: {
        _dbg("ESP_EVT_RESET");
        break;
    }
    case ESP_EVT_RESET_DETECTED: {
        _dbg("ESP_EVT_RESET_DETECTED");
        break;
    }
    case ESP_EVT_WIFI_GOT_IP: {
        _dbg("ESP_EVT_WIFI_GOT_IP");
        esp_state = ETH_NETIF_UP;
        break;
    }
    case ESP_EVT_WIFI_CONNECTED: {
        uint32_t message = EVT_NETDEV_INIT_FINISHED(NETDEV_ESP_ID, 0);
        _dbg("ESP_EVT_WIFI_CONNECTED");
        osMessagePut(networkMbox_id, message, 0);
        break;
    }
    case ESP_EVT_WIFI_DISCONNECTED: {
        _dbg("ESP_EVT_WIFI_DISCONNECTED");
        esp_state = ETH_NETIF_DOWN;
        break;
    }
    default:
        break;
    }
    return espOK;
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

    wui_eth_config.var_mask = ETHVAR_EEPROM_CONFIG;
    load_eth_params(&wui_eth_config);

    wui_marlin_client_init();
    tcpip_init(tcpip_init_done_callback, NULL);
    esp_init(esp_callback_func, 0);

    for (;;) {
        osEvent evt = osMessageGet(networkMbox_id, LOOP_EVT_TIMEOUT);
        if (evt.status == osEventMessage) {
            switch (evt.value.v) {
            case EVT_TCPIP_INIT_FINISHED:
                netifapi_netif_set_default(&eth0);
                if (IS_LAN_ON(wui_eth_config.lan.flag)) {
                    netdev_set_up(NETDEV_ETH_ID);
                }
                break;
            case EVT_NETDEV_INIT_FINISHED(NETDEV_ETH_ID, 0):
                if (!IS_LAN_STATIC(wui_eth_config.lan.flag)) {
                    netdev_set_dhcp(NETDEV_ETH_ID);
                } else {
                    netifapi_netif_set_up(&eth0);
                    netdev_set_static(NETDEV_ETH_ID);
                }
                httpd_init();
                sntp_client_init();
                break;
            case EVT_NETDEV_INIT_FINISHED(NETDEV_ETH_ID, 1):
                netifapi_netif_set_link_down(&eth0);
                netifapi_netif_set_down(&eth0);
                break;
            case EVT_NETDEV_INIT_FINISHED(NETDEV_ESP_ID, 0):
                if (!IS_LAN_STATIC(wui_eth_config.lan.flag)) {
                    netdev_set_dhcp(NETDEV_ETH_ID);
                } else {
                    netdev_set_static(NETDEV_ETH_ID);
                }
                httpd_init();
                break;
            default:
                break;
            }
        }

        if (esp_state == ETH_UNLINKED && IS_TIME_TO_CHECK_ESP(esp_check_counter * LOOP_EVT_TIMEOUT)) {
            esp_sw_version_t version;
            esp_check_counter = 0;
            if (esp_get_current_at_fw_version(&version)) {
                esp_state == ETH_NETIF_DOWN;
            }
        } else {
            ++esp_check_counter;
        }

        sync_with_marlin_server();
    }
}

struct altcp_pcb *prusa_alloc(void *arg, uint8_t ip_type) {
    if (get_eth_status() == ETH_NETIF_UP)
        return altcp_tcp_new_ip_type(ip_type);
    else if (get_wifi_status() == ETH_NETIF_UP) {
        return altcp_esp_new_ip_type(ip_type);
    } else {
        return NULL;
    }
}

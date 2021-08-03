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
#include "esp.h"
#include "netifapi.h"
#include "dns.h"
#include "httpd.h"

typedef enum {
    WUI_IP4_DHCP,
    WUI_IP4_STATIC
} WUI_IP4_TYPE;

#define DNS_1                       0
#define DNS_2                       1
#define create_evt_eth(flag, value) (((flag) << 16) | (value))

#define EVT_ETH_INIT_FINISHED (0xFFFFFFFFUL)
#define EVT_ETH_OFF           create_evt_eth(LAN_FLAG_ONOFF_POS, 1)
#define EVT_ETH_ON            create_evt_eth(LAN_FLAG_ONOFF_POS, 0)
#define EVT_ETH_STATIC        create_evt_eth(LAN_FLAG_TYPE_POS, 1)
#define EVT_ETH_DHCP          create_evt_eth(LAN_FLAG_TYPE_POS, 0)

osMessageQDef(networkMbox, 16, NULL);
static osMessageQId networkMbox_id;

// WUI thread mutex for updating marlin vars
osMutexDef(wui_thread_mutex);
osMutexId(wui_thread_mutex_id);

static marlin_vars_t *wui_marlin_vars;
wui_vars_t wui_vars;                              // global vriable for data relevant to WUI
static char wui_media_LFN[FILE_NAME_MAX_LEN + 1]; // static buffer for gcode file name
static char *api_key = "miniPL-apikey";
static uint32_t ip4_type = WUI_IP4_DHCP;
struct netif eth0;           // network interface structure for ETH
ETH_config_t wui_eth_config; // the active WUI configuration for ethernet, connect and server

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
}

static void netif_status_callback(struct netif *eth) {
    ethernetif_update_config(eth);
}

static void tcpip_init_done_callback(void *arg) {
    ETH_config_t *ethconfig = (ETH_config_t *)arg;
    uint32_t message = EVT_ETH_INIT_FINISHED;

    if (IS_LAN_STATIC(ethconfig->lan.flag)) {
        ip4_type = WUI_IP4_STATIC;
        dns_setserver(DNS_1, &ethconfig->dns1_ip4);
        dns_setserver(DNS_2, &ethconfig->dns2_ip4);
    } else {
        ip4_type = WUI_IP4_DHCP;
        ethconfig->lan.addr_ip4.addr = 0;
        ethconfig->lan.msk_ip4.addr = 0;
        ethconfig->lan.gw_ip4.addr = 0;
    }

    /* add the network interface (IPv4/IPv6) with RTOS */
    netif_add(&eth0, (const ip4_addr_t *)&(ethconfig->lan.addr_ip4.addr), (const ip4_addr_t *)&(ethconfig->lan.msk_ip4.addr), (const ip4_addr_t *)&(ethconfig->lan.gw_ip4.addr), NULL, &ethernetif_init, &tcpip_input);

    // set the host name
    eth0.hostname = ethconfig->hostname;
    /* Setting necessary callbacks after initial setup */
    netif_set_link_callback(&eth0, netif_link_callback);
    netif_set_status_callback(&eth0, netif_status_callback);
    osMessagePut(networkMbox_id, message, 0);
}

const ETH_STATUS_t get_eth_status(void) {
    if (!ethernetif_link(&eth0)) {
        return ETH_UNLINKED;
    }
    return !netif_is_link_up(&eth0) ? ETH_NETIF_DOWN : ETH_NETIF_UP;
}

uint8_t get_lan_flag(void) {
    return wui_eth_config.lan.flag;
}

void get_eth_address(ETH_config_t *config) {
    config->lan.addr_ip4.addr = netif_ip4_addr(&eth0)->addr;
    config->lan.msk_ip4.addr = netif_ip4_netmask(&eth0)->addr;
    config->lan.gw_ip4.addr = netif_ip4_gw(&eth0)->addr;
}

void eth_change_setting(uint16_t flag, uint16_t value) {
    uint32_t message = create_evt_eth(flag, value);
    osMessagePut(networkMbox_id, message, 0);
}

extern void netconn_client_thread(void const *arg);

void StartWebServerTask(void const *argument) {
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

    // get settings from ini file
    if (IS_LAN_ON(wui_eth_config.lan.flag) && load_ini_file(&wui_eth_config)) {
        save_eth_params(&wui_eth_config);
    }
    // marlin client initialization for WUI
    wui_marlin_client_init();

    // TcpIp related initalizations
    tcpip_init(tcpip_init_done_callback, &wui_eth_config);

    // LwESP stuffs
    /*
    // TODO: Handle enable disable of ESP
    _dbg("LwESP initialized with result = %ld", esp_initialize());
    ap_entry_t ap = { "esptest", "lwesp8266" };
    if (!esp_connect_to_AP(&ap)) {
        _dbg("LwESP connect to AP %s!", ap.ssid);
        httpd_init();
    }
    */

    for (;;) {
        osEvent evt = osMessageGet(networkMbox_id, 500);
        if (evt.status == osEventMessage) {
            load_eth_params(&wui_eth_config);
            switch (evt.value.v) {
            case EVT_ETH_INIT_FINISHED:
                /* Registers the default network interface */
                netifapi_netif_set_default(&eth0);

                if (IS_LAN_ON(wui_eth_config.lan.flag)) {
                    /* When the netif is fully configured this function must be called */
                    netifapi_netif_set_link_up(&eth0);
                    netifapi_netif_set_up(&eth0);
                    // start the DHCP if needed!
                    if (!IS_LAN_STATIC(wui_eth_config.lan.flag)) {
                        netifapi_dhcp_start(&eth0);
                    } else {
                        dns_setserver(DNS_1, &wui_eth_config.dns1_ip4);
                        dns_setserver(DNS_2, &wui_eth_config.dns2_ip4);
                        netifapi_netif_set_addr(&eth0, &wui_eth_config.lan.addr_ip4, &wui_eth_config.lan.msk_ip4, &wui_eth_config.lan.gw_ip4);
                        netifapi_dhcp_inform(&eth0);
                    }
                    httpd_init();
                    sntp_client_init();
                } else {
                    /* When the netif link is down this function must be called */
                    netifapi_netif_set_link_down(&eth0);
                    netifapi_netif_set_down(&eth0);
                }
                break;
            case EVT_ETH_DHCP:
                netifapi_dhcp_start(&eth0);
                break;
            case EVT_ETH_STATIC:
                dns_setserver(DNS_1, &wui_eth_config.dns1_ip4);
                dns_setserver(DNS_2, &wui_eth_config.dns2_ip4);
                netifapi_netif_set_addr(&eth0, &wui_eth_config.lan.addr_ip4, &wui_eth_config.lan.msk_ip4, &wui_eth_config.lan.gw_ip4);
                netifapi_dhcp_inform(&eth0);
                break;
            case EVT_ETH_ON:
                netifapi_netif_set_link_up(&eth0);
                netifapi_netif_set_up(&eth0);
                // start the DHCP if needed!
                if (!IS_LAN_STATIC(wui_eth_config.lan.flag)) {
                    netifapi_dhcp_start(&eth0);
                }
                TURN_FLAG_ON(wui_eth_config.lan.flag);
                break;
            case EVT_ETH_OFF:
                /* When the netif link is down this function must be called */
                netifapi_netif_set_link_down(&eth0);
                netifapi_netif_set_down(&eth0);
                TURN_FLAG_OFF(wui_eth_config.lan.flag);
                break;
            default:
                break;
            }
            save_eth_params(&wui_eth_config);
        }

        sync_with_marlin_server();
    }
}

const char *wui_get_api_key() {
    return api_key;
}

struct altcp_pcb *prusa_alloc(void *arg, uint8_t ip_type) {
    if (get_eth_status() == ETH_NETIF_UP)
        return altcp_tcp_new_ip_type(ip_type);
    else
        return altcp_esp_new_ip_type(ip_type);
}

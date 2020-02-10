/*
 * wui.h
 * \brief main interface functions for Web User Interface (WUI) thread
 *
 *  Created on: Dec 12, 2019
 *      Author: joshy
 */

#include "wui.h"
#include "marlin_client.h"
#include "lwip.h"
#include "ethernetif.h"
#include "http_client_prusa.h"

#include "cmsis_os.h"
#include "http/http_client_prusa.h"

#define MAX_WUI_REQUEST_LEN 100
#define MAX_MARLIN_REQUEST_LEN 100
#define WUI_FLG_PEND_REQ 0x0001

osMessageQId wui_queue = 0; // input queue (uint8_t)
osSemaphoreId wui_sema = 0; // semaphore handle
osMutexDef (wui_web_mutex);    // Declare mutex
osMutexId  (wui_web_mutex_id); // Mutex ID

typedef struct {
    uint32_t flags;
    marlin_vars_t * wui_marlin_vars;
    char request[MAX_REQUEST_LEN];
    uint16_t request_len;
} web_client_t;

marlin_vars_t webserver_marlin_vars;

web_client_t wui;

static void wui_queue_cycle(void);
static int process_wui_request(void);

void StartWebServerTask(void const *argument) {
    osMessageQDef(wuiQueue, 64, uint8_t);
    wui_queue = osMessageCreate(osMessageQ(wuiQueue), NULL);
    osSemaphoreDef(wuiSema);
    wui_sema = osSemaphoreCreate(osSemaphore(wuiSema), 1);
    wui_web_mutex_id = osMutexCreate(osMutex(wui_web_mutex));
    wui.wui_marlin_vars = marlin_client_init(); // init the client
    wui.flags = wui.request_len = 0;

    uint32_t POST_timer = HAL_GetTick();
    bool got_addr = false;

    MX_LWIP_Init();
    http_server_init();
    for (;;) {
//        marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_MSK_TEMP_ALL));
        ethernetif_link(&eth0);
        wui_queue_cycle();

        if(wui.wui_marlin_vars) {
            marlin_client_loop();
        }

        if(netif_ip4_addr(&eth0)->addr != 0 && !got_addr){  //REWORK
            got_addr = true;
        }
        if(got_addr && HAL_GetTick() - POST_timer >= 1000 && wui.wui_marlin_vars){
            tcp_connect_to_server("GET /api/printer HTTP/1.1\r\nHost: 192.168.1.182\r\n\r\n");
            POST_timer = HAL_GetTick();
        }

        osMutexWait(wui_web_mutex_id, osWaitForever);
        webserver_marlin_vars = *(wui.wui_marlin_vars);
        osMutexRelease(wui_web_mutex_id);
        osDelay(100);
        // http client loop
        buddy_http_client_loop();
    }
}

static void wui_queue_cycle(){
    osEvent ose;
    char ch;

    if(wui.flags & WUI_FLG_PEND_REQ){
        if(process_wui_request()){
            wui.flags &= ~WUI_FLG_PEND_REQ;
            wui.request_len = 0;
        }
    }

    while ((ose = osMessageGet(wui_queue, 0)).status == osEventMessage) {
        ch = (char)((uint8_t)(ose.value.v));
        switch (ch) {
        case '\r':
        case '\n':
            ch = 0;
            break;
        }
        if (wui.request_len < MAX_WUI_REQUEST_LEN)
            wui.request[wui.request_len++] = ch;
        else {
            //TOO LONG
            wui.request_len = 0;
        }
        if ((ch == 0) && (wui.request_len > 1)) {
            if (process_wui_request()) {
                wui.request_len = 0;
            } else {
                wui.flags |= WUI_FLG_PEND_REQ;
                break;
            }
        }
    }
}
static int process_wui_request(){

    //if(wui.request == gcode)
    marlin_json_gcode(wui.request);
    return 1;
}

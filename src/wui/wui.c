/*
 * wui.h
 * \brief main interface functions for Web User Interface (WUI) thread
 *
 *  Created on: Dec 12, 2019
 *      Author: joshy
 */

#include "wui.h"
#include "stdbool.h"
#include "marlin_client.h"
#include "lwip.h"
#include "ethernetif.h"

#include "cmsis_os.h"

osMutexDef(wui_web_mutex);   // Declare mutex
osMutexId(wui_web_mutex_id); // Mutex ID

marlin_vars_t *wui_marlin_vars = 0;
marlin_vars_t webserver_marlin_vars;

void StartWebServerTask(void const *argument) {
    wui_web_mutex_id = osMutexCreate(osMutex(wui_web_mutex));
    wui_marlin_vars = marlin_client_init(); // init the client
    MX_LWIP_Init();
    http_server_init();
    for (;;) {
        ethernetif_link(&eth0);
        // char* ip_str = ip4addr_ntoa(&(eth0.ip_addr)); // for tests when GUI is absent
        if (wui_marlin_vars) {
            marlin_client_loop();
        }
        osMutexWait(wui_web_mutex_id, osWaitForever);
        webserver_marlin_vars = *wui_marlin_vars;
        osMutexRelease(wui_web_mutex_id);
        osDelay(100);
    }
}

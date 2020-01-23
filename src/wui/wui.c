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

#include "cmsis_os.h"

marlin_vars_t* webserver_marlin_vars = 0;

void init_wui() {
    webserver_marlin_vars = marlin_client_init(); // init the client
}

void StartWebServerTask(void const *argument) {
    void init_wui();
	MX_LWIP_Init();
    http_server_init();
    for (;;) {
        osDelay(100);
        marlin_var_update();
    }
}

void marlin_var_update() {
    if(webserver_marlin_vars) {
        marlin_client_loop();
    }
}

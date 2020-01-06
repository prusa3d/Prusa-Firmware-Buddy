#include "wui.h"
#include "stdbool.h"
#include "marlin_client.h"

marlin_vars_t* webserver_marlin_vars = 0;

void init_wui() {
    webserver_marlin_vars = marlin_client_init(); // init the client
}

void marlin_var_update() {
    if(webserver_marlin_vars) {
        marlin_client_loop();
    }
}

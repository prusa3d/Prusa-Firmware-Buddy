#include "esp/esp_private.h"
#include "esp/esp_debug.h"
#include "esp/esp.h"

#if ESP_CFG_DBG || __DOXYGEN__

const char*
espi_dbg_msg_to_string(esp_cmd_t cmd) {
    static char tmp_arr[100];
    if (cmd) {
        sprintf(tmp_arr, "%d", (int)cmd);
        return tmp_arr;
    }
    return "";
}

#endif /* ESP_CFG_DBG || __DOXYGEN__ */

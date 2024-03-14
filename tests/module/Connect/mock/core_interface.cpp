#include "core_interface.hpp"
#include "os_porting.hpp"
#include <variant>
#include <cstring>
#include <cstdbool>
#include <ctype.h>
#include <cstdio>
#include <assert.h>

void core_interface::get_data(device_params_t *params) {
    if (NULL == params) {
        assert(0);
        return;
    }

    memset(params, 0, sizeof(device_params_t));

    params->temp_bed = 25.00;
    params->target_bed = 25.00;
    params->temp_nozzle = 25.00;
    params->target_nozzle = 25.00;
    params->pos[X_AXIS_POS] = 0;
    params->pos[Y_AXIS_POS] = 0;
    params->pos[Z_AXIS_POS] = 0;
    params->print_speed = 100;
    params->flow_factor = 100;
    params->state = DEVICE_STATE_READY;
}

core_interface::core_interface() {
}

std::optional<Error> core_interface::get_printer_info(printer_info_t *printer_info) {
    if (NULL == printer_info) {
        return Error::ERROR;
    }

    snprintf(printer_info->firmware_version, FW_VER_BUFR_LEN, "%s", "4.1.0-CONN+0000");

    printer_info->printer_type = 2;
    constexpr char serial_number[] = "CZPX2345X234XC12345";
    strlcpy(printer_info->serial_number, serial_number, SER_NUM_BUFR_LEN);

    memcpy(printer_info->fingerprint, "FB34DF45FB34DF45", FINGERPRINT_SIZE);
    printer_info->fingerprint[FINGERPRINT_SIZE] = 0;

    printer_info->appendix = false;
    return std::nullopt;
}

std::optional<Error> core_interface::get_connect_config(configuration_t *config) {
    snprintf(config->url, CONNECT_URL_BUF_LEN, "%s", "dev.connect.prusa");
    snprintf(config->token, CONNECT_TOKEN_BUF_LEN, "%s", "0fb07fc9bc205e2f2fc3");
    config->port = 8000;
}

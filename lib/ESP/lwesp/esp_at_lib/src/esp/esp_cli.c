/**
 * \file            esp_cli.c
 * \brief           Esp CLI commands
 */

/*
 * Copyright (c) 2019 Miha CESNIK
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Author:          Miha CESNIK <>
 * Version:         $_version_$
 */

#include <stdbool.h>
#include <stdint.h>
#include "esp/esp.h"
#if ESP_CFG_MODE_STATION
#include "esp/esp_sta.h"
#endif /* ESP_CFG_MODE_STATION */
#include "cli/cli.h"
#include "cli/cli_config.h"

#if ESP_CFG_MODE_STATION
static void cli_station_info(cli_printf cliprintf, int argc, char** argv);
#endif /* ESP_CFG_MODE_STATION */

static const cli_command_t
commands[] = {
#if ESP_CFG_MODE_STATION
    { "station-info",       "Get current station info",                 cli_station_info },
#endif /* ESP_CFG_MODE_STATION */

};

/**
 * \brief           CLI Init function for adding basic CLI commands
 */
void
esp_cli_register_commands(void) {
    cli_register_commands(commands, sizeof(commands)/sizeof(commands[0]));
}

#if ESP_CFG_MODE_STATION || __DOXYGEN__

/**
 * \brief           CLI command for reading current AP info
 * \param[in]       cliprintf: Pointer to CLI printf function
 * \param[in]       argc: Number fo arguments in argv
 * \param[in]       argv: Pointer to the commands arguments
 */
static void
cli_station_info(cli_printf cliprintf, int argc, char** argv) {
    espr_t res;
    esp_sta_info_ap_t info;

    res = esp_sta_get_ap_info(&info, NULL, NULL, 1);
    if (res != espOK) {
        cliprintf("Error: Failed to read station info (%d)"CLI_NL, res);
        return;
    }

    cliprintf("  SSID:    %s"CLI_NL, info.ssid);
    cliprintf("  MAC:     %02X:%02X:%02X:%02X:%02X"CLI_NL, info.mac.mac[0], info.mac.mac[1], info.mac.mac[2], info.mac.mac[3], info.mac.mac[4], info.mac.mac[5]);
    cliprintf("  RSSI:    %d"CLI_NL, info.rssi);
    cliprintf("  Channel: %d"CLI_NL, info.ch);

    ESP_UNUSED(argc);
    ESP_UNUSED(argv);
}

#endif /* ESP_CFG_MODE_STATION || __DOXYGEN__ */

/**
 * \file            esp_cli.c
 * \brief           Esp CLI commands
 */

/*
 * Copyright (c) 2018 Miha Cesnik
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
 * Author:          Miha ČESNIK
 */

#include <stdbool.h>
#include <stdint.h>
#include "esp/esp.h"
#if ESP_CFG_MODE_STATION
#include "esp/esp_sta.h"
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_HOSTNAME
#include "esp/esp_hostname.h"
#endif /* ESP_CFG_HOSTNAME */
#include "cli/cli.h"
#include "cli/cli_config.h"

#if ESP_CFG_MODE_STATION
static void cli_station_info(cli_printf cliprintf, int argc, char** argv);
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_HOSTNAME
static void cli_hostname_get(cli_printf cliprintf, int argc, char** argv);
static void cli_hostname_set(cli_printf cliprintf, int argc, char** argv);
#endif /* ESP_CFG_HOSTNAME */

static const cli_command_t
commands[] = {
#if ESP_CFG_MODE_STATION
    { "station-info",       "Get current station info",                 cli_station_info },
#endif /* ESP_CFG_MODE_STATION */
#if ESP_CFG_HOSTNAME
    { "hostname-get",       "Read the ESPs hostname",                   cli_hostname_get },
    { "hostname-set",       "Set the ESPs hostname",                    cli_hostname_set },
#endif /* ESP_CFG_HOSTNAME */

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
}

#endif /* ESP_CFG_MODE_STATION || __DOXYGEN__ */

#if ESP_CFG_HOSTNAME || __DOXYGEN__

/**
 * \brief           CLI command for reading ESPs hostname
 * \param[in]       cliprintf: Pointer to CLI printf function
 * \param[in]       argc: Number fo arguments in argv
 * \param[in]       argv: Pointer to the commands arguments
 */
static void
cli_hostname_get(cli_printf cliprintf, int argc, char** argv) {
    espr_t res;
    char hostname[32];

    res = esp_hostname_get(hostname, sizeof(hostname), NULL, NULL, 1);
    if (res != espOK) {
        cliprintf("Error: Failed to get the hostname (%d)"CLI_NL, res);
        return;
    }

    cliprintf("  %s"CLI_NL, hostname);
}

/**
 * \brief           CLI command for setting the ESP hostname
 * \param[in]       cliprintf: Pointer to CLI printf function
 * \param[in]       argc: Number fo arguments in argv
 * \param[in]       argv: Pointer to the commands arguments
 */
static void
cli_hostname_set(cli_printf cliprintf, int argc, char** argv) {
    espr_t res;
    char * hostname;

    if (argc < 2) {
        cliprintf("Error: too few parameters. Provied desired ESP hostname"CLI_NL);
        return;
    }

    hostname = argv[1];

    res = esp_hostname_set(hostname, NULL, NULL, 1);
    if (res != espOK) {
        cliprintf("Error: Failed to set the hostname (%d)"CLI_NL, res);
        return;
    }

    cliprintf("  Set to: %s"CLI_NL, hostname);
}

#endif /* ESP_CFG_HOSTNAME || __DOXYGEN__ */

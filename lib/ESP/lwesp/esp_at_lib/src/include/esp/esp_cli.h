/**
 * \file            esp_cli.h
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
#ifndef ESP_HDR_CLI_H
#define ESP_HDR_CLI_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \ingroup         ESP
 * \defgroup        ESP_CLI Command line interface
 * \brief           Command line interface
 * \{
 */

#include "esp/esp.h"

void esp_cli_register_commands(void);

/**
 * \}
 */

#ifdef __cplusplus
}
#endif

#endif /* ESP_HDR_CLI_H */


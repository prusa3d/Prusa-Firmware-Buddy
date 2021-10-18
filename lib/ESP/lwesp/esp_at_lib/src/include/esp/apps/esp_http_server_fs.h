/**
 * \file            esp_http_server_fs.h
 * \brief           Function declaration for file system
 */

/*
 * Copyright (c) 2019 Tilen MAJERLE
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
 * This file is part of ESP-AT library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         $_version_$
 */
#ifndef ESP_HDR_HTTP_SERVER_FS_H
#define ESP_HDR_HTTP_SERVER_FS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "esp/apps/esp_http_server.h"

/**
 * \ingroup         ESP_APP_HTTP_SERVER
 * \defgroup        ESP_APP_HTTP_SERVER_FS_FAT FAT File System
 * \brief           FATFS file system implementation for dynamic files
 * \{
 */

uint8_t     http_fs_open(http_fs_file_t* file, const char* path);
uint32_t    http_fs_read(http_fs_file_t* file, void* buff, size_t btr);
uint8_t     http_fs_close(http_fs_file_t* file);

/**
 * \}
 */

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* ESP_HDR_HTTP_SERVER_H */

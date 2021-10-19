/**
 * \file            esp_mem_lwmem.c
 * \brief           Dynamic memory manager implemented with LwMEM
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
#include "esp/esp.h"

/* See esp_mem.c file for function documentation on parameters and return values */

#if ESP_CFG_MEM_CUSTOM && !__DOXYGEN__

/*
 * Before this driver can be used, user must:
 *
 *  - Configure LwMEM for thread-safety mode by enabling operating system features
 *  - Set memory regions during start of application
 */

#include "lwmem/lwmem.h"

void *
esp_mem_malloc(size_t size) {
    return lwmem_malloc(size);
}

void *
esp_mem_realloc(void* ptr, size_t size) {
    return lwmem_realloc(ptr, size);
}

void *
esp_mem_calloc(size_t num, size_t size) {
    return lwmem_calloc(num, size);
}

void
esp_mem_free(void* ptr) {
    lwmem_free(ptr);
}

#endif /* ESP_CFG_MEM_CUSTOM && !__DOXYGEN__ */
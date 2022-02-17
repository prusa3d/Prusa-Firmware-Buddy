/* Copyright 2020 Espressif Systems (Shanghai) PTE LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/param.h>
#include <stdio.h>
#include "stm32_port.h"
#include <FreeRTOS.h>
#include <task.h>
#include "espif.h"

#define _dbg(...)
// #define SERIAL_DEBUG_ENABLE

static UART_HandleTypeDef *uart;
static GPIO_TypeDef *gpio_port_io0, *gpio_port_rst;
static uint16_t gpio_num_io0, gpio_num_rst;

#ifdef SERIAL_DEBUG_ENABLE

static void dec_to_hex_str(const uint8_t dec, uint8_t hex_str[3]) {
    static const uint8_t dec_to_hex[] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    };

    hex_str[0] = dec_to_hex[(dec >> 4)];
    hex_str[1] = dec_to_hex[(dec & 0xF)];
    hex_str[2] = '\0';
}

static void serial_debug_print(const uint8_t *data, uint16_t size, bool write) {
    static bool write_prev = false;
    uint8_t hex_str[3];

    if (write_prev != write) {
        write_prev = write;
        printf("\n--- %s ---\n", write ? "WRITE" : "READ");
    }

    for (uint32_t i = 0; i < size; i++) {
        dec_to_hex_str(data[i], hex_str);
        printf("%s ", hex_str);
    }
}

#else

static void serial_debug_print(const uint8_t *data, uint16_t size, bool write) {}

#endif

static uint32_t s_time_end;

esp_loader_error_t loader_port_serial_write(const uint8_t *data, uint16_t size, uint32_t timeout) {
    serial_debug_print(data, size, true);

    HAL_StatusTypeDef err = HAL_UART_Transmit(uart, (uint8_t *)data, size, timeout);

    if (err == HAL_OK) {
        return ESP_LOADER_SUCCESS;
    } else if (err == HAL_TIMEOUT) {
        return ESP_LOADER_ERROR_TIMEOUT;
    } else {
        return ESP_LOADER_ERROR_FAIL;
    }
}

static uint32_t uart_dma_position = 0;

esp_loader_error_t loader_port_serial_read(uint8_t *data, uint16_t size, uint32_t timeout) {
    memset(data, 0x22, size);

    // Wait for enough data in read buffer
    for (;;) {
        const uint32_t pos = sizeof(dma_buffer_rx) - __HAL_DMA_GET_COUNTER(uart->hdmarx);
        if (pos - uart_dma_position >= size) {
            break;
        }

        if (timeout-- == 0) {
            return ESP_LOADER_ERROR_TIMEOUT;
        }

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // Copy data from DMA buffer to output
    for (uint i = 0; i < size; ++i) {
        data[i] = dma_buffer_rx[uart_dma_position];
        // move to new DMA buffer position (wrap at the end of the buffer)
        uart_dma_position = (uart_dma_position + 1) % sizeof(dma_buffer_rx);
    }

    serial_debug_print(data, size, false);

    return ESP_LOADER_SUCCESS;
}

void loader_port_stm32_init(loader_stm32_config_t *config) {
    uart = config->huart;
    gpio_port_io0 = config->port_io0;
    gpio_port_rst = config->port_rst;
    gpio_num_io0 = config->pin_num_io0;
    gpio_num_rst = config->pin_num_rst;
    uart_dma_position = 0;
}

// Set GPIO0 LOW, then
// assert reset pin for 100 milliseconds.
void loader_port_enter_bootloader(void) {
    HAL_GPIO_WritePin(gpio_port_rst, gpio_num_rst, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(gpio_port_io0, gpio_num_io0, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(gpio_port_rst, gpio_num_rst, GPIO_PIN_SET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(gpio_port_io0, gpio_num_io0, GPIO_PIN_SET);
}

void loader_port_reset_target(void) {
    HAL_GPIO_WritePin(gpio_port_rst, gpio_num_rst, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(gpio_port_rst, gpio_num_rst, GPIO_PIN_SET);
}

void loader_port_delay_ms(uint32_t ms) {
    HAL_Delay(ms);
}

void loader_port_start_timer(uint32_t ms) {
    s_time_end = HAL_GetTick() + ms;
}

uint32_t loader_port_remaining_time(void) {
    int32_t remaining = s_time_end - HAL_GetTick();
    return (remaining > 0) ? (uint32_t)remaining : 0;
}

void loader_port_debug_print(const char *str) {
    _dbg("DEBUG: %s", str);
}

esp_loader_error_t loader_port_change_baudrate(uint32_t baudrate) {
    uart->Init.BaudRate = baudrate;

    if (HAL_UART_Init(uart) != HAL_OK) {
        return ESP_LOADER_ERROR_FAIL;
    }

    return ESP_LOADER_SUCCESS;
}

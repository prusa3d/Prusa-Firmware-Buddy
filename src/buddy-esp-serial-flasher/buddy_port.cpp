#include <buddy/esp_uart_dma_buffer_rx.hpp>
#include <cstdlib>
#include <device/peripherals_uart.hpp>
#include <device/peripherals.h>
#include <esp_loader.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

static uint32_t uart_dma_position = 0;
static uint32_t s_time_end;

extern "C" {

esp_loader_error_t loader_port_write(const uint8_t *data, uint16_t size, uint32_t timeout) {

    HAL_StatusTypeDef err = HAL_UART_Transmit(&uart_handle_for_esp, (uint8_t *)data, size, timeout);

    if (err == HAL_OK) {
        return ESP_LOADER_SUCCESS;
    } else if (err == HAL_TIMEOUT) {
        return ESP_LOADER_ERROR_TIMEOUT;
    } else {
        return ESP_LOADER_ERROR_FAIL;
    }
}

esp_loader_error_t loader_port_read(uint8_t *data, uint16_t size, uint32_t timeout) {
    memset(data, 0x22, size);

    // Wait for enough data in read buffer
    for (;;) {
        const uint32_t pos = sizeof(dma_buffer_rx) - __HAL_DMA_GET_COUNTER(uart_handle_for_esp.hdmarx);
        if (pos - uart_dma_position >= size) {
            break;
        }

        if (timeout-- == 0) {
            return ESP_LOADER_ERROR_TIMEOUT;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }

    // Copy data from DMA buffer to output
    for (uint32_t i = 0; i < size; ++i) {
        data[i] = dma_buffer_rx[uart_dma_position];
        // move to new DMA buffer position (wrap at the end of the buffer)
        uart_dma_position = (uart_dma_position + 1) % sizeof(dma_buffer_rx);
    }

    return ESP_LOADER_SUCCESS;
}

esp_loader_error_t loader_port_change_transmission_rate(uint32_t baudrate) {
    uart_handle_for_esp.Init.BaudRate = baudrate;

    if (HAL_UART_Init(&uart_handle_for_esp) != HAL_OK) {
        return ESP_LOADER_ERROR_FAIL;
    }

    if (HAL_UART_Receive_DMA(&uart_handle_for_esp, (uint8_t *)dma_buffer_rx, RX_BUFFER_LEN) != HAL_OK) {
        return ESP_LOADER_ERROR_FAIL;
    }

    // Note: When resetting position in the buffer, make sure we also reset
    //       the buffer in order to not receive already received messages.
    uart_dma_position = 0;
    memset(dma_buffer_rx, 0x22, RX_BUFFER_LEN);

    return ESP_LOADER_SUCCESS;
}

// Set GPIO0 LOW, then
// assert reset pin for 100 milliseconds.
void loader_port_enter_bootloader(void) {
    HAL_GPIO_WritePin(ESP_RST_GPIO_Port, ESP_RST_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(ESP_GPIO0_GPIO_Port, ESP_GPIO0_Pin, GPIO_PIN_RESET);
    vTaskDelay(pdMS_TO_TICKS(100));
    HAL_GPIO_WritePin(ESP_RST_GPIO_Port, ESP_RST_Pin, GPIO_PIN_SET);
    vTaskDelay(pdMS_TO_TICKS(100));
    HAL_GPIO_WritePin(ESP_GPIO0_GPIO_Port, ESP_GPIO0_Pin, GPIO_PIN_SET);

    // Start receiving only after bootloader sends its message.
    // No need to receive those pesky framing errors and recover from them.
    if (loader_port_change_transmission_rate(115200) != ESP_LOADER_SUCCESS) {
        // No way to signalize error to the caller :-(
        abort();
    }
}

void loader_port_reset_target(void) {
    HAL_GPIO_WritePin(ESP_RST_GPIO_Port, ESP_RST_Pin, GPIO_PIN_RESET);
    vTaskDelay(pdMS_TO_TICKS(100));
    HAL_GPIO_WritePin(ESP_RST_GPIO_Port, ESP_RST_Pin, GPIO_PIN_SET);
}

void loader_port_delay_ms(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void loader_port_start_timer(uint32_t ms) {
    s_time_end = HAL_GetTick() + ms;
}

uint32_t loader_port_remaining_time(void) {
    int32_t remaining = s_time_end - HAL_GetTick();
    return (remaining > 0) ? (uint32_t)remaining : 0;
}
}

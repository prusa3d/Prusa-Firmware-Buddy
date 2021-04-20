/*
 * socket_uart.c
 *
 *  Created on: Apr 1, 2021
 *      Author: joshy
 */
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "main.h"
#include "uart_socket.h"

/*
 * UART and other pin configuration for ESP01 module
 *
 * UART:                USART6
 * STM32 TX (ESP RX):   GPIOC, GPIO_PIN_6
 * STM32 RX (ESP TX):   GPIOC, GPIO_PIN_7
 * RESET:               GPIOC, GPIO_PIN_13
 * GPIO0:               GPIOE, GPIO_PIN_6
 * GPIO2:               not connected
 * CH_PD:               connected to board 3.3 V
 *
 * UART_DMA:           DMA2
 * UART_RX_STREAM      STREAM_1
 * UART_TX_STREAM      STREAM_6
 */

//message queue stuffs
osMessageQDef(uartSocketMbox, 16, NULL);
static osMessageQId uartSocketMbox_id;

//UART buffer stuffs
#define RX_BUFFER_LEN 0x1000

static uint8_t is_running;
static size_t old_pos;
static uint8_t uart_initalized = 0;
static int socket_id = -1;

uint8_t data_rx[RX_BUFFER_LEN];
uint8_t dma_buffer_rx[RX_BUFFER_LEN];

void handle_rx_data(UART_HandleTypeDef *huart) {
    if (uartSocketMbox_id != NULL) {
        uint32_t message = 0;
        osMessagePut(uartSocketMbox_id, message, 0);
    }
}

void configure_uart_socket(uint32_t baudrate) {
    __HAL_UART_ENABLE_IT(&huart6, UART_IT_IDLE);
    if (HAL_UART_Receive_DMA(&huart6, (uint8_t *)dma_buffer_rx, RX_BUFFER_LEN) != HAL_OK) {
        Error_Handler();
    }
    is_running = 0;
    old_pos = 0;
    is_running = 1;

    /* Create mbox and start thread */
    if (uartSocketMbox_id == NULL) {
        uartSocketMbox_id = osMessageCreate(osMessageQ(uartSocketMbox), NULL);
        if (uartSocketMbox_id == NULL) {
            printf("error!");
        }
    }
}

int uart_socket(int domain, int type, int protocol) {
    if (!uart_initalized) {
        configure_uart_socket(115200);
        uart_initalized = 1;
    }
    socket_id++;
    return socket_id;
}

int uart_connect(int s, const struct sockaddr *name, socklen_t namelen) {
    return 0;
}

ssize_t uart_send(int s, const void *data, size_t size, int flags) {
    for (size_t i = 0; i < size; ++i) {
        HAL_UART_Transmit(&huart6, (uint8_t *)(data + i), 1, 10);
    }
    return size;
}

ssize_t uart_recv(int s, void *mem, size_t len, int flags) {
    size_t pos;
    ssize_t received_len = 0;
    /* Wait for the event message from DMA or USART */
    osMessageGet(uartSocketMbox_id, 500);

    /* Read data */
    uint16_t dma_bytes_left = (uint16_t)(&huart6)->hdmarx->Instance->NDTR; // no. of bytes left for buffer full
    pos = sizeof(dma_buffer_rx) - dma_bytes_left;
    if (pos != old_pos && is_running) {
        if (pos > old_pos) {
            received_len = pos - old_pos;
            memcpy(mem, &dma_buffer_rx[old_pos], len);
        } else {
            received_len = sizeof(dma_buffer_rx) - old_pos;
            memcpy(mem, &dma_buffer_rx[old_pos], len);
            if (pos > 0) {
                received_len = len + pos;
                memcpy((mem + (sizeof(dma_buffer_rx) - old_pos)), &dma_buffer_rx[0], pos);
            }
        }
        old_pos = pos;
        if (old_pos == sizeof(dma_buffer_rx)) {
            old_pos = 0;
        }
    }
    return received_len;
}

int uart_close(int s) {
    socket_id--;
    return 0;
}

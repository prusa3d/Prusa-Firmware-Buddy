#include "buffered_serial.hpp"
#include "stm32f4xx_hal.h"
#include "config.h"
#include <algorithm>
#include "FreeRTOS.h"
#include "bsod.h"
#include "timing.h"

using namespace buddy::hw;

// FIXME: remove uart2 definition from this file
extern "C" {
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_rx;
}

static uint8_t uart2rx_data[32];
BufferedSerial BufferedSerial::uart2(&huart2, &hdma_usart2_rx, false, uart2rx_data, sizeof(uart2rx_data));

BufferedSerial::BufferedSerial(
    UART_HandleTypeDef *uart, DMA_HandleTypeDef *rxDma, bool halfDuplex,
    uint8_t *rxBufPool, size_t rxBufPoolSize)
    : readTimeoutMs(100)
    , writeTimeoutMs(100)
    , isOpen(false)
    , uart(uart)
    , rxDma(rxDma)
    , isHalfDuplex(halfDuplex)
    , pendingWrite(false) {

    uartrxbuff_init(&rxBuf, uart, rxDma, rxBufPoolSize, rxBufPool);
    eventGroup = xEventGroupCreate();
}

BufferedSerial::~BufferedSerial() {
    Close();
}

void BufferedSerial::Open() {
    if (isOpen)
        return;

    // Start receiving data
    HAL_UART_Receive_DMA(uart, rxBuf.buffer, rxBuf.buffer_size);
    if (isHalfDuplex)
        HAL_HalfDuplex_EnableReceiver(uart);

    // Clear the buffer
    uartrxbuff_reset(&rxBuf);

    isOpen = true;
}

void BufferedSerial::Close() {
    if (!isOpen)
        return;

    // Stop receiving new data
    HAL_UART_AbortReceive(uart);

    // Clear the buffer
    uartrxbuff_reset(&rxBuf);

    isOpen = false;
}

size_t BufferedSerial::Read(char *buf, size_t len) {
    if (!isOpen)
        return 0;

    size_t read = 0;
    auto tickStart = ticks_ms();

    while (read != len) {
        int ch = uartrxbuff_getchar(&rxBuf);

        if (ch >= 0) {
            buf[read++] = (char)ch;
        } else {
            int spentMs = ticks_diff(ticks_ms(), tickStart);
            int budgetMs = readTimeoutMs - spentMs;
            if (budgetMs <= 0) {
                break;
            }

            uartrxbuff_wait_for_event(&rxBuf, std::min(budgetMs, 1));
        }
    }

    return read;
}

size_t BufferedSerial::Write(const char *buf, size_t len) {
    TickType_t timeout = writeTimeoutMs / portTICK_PERIOD_MS;

    // make sure there are no pending event flags
    xEventGroupClearBits(eventGroup, static_cast<EventBits_t>(Event::WriteFinished));

    // initiate the transfer
    pendingWrite = true;
    if (HAL_UART_Transmit_IT(uart, (uint8_t *)buf, len) != HAL_OK) {
        pendingWrite = false;
        if (isHalfDuplex)
            HAL_HalfDuplex_EnableReceiver(uart);
        return 0;
    }

    // wait for the transfer to finish
    auto caughtEvents = xEventGroupWaitBits(eventGroup, static_cast<EventBits_t>(Event::WriteFinished),
        /*xClearOnExit=*/pdTRUE,
        /*xWaitForAllBits=*/pdTRUE,
        /*xTicksToWait=*/timeout);
    pendingWrite = false;

    if (caughtEvents & static_cast<EventBits_t>(Event::WriteFinished)) {
        return len;
    } else {
        // if we did not see the WriteFinished event, make sure
        //   to abort the transfer before returning error
        HAL_UART_AbortTransmit_IT(uart);
        return 0;
    }
}

void BufferedSerial::WriteFinishedISR() {
    // NOTE: This flag might not be necessary. I have seen some occasions when the TC ISR
    //  triggered before FreeRTOS was up (which would be a problem). Something we might
    //  investigate, but should not be an issue.
    if (!pendingWrite)
        return;

    // As soon as the transfer is complete, switch back to receiver mode (half duplex)
    if (isHalfDuplex)
        HAL_HalfDuplex_EnableReceiver(uart);

    // Clear the interrupt flag
    __HAL_UART_CLEAR_FLAG(uart, UART_FLAG_TC);

    // Inform the Write() method about the event
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    BaseType_t result = xEventGroupSetBitsFromISR(
        eventGroup, static_cast<EventBits_t>(Event::WriteFinished), &higherPriorityTaskWoken);
    if (result != pdFAIL) {
        // Switch context after returning from ISR if we have just woken a higher-priority task
        portYIELD_FROM_ISR(higherPriorityTaskWoken);
    }
}

void BufferedSerial::IdleISR() {
    uartrxbuff_idle_cb(&rxBuf);
}

void BufferedSerial::FirstHalfReachedISR() {
    uartrxbuff_rxhalf_cb(&rxBuf);
}

void BufferedSerial::SecondHalfReachedISR() {
    uartrxbuff_rxcplt_cb(&rxBuf);
}

void BufferedSerial::Flush() {
    uartrxbuff_reset(&rxBuf);
}

extern "C" void uart2_idle_cb() {
    BufferedSerial::uart2.IdleISR();
}

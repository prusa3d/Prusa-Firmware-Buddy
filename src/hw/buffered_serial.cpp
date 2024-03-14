#include <device/board.h>

#include "buffered_serial.hpp"
#include "stm32f4xx_hal.h"
#include "config.h"
#include <algorithm>
#include "FreeRTOS.h"
#include "bsod.h"
#include "timing.h"
#include <ccm_thread.hpp>
#include <option/has_puppies.h>

#include "log.h"

// FIXME: remove uart2 definition from this file
extern "C" {
#if BOARD_IS_BUDDY || BOARD_IS_XBUDDY
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_rx;
#endif // buddy or xbuddy
extern UART_HandleTypeDef huart6;
extern DMA_HandleTypeDef hdma_usart6_rx;
}

namespace buddy::hw {
LOG_COMPONENT_DEF(BufferedSerial, LOG_SEVERITY_DEBUG);
#if BOARD_IS_BUDDY
static uint8_t uart2rx_data[32];
BufferedSerial BufferedSerial::uart2(&huart2, &hdma_usart2_rx, nullptr, uart2rx_data, sizeof(uart2rx_data), BufferedSerial::CommunicationMode::IT);
#endif // buddy or xbuddy
#if BOARD_IS_XBUDDY
    #if !HAS_PUPPIES()
static uint8_t uart6rx_data[32];
BufferedSerial BufferedSerial::uart6(&huart6, &hdma_usart6_rx, nullptr, uart6rx_data, sizeof(uart6rx_data), BufferedSerial::CommunicationMode::DMA);
    #endif
#endif // xbuddy

BufferedSerial::BufferedSerial(
    UART_HandleTypeDef *uart, DMA_HandleTypeDef *rxDma, BufferedSerial::HalfDuplexSwitchCallback_t halfDuplexSwitchCallback,
    uint8_t *rxBufPool, size_t rxBufPoolSize, BufferedSerial::CommunicationMode txMode)
    : readTimeoutMs(100)
    , writeTimeoutMs(100)
    , isOpen(false)
    , uart(uart)
    , rxDma(rxDma)
    , txMode(txMode)
    , halfDuplexSwitchCallback(halfDuplexSwitchCallback)
    , pendingWrite(false)
    , rxBufPool(rxBufPool)
    , rxBufPoolSize(rxBufPoolSize) {
    eventGroup = xEventGroupCreate();
}

BufferedSerial::~BufferedSerial() {
    Close();
}

void BufferedSerial::Open() {
    if (isOpen) {
        return;
    }

    uartrxbuff_init(&rxBuf, rxDma, rxBufPoolSize, rxBufPool);

    StartReceiving();

    // Clear the buffer
    uartrxbuff_reset(&rxBuf);

    isOpen = true;
}

void BufferedSerial::Close() {
    if (!isOpen) {
        return;
    }

    // Stop receiving new data
    HAL_UART_AbortReceive(uart);

    // Clear the buffer
    uartrxbuff_reset(&rxBuf);

    uartrxbuff_deinit(&rxBuf);

    isOpen = false;
}

size_t BufferedSerial::Read(char *buf, size_t len, bool terminate_on_idle /* = false */) {
    if (!isOpen) {
        return 0;
    }

    size_t read = 0;
    auto tickStart = ticks_ms();

    while (read != len) {
        int ch = uartrxbuff_getchar(&rxBuf);

        if (ch >= 0) {
            buf[read++] = (char)ch;
        } else if (ch == UARTRXBUFF_ERR_OVERFLOW) {
            log_error(BufferedSerial, "Overflow detected");
            break;
        } else if (ch == UARTRXBUFF_ERR_IDLE) {
            if (terminate_on_idle && read > 0) {
                break;
            }
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

    if (halfDuplexSwitchCallback) {
        halfDuplexSwitchCallback(true);
    }
    // initiate the transfer
    pendingWrite = true;

    HAL_StatusTypeDef transmissionReturnStatus = HAL_ERROR;
    if (txMode == CommunicationMode::DMA) {
        assert("Data for DMA cannot be in CCMRAM" && can_be_used_by_dma(reinterpret_cast<uintptr_t>(buf)));
        transmissionReturnStatus = HAL_UART_Transmit_DMA(uart, (uint8_t *)buf, len);
    } else {
        transmissionReturnStatus = HAL_UART_Transmit_IT(uart, (uint8_t *)buf, len);
    }

    if (transmissionReturnStatus != HAL_OK) {
        pendingWrite = false;
        if (halfDuplexSwitchCallback) {
            halfDuplexSwitchCallback(false);
        }
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
    if (!pendingWrite) {
        return;
    }

    // As soon as the transfer is complete, switch back to receiver mode (half duplex)
    if (halfDuplexSwitchCallback) {
        halfDuplexSwitchCallback(false);
    }

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

void BufferedSerial::ErrorRecovery() {
    if (uart->ErrorCode != HAL_UART_ERROR_NONE) {
        log_error(BufferedSerial, "BufferedSerial recovering from error");
        StartReceiving();
    }
}

void BufferedSerial::StartReceiving() {
    // Start receiving data
    assert("Data for DMA cannot be in CCMRAM" && can_be_used_by_dma(reinterpret_cast<uintptr_t>(rxBuf.buffer)));
    HAL_UART_Receive_DMA(uart, rxBuf.buffer, rxBuf.buffer_size);
    if (halfDuplexSwitchCallback) {
        halfDuplexSwitchCallback(false);
    }
}

} // namespace buddy::hw

#if BOARD_IS_BUDDY
extern "C" void uart2_idle_cb() {
    buddy::hw::BufferedSerial::uart2.IdleISR();
}
#endif // boddy or xbuddy
#if BOARD_IS_XBUDDY
    #if !HAS_PUPPIES()
extern "C" void uart6_idle_cb() {
    buddy::hw::BufferedSerial::uart6.IdleISR();
}
    #endif
#endif

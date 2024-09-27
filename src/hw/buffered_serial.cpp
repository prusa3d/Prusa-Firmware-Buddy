#include <hw/buffered_serial.hpp>

#include <algorithm>
#include <ccm_thread.hpp>
#include <common/bsod.h>
#include <common/timing.h>
#include <logging/log.hpp>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

namespace buddy::hw {

#define UARTRXBUFF_ERR_NO_DATA  -1
#define UARTRXBUFF_ERR_OVERFLOW -2
#define UARTRXBUFF_ERR_IDLE     -3

enum {
    UARTRXBUFF_EVT_FIRST_HALF_FULL = (1 << 0),
    UARTRXBUFF_EVT_SECOND_HALF_FULL = (1 << 1),
    UARTRXBUFF_EVT_OVERFLOW_DETECTED = (1 << 2),
    UARTRXBUFF_EVT_IDLE = (1 << 3),

    UARTRXBUFF_EVT_ALL = UARTRXBUFF_EVT_FIRST_HALF_FULL | UARTRXBUFF_EVT_SECOND_HALF_FULL | UARTRXBUFF_EVT_OVERFLOW_DETECTED | UARTRXBUFF_EVT_IDLE,
};

static void set_events_from_isr(EventGroupHandle_t event_group, EventBits_t events) {
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    BaseType_t result = xEventGroupSetBitsFromISR(event_group, events, &higherPriorityTaskWoken);
    if (result != pdFAIL) {
        // Switch context after returning from ISR if we have just woken a higher-priority task
        portYIELD_FROM_ISR(higherPriorityTaskWoken);
    }
}

static void detect_overflow(EventGroupHandle_t event_group) {
    EventBits_t events = xEventGroupGetBitsFromISR(event_group);
    if (events & (UARTRXBUFF_EVT_FIRST_HALF_FULL | UARTRXBUFF_EVT_SECOND_HALF_FULL)) {
        // Why? If both those flags are set, it means we haven't finished reading one half of the buffer,
        //  while the DMA finished writing to the other side (and is going to write to the one we still
        //  haven't finished reading). Therefore, an overflow can happen at any time without us
        //  being aware (or it happened already).
        set_events_from_isr(event_group, UARTRXBUFF_EVT_OVERFLOW_DETECTED);
    }
}

void uartrxbuff_reset(BufferedSerial::uartrxbuff_t *prxbuff) {
    // Clear all the event flags
    xEventGroupClearBits(prxbuff->event_group,
        UARTRXBUFF_EVT_ALL);
    // Reset the current buffer position to the one reported by the DMA peripheral
    prxbuff->buffer_pos
        = prxbuff->buffer_size - prxbuff->phdma->Instance->NDTR;
    prxbuff->idle_at_NDTR = UINT32_MAX;
}

// an alternative approach to solving the first_half_full issue
// as suggested by Alan - do not read the last character from the lower
// half of the buffer unless the first_half_full flag is set
// - i.e. return NO_DATA in such a case and the caller is obliged to
// make another call to uartrxbuff_getchar to read the character.
int uartrxbuff_getchar(BufferedSerial::uartrxbuff_t *prxbuff) {
    int retval;

    uint32_t ndtr = prxbuff->phdma->Instance->NDTR;
    uint8_t cnt = prxbuff->buffer_size - ndtr;

    EventBits_t events = xEventGroupGetBits(prxbuff->event_group);
    EventBits_t events_to_clear = UARTRXBUFF_EVT_IDLE;
    bool first_half_full = events & UARTRXBUFF_EVT_FIRST_HALF_FULL;
    bool second_half_full = events & UARTRXBUFF_EVT_SECOND_HALF_FULL;

    if (events & UARTRXBUFF_EVT_OVERFLOW_DETECTED) {
        retval = UARTRXBUFF_ERR_OVERFLOW;
    } else if (prxbuff->idle_at_NDTR != UINT32_MAX && prxbuff->buffer_pos == (prxbuff->buffer_size - (int)prxbuff->idle_at_NDTR)) {
        // idle occured at this position - return it
        retval = UARTRXBUFF_ERR_IDLE;
        prxbuff->idle_at_NDTR = UINT32_MAX;
    } else if (prxbuff->buffer_pos < (prxbuff->buffer_size / 2)) {
        // We are reading the first half of the buffer
        if (first_half_full || (prxbuff->buffer_pos < cnt)) {
            if ((!first_half_full) && (prxbuff->buffer_pos == (prxbuff->buffer_size / 2) - 1)) {
                // special case - caught the bug - first_half_full is NOT set while we already have the
                // lower part of the buffer full
                retval = UARTRXBUFF_ERR_NO_DATA;
            } else {
                // normal operation
                retval = prxbuff->buffer[prxbuff->buffer_pos++];
                if (prxbuff->buffer_pos == (prxbuff->buffer_size / 2)) {
                    // we just reached second half of the buffer, so let's mark the first half as "not pending"
                    events_to_clear = UARTRXBUFF_EVT_FIRST_HALF_FULL;
                }
            }
        } else {
            retval = UARTRXBUFF_ERR_NO_DATA;
        }
    } else {
        // We are reading the second half of the buffer
        if (second_half_full || (prxbuff->buffer_pos < cnt)) {
            retval = prxbuff->buffer[prxbuff->buffer_pos++];
            if (prxbuff->buffer_pos >= prxbuff->buffer_size) {
                // we reached the end of the buffer, go back to the beginning and clear the "second half is full" flag
                prxbuff->buffer_pos = 0;
                events_to_clear = UARTRXBUFF_EVT_SECOND_HALF_FULL;
            }
        } else {
            retval = UARTRXBUFF_ERR_NO_DATA;
        }
    }

    if (events_to_clear) {
        xEventGroupClearBits(prxbuff->event_group, events_to_clear);
    }

    return retval;
}

LOG_COMPONENT_DEF(BufferedSerial, logging::Severity::debug);

BufferedSerial::BufferedSerial(
    UART_HandleTypeDef *uart, BufferedSerial::HalfDuplexSwitchCallback_t halfDuplexSwitchCallback,
    uint8_t *rxBufPool, size_t rxBufPoolSize, BufferedSerial::CommunicationMode txMode)
    : readTimeoutMs(100)
    , writeTimeoutMs(100)
    , isOpen(false)
    , uart(uart)
    , txMode(txMode)
    , halfDuplexSwitchCallback(halfDuplexSwitchCallback)
    , pendingWrite(false)
    , rxBufPool(rxBufPool)
    , rxBufPoolSize(rxBufPoolSize) {
    eventGroup = xEventGroupCreate(); // Event group for writing (Event group for reading is in rxBuff)
}

BufferedSerial::~BufferedSerial() {
    Close();
}

void BufferedSerial::Open() {
    if (isOpen) {
        uartrxbuff_reset(&rxBuf);
        return;
    }

    memset(&rxBuf, 0, sizeof(BufferedSerial::uartrxbuff_t));
    assert(rxBufPoolSize <= 256); // uint8_t is used
    rxBuf.phdma = uart->hdmarx;
    rxBuf.buffer_size = rxBufPoolSize;
    rxBuf.buffer = rxBufPool;
    rxBuf.event_group = xEventGroupCreate();
    uartrxbuff_reset(&rxBuf);

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

    vEventGroupDelete(rxBuf.event_group);

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

            uint32_t timeout = std::min(budgetMs, 1);
            TickType_t ticks = timeout / portTICK_PERIOD_MS;
            xEventGroupWaitBits(rxBuf.event_group,
                UARTRXBUFF_EVT_FIRST_HALF_FULL | UARTRXBUFF_EVT_SECOND_HALF_FULL | UARTRXBUFF_EVT_OVERFLOW_DETECTED | UARTRXBUFF_EVT_IDLE,
                /*xClearOnExit=*/pdFALSE,
                /*xWaitForAllBits=*/pdFALSE,
                /*xTicksToWait=*/ticks);
            xEventGroupClearBits(rxBuf.event_group, UARTRXBUFF_EVT_IDLE);
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
        assert(can_be_used_by_dma(buf));
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
    rxBuf.idle_at_NDTR = rxBuf.phdma->Instance->NDTR;
    set_events_from_isr(rxBuf.event_group, UARTRXBUFF_EVT_IDLE);
}

void BufferedSerial::FirstHalfReachedISR() {
    set_events_from_isr(rxBuf.event_group, UARTRXBUFF_EVT_FIRST_HALF_FULL);
    detect_overflow(rxBuf.event_group);
}

void BufferedSerial::SecondHalfReachedISR() {
    set_events_from_isr(rxBuf.event_group, UARTRXBUFF_EVT_SECOND_HALF_FULL);
    detect_overflow(rxBuf.event_group);
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
    assert(can_be_used_by_dma(rxBuf.buffer));
    HAL_UART_Receive_DMA(uart, rxBuf.buffer, rxBuf.buffer_size);
    if (halfDuplexSwitchCallback) {
        halfDuplexSwitchCallback(false);
    }
}

} // namespace buddy::hw

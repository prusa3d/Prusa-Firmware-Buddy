#include "uartrxbuff.h"
#include "bsod.h"
#include <string.h>

static void uartrxbuff_set_events_from_isr(uartrxbuff_t *prxbuff, EventBits_t events) {
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    BaseType_t result = xEventGroupSetBitsFromISR(prxbuff->event_group, events, &higherPriorityTaskWoken);
    if (result != pdFAIL) {
        // Switch context after returning from ISR if we have just woken a higher-priority task
        portYIELD_FROM_ISR(higherPriorityTaskWoken);
    }
}

static bool uartrxbuff_detect_overflow(uartrxbuff_t *prxbuff) {
    EventBits_t events = xEventGroupGetBitsFromISR(prxbuff->event_group);
    if (events & (UARTRXBUFF_EVT_FIRST_HALF_FULL | UARTRXBUFF_EVT_SECOND_HALF_FULL)) {
        // Why? If both those flags are set, it means we haven't finished reading one half of the buffer,
        //  while the DMA finished writing to the other side (and is going to write to the one we still
        //  haven't finished reading). Therefore, an overflow can happen at any time without us
        //  being aware (or it happened already).
        uartrxbuff_set_events_from_isr(prxbuff, UARTRXBUFF_EVT_OVERFLOW_DETECTED);
        return true;
    } else if (events & UARTRXBUFF_EVT_OVERFLOW_DETECTED) {
        // We have already detected an overflow without clearing the flag. Still an overflow!
        return true;
    } else {
        return false;
    }
}

void uartrxbuff_init(uartrxbuff_t *prxbuff, UART_HandleTypeDef *phuart, DMA_HandleTypeDef *phdma, int size, uint8_t *pdata) {
    memset(prxbuff, 0, sizeof(uartrxbuff_t));
    prxbuff->phdma = phdma;
    prxbuff->buffer_size = size;
    prxbuff->buffer = pdata;
    prxbuff->event_group = xEventGroupCreate();
}

void uartrxbuff_reset(uartrxbuff_t *prxbuff) {
    // Clear all the event flags
    xEventGroupClearBits(prxbuff->event_group,
        UARTRXBUFF_EVT_FIRST_HALF_FULL | UARTRXBUFF_EVT_SECOND_HALF_FULL
            | UARTRXBUFF_EVT_OVERFLOW_DETECTED);
    // Reset the current buffer position to the one reported by the DMA peripheral
    prxbuff->buffer_pos
        = prxbuff->buffer_size - prxbuff->phdma->Instance->NDTR;
}

int uartrxbuff_getchar(uartrxbuff_t *prxbuff) {
    int retval;

    uint32_t ndtr = prxbuff->phdma->Instance->NDTR;
    uint8_t cnt = prxbuff->buffer_size - ndtr;

    EventBits_t events = xEventGroupGetBits(prxbuff->event_group);
    EventBits_t events_to_clear = UARTRXBUFF_EVT_IDLE;
    bool first_half_full = events & UARTRXBUFF_EVT_FIRST_HALF_FULL;
    bool second_half_full = events & UARTRXBUFF_EVT_SECOND_HALF_FULL;

    if (events & UARTRXBUFF_EVT_OVERFLOW_DETECTED) {
        retval = UARTRXBUFF_ERR_OVERFLOW;
    } else if (prxbuff->buffer_pos < (prxbuff->buffer_size / 2)) {
        // We are reading the first half of the buffer
        if (first_half_full || (prxbuff->buffer_pos < cnt)) {
            retval = prxbuff->buffer[prxbuff->buffer_pos++];
            if (prxbuff->buffer_pos == (prxbuff->buffer_size / 2)) {
                // we just reached second half of the buffer, so let's mark the first half as "not pending"
                events_to_clear = UARTRXBUFF_EVT_FIRST_HALF_FULL;
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

    if (events_to_clear)
        xEventGroupClearBits(prxbuff->event_group, events_to_clear);

    return retval;
}

void uartrxbuff_rxhalf_cb(uartrxbuff_t *prxbuff) {
    // the DMA peripheral filled the first half of the buffer
    uartrxbuff_set_events_from_isr(prxbuff, UARTRXBUFF_EVT_FIRST_HALF_FULL);
    uartrxbuff_detect_overflow(prxbuff);
}

void uartrxbuff_rxcplt_cb(uartrxbuff_t *prxbuff) {
    // the DMA peripheral filled the second half of the buffer
    uartrxbuff_set_events_from_isr(prxbuff, UARTRXBUFF_EVT_SECOND_HALF_FULL);
    uartrxbuff_detect_overflow(prxbuff);
}

void uartrxbuff_idle_cb(uartrxbuff_t *prxbuff) {
    uartrxbuff_set_events_from_isr(prxbuff, UARTRXBUFF_EVT_IDLE);
}

void uartrxbuff_wait_for_event(uartrxbuff_t *prxbuff, uint32_t timeout) {
    TickType_t ticks = timeout / portTICK_PERIOD_MS;
    xEventGroupWaitBits(prxbuff->event_group,
        UARTRXBUFF_EVT_FIRST_HALF_FULL | UARTRXBUFF_EVT_SECOND_HALF_FULL | UARTRXBUFF_EVT_OVERFLOW_DETECTED | UARTRXBUFF_EVT_IDLE,
        /*xClearOnExit=*/pdFALSE,
        /*xWaitForAllBits=*/pdFALSE,
        /*xTicksToWait=*/ticks);
    xEventGroupClearBits(prxbuff->event_group, UARTRXBUFF_EVT_IDLE);
}

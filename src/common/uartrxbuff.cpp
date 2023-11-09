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

void uartrxbuff_init(uartrxbuff_t *prxbuff, DMA_HandleTypeDef *phdma, int size, uint8_t *pdata) {
    memset(prxbuff, 0, sizeof(uartrxbuff_t));
    prxbuff->phdma = phdma;
    prxbuff->buffer_size = size;
    assert(size <= 256); // uint8_t is used
    prxbuff->buffer = pdata;
    prxbuff->event_group = xEventGroupCreate();
    uartrxbuff_reset(prxbuff);
}

void uartrxbuff_deinit(uartrxbuff_t *prxbuff) {
    vEventGroupDelete(prxbuff->event_group);
}

void uartrxbuff_reset(uartrxbuff_t *prxbuff) {
    // Clear all the event flags
    xEventGroupClearBits(prxbuff->event_group,
        UARTRXBUFF_EVT_ALL);
    // Reset the current buffer position to the one reported by the DMA peripheral
    prxbuff->buffer_pos
        = prxbuff->buffer_size - prxbuff->phdma->Instance->NDTR;
    prxbuff->idle_at_NDTR = UINT32_MAX;
}

#if 0
// First attempt to workaround a mysterious bug when
// first half of the RX buffer is really full, but the first_half_full is NOT set
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
                // verify the hypothesis of the incorrectly unset flag
                // if( ! first_half_full )... yes, it really happens - looks like some synchronization issues, not sure where they could be
            }
        } else {
            retval = UARTRXBUFF_ERR_NO_DATA;
        }
    } else {
        if (first_half_full) {
            // This really happens and it is logical (after a day of debugging)
            // The problem lies above in the assumption, that once
            // `if (prxbuff->buffer_pos == (prxbuff->buffer_size / 2))` is true
            // also UARTRXBUFF_EVT_FIRST_HALF_FULL was set - which may OR MAY NOT be true
            // and it is unclear yet why that might happen (looks like a synchronization issue of some kind)
            //
            // Therefore we must check once again here for the first_half_full
            // and if it is true, we must clear the flag.
            // We can safely do it since we are already reading the second half of the buffer
            // which obviously means we already read the first half of the buffer completely.
            //
            // Surprisingly I haven't seen a similar situation with the second half of the buffer yet.
            //
            // An alternative solution would be to block reading the last character from the first half of the buffer
            // until the UARTRXBUFF_EVT_FIRST_HALF_FULL is set correctly.
            // However, that would bring an unnecessary latency problem requiring a "second read call".
            // Hacking the UARTRXBUFF_EVT_FIRST_HALF_FULL at this spot just cleans up the rubbish without causing any delays
            // (but leaves the read buffer in an inconsistent state for a while).
            // Both approaches are acceptable for communication with the MMU, but hard to say about others.
            events_to_clear = UARTRXBUFF_EVT_FIRST_HALF_FULL;
        }
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
#else
// an alternative approach to solving the first_half_full issue
// as suggested by Alan - do not read the last character from the lower
// half of the buffer unless the first_half_full flag is set
// - i.e. return NO_DATA in such a case and the caller is obliged to
// make another call to uartrxbuff_getchar to read the character.
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
#endif
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
    prxbuff->idle_at_NDTR = prxbuff->phdma->Instance->NDTR;
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

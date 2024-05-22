#include "log_dest_swo.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"
#include "log_dest_shared.h"
#include <common/freertos_mutex.hpp>
#include <task.h>

// Note: This is not required to be in CCMRAM and can be moved to regular RAM if needed.
static __attribute__((section(".ccmram"))) freertos::Mutex swo_mutex;

static bool swo_is_enabled() {
    return (((ITM->TCR & ITM_TCR_ITMENA_Msk) != 0UL) && // ITM enabled
        ((ITM->TER & 1UL) != 0UL)); // ITM Port #0 enabled
}

/// Send one character over SWO (blocking)
static void swo_put_char(char character, [[maybe_unused]] void *arg) {
    while (ITM->PORT[0U].u32 == 0UL) {
        __NOP();
    }
    ITM->PORT[0U].u8 = (uint8_t)character;
}

static void swo_log_event_unlocked(log_destination_t *destination, log_event_t *event) {
    destination->log_format_fn(event, swo_put_char, NULL);
    swo_put_char('\r', NULL);
    swo_put_char('\n', NULL);
}

void swo_log_event(log_destination_t *destination, log_event_t *event) {
    if (!swo_is_enabled()) {
        return;
    }

    // send the log message
    // (even if we didn't acquire the lock; we might break some logs in the console,
    // but that seems like a better option than silently suppressing the logs)
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING && !xPortIsInsideInterrupt()) {
        std::unique_lock lock { swo_mutex };
        swo_log_event_unlocked(destination, event);
    } else {
        swo_log_event_unlocked(destination, event);
    }
}

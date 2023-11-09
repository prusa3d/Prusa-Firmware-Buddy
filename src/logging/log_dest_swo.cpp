#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"

#include "log_dest_swo.h"
#include "log_dest_shared.h"

static SemaphoreHandle_t swo_lock;

static inline bool initialize_swo_lock() {
    if (swo_lock) {
        return true;
    }

    if (xPortIsInsideInterrupt() || xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
        return false;
    }

    swo_lock = xSemaphoreCreateBinary();
    xSemaphoreGive(swo_lock);
    return true;
}

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

void swo_log_event(log_destination_t *destination, log_event_t *event) {
    if (!swo_is_enabled()) {
        return;
    }

    const bool lock_initialized = initialize_swo_lock();
    bool lock_acquired = false;

    // acquire the lock
    if (lock_initialized && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING && !xPortIsInsideInterrupt()) {
        xSemaphoreTake(swo_lock, portMAX_DELAY);
        lock_acquired = true;
    }

    // send the log message
    // (even if we didn't acquire the lock; we might break some logs in the console,
    // but that seems like a better option than silently suppressing the logs)
    destination->log_format_fn(event, swo_put_char, NULL);
    ITM_SendChar('\r');
    ITM_SendChar('\n');

    // release the lock
    if (lock_acquired) {
        xSemaphoreGive(swo_lock);
    }
}

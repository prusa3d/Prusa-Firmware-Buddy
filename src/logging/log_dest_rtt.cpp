#include "log_dest_rtt.h"
#include "cmsis_os.h"
#include "SEGGER_RTT.h"

static SemaphoreHandle_t rtt_lock;

static void rtt_put_char(char character, [[maybe_unused]] void *arg) {
    SEGGER_RTT_PutCharSkipNoLock(0, character);
}

static inline void initialize_rtt_subsystem() {
    static bool subsystem_initialized = false;
    if (subsystem_initialized == false) {
        SEGGER_RTT_Init();
        subsystem_initialized = true;
    }
}

static inline bool initialize_rtt_lock() {
    if (rtt_lock) {
        return true;
    }

    if (xPortIsInsideInterrupt() || xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
        return false;
    }

    rtt_lock = xSemaphoreCreateBinary();
    xSemaphoreGive(rtt_lock);
    return true;
}

void rtt_log_event(log_destination_t *destination, log_event_t *event) {
    initialize_rtt_subsystem();

    const bool lock_initialized = initialize_rtt_lock();
    bool lock_acquired = false;

    // acquire the lock
    if (lock_initialized && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING && !xPortIsInsideInterrupt()) {
        xSemaphoreTake(rtt_lock, portMAX_DELAY);
        lock_acquired = true;
    }

    // send the log message
    destination->log_format_fn(event, rtt_put_char, NULL);
    rtt_put_char('\r', NULL);
    rtt_put_char('\n', NULL);

    // release the lock
    if (lock_acquired) {
        xSemaphoreGive(rtt_lock);
    }
}

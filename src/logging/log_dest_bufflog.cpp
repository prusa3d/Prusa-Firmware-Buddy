#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "printf.h"

#include "log_dest_bufflog.h"
#include "log_platform.h"
#include "otp.hpp"

#define BUFFLOG_BUFFER_SIZE 256

osMutexDef(bufflog_buffer_lock);
osMutexId bufflog_buffer_lock_id;

static bool initialized = false;

typedef struct {
    char *data;
    size_t read;
    size_t write;
} buffer_output_state_t;

static void buffer_output(char character, void *arg) {
    buffer_output_state_t *state = (buffer_output_state_t *)arg;
    const size_t next = (state->write + 1) % BUFFLOG_BUFFER_SIZE;
    if (next != state->read) {
        state->write = next;
        state->data[state->write] = character;
    }
}

void bufflog_initialize() {
    if (initialized) {
        return;
    }

    bufflog_buffer_lock_id = osMutexCreate(osMutex(bufflog_buffer_lock));
    initialized = true;
}

static char buffer[BUFFLOG_BUFFER_SIZE];
static buffer_output_state_t buffer_state = {
    .data = buffer,
    .read = 0,
    .write = 0,
};

void bufflog_log_event(log_destination_t *destination, log_event_t *event) {
    // initialize the bufflog buffer if it is safe to do so
    if (!initialized && !xPortIsInsideInterrupt() && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING && !log_platform_is_low_on_resources()) {
        bufflog_initialize();
    } else if (!initialized) {
        return;
    }

    // prepare the message
    if (osMutexWait(bufflog_buffer_lock_id, osWaitForever) != osOK) {
        return;
    }

    destination->log_format_fn(event, buffer_output, &buffer_state);
    buffer_output(BUFFLOG_TERMINATION_CHAR, &buffer_state);

    osMutexRelease(bufflog_buffer_lock_id);
}

static size_t min(size_t a, size_t b) {
    return a < b ? a : b;
}

size_t bufflog_pickup(char *dest, size_t buffer_size) {
    if (osMutexWait(bufflog_buffer_lock_id, osWaitForever) != osOK) {
        return 0;
    }

    const size_t available = (buffer_state.write - buffer_state.read) % BUFFLOG_BUFFER_SIZE;
    const size_t to_copy = min(buffer_size, available);

    // Copy data to buffer - read pos to end
    const size_t first_part_size = min(to_copy, BUFFLOG_BUFFER_SIZE - buffer_state.read);
    memcpy(dest, buffer + buffer_state.read, first_part_size);
    // Copy data to buffer - begin to write pos
    memcpy(dest + first_part_size, buffer, to_copy - first_part_size);

    buffer_state.read = (buffer_state.read + to_copy) % BUFFLOG_BUFFER_SIZE;

    osMutexRelease(bufflog_buffer_lock_id);
    return to_copy;
}

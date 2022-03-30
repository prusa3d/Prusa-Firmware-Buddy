#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "tcpip.h"
#include "printf.h"

#include "log_dest_syslog.h"
#include "log_platform.h"
#include "syslog.h"
#include "otp.h"

osMutexDef(syslog_buffer_lock);
osMutexId syslog_buffer_lock_id;

static bool initialized = false;
static char *remote_ip_address = "";
static int remote_port = 6514;
static syslog_transport_t syslog_transport = { 0 };

static int log_severity_to_syslog_severity(log_severity_t severity) {
    switch (severity) {
    case LOG_SEVERITY_DEBUG:
        return 7;
    case LOG_SEVERITY_INFO:
        return 6;
    case LOG_SEVERITY_WARNING:
        return 4;
    case LOG_SEVERITY_ERROR:
        return 3;
    case LOG_SEVERITY_CRITICAL:
    default:
        return 2;
    }
}

typedef struct {
    char *buffer;
    int buffer_len;
    int used;
} buffer_output_state_t;

void buffer_output(char character, void *arg) {
    buffer_output_state_t *state = (buffer_output_state_t *)arg;
    if (state->buffer_len - state->used > 0) {
        state->buffer[state->used] = character;
        state->used += 1;
    }
}

void syslog_initialize() {
    if (initialized)
        return;

    syslog_buffer_lock_id = osMutexCreate(osMutex(syslog_buffer_lock));
    initialized = true;
}

void syslog_format_event(log_event_t *event, void (*out_fn)(char character, void *arg), void *arg) {
    const int facility = 1; // user level message
    int severity = log_severity_to_syslog_severity(event->severity);
    int priority = facility * 8 + severity;
    const char *hostname = otp_get_mac_address_str();
    const char *appname = "buddy";
    fctprintf(out_fn, arg, "<%i>1 - %s %s %s - - ", priority, hostname, appname, event->component->name);
    vfctprintf(out_fn, arg, event->fmt, *event->args);
}

void syslog_log_event(log_destination_t *destination, log_event_t *event) {
    // initialize the syslog buffer if it is safe to do so
    if (!initialized && !xPortIsInsideInterrupt() && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING && !log_platform_is_low_on_resources()) {
        syslog_initialize();
    } else if (!initialized) {
        return;
    }

    // do not use syslog in case we are running out of stack or we are within an ISR
    if (xPortIsInsideInterrupt() || log_platform_is_low_on_resources())
        return;

    // check that we are not logging from within the LwIP stack
    // as calling LwIP again "from the outside" would cause a deadlock
    if (lock_tcpip_core == 0 || osSemaphoreGetCount(lock_tcpip_core) == 0)
        return;

    // prevent (infinite) recursion within the syslog
    // handler (for example, the syslog_transport might try to log something)
    if ((intptr_t)pvTaskGetThreadLocalStoragePointer(NULL, THREAD_LOCAL_STORAGE_SYSLOG_IDX) == 1)
        return;
    vTaskSetThreadLocalStoragePointer(NULL, THREAD_LOCAL_STORAGE_SYSLOG_IDX, (void *)1);

    // is the transport ready?
    bool open = syslog_transport_check_is_open(&syslog_transport);
    // if not, try to open it
    if (!open)
        open = syslog_transport_open(&syslog_transport, remote_ip_address, remote_port);
    // don't bother with the message if we still don't have an open transport
    if (!open)
        goto cleanup_and_return;

    // prepare the message
    static char buffer[128];
    if (osMutexWait(syslog_buffer_lock_id, osWaitForever) != osOK)
        goto cleanup_and_return;
    buffer_output_state_t buffer_state = {
        .buffer = buffer,
        .buffer_len = sizeof(buffer),
        .used = 0,
    };
    destination->log_format_fn(event, buffer_output, &buffer_state);

    // try to send the message if we have an open socket
    bool sent = syslog_transport_send(&syslog_transport, buffer, buffer_state.used);
    if (!sent)
        syslog_transport_close(&syslog_transport);

    osMutexRelease(syslog_buffer_lock_id);
cleanup_and_return:
    vTaskSetThreadLocalStoragePointer(NULL, THREAD_LOCAL_STORAGE_SYSLOG_IDX, (void *)0);
}

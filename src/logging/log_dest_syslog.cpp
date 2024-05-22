#include "FreeRTOS.h"
#include "tcpip.h"
#include "printf.h"

#include "log_dest_syslog.h"
#include "log_platform.h"
#include "syslog_transport.hpp"
#include "otp.hpp"
#include <option/development_items.h>
#include <config_store/store_instance.hpp>
#include <common/freertos_mutex.hpp>

#include <atomic>

// Note: These are not required to be in CCMRAM and can be moved to regular RAM if needed.
static __attribute__((section(".ccmram"))) freertos::Mutex syslog_buffer_mutex;
static __attribute__((section(".ccmram"))) SyslogTransport syslog_transport;

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
    char *data;
    int len;
    int used;
} buffer_output_state_t;

void buffer_output(char character, void *arg) {
    buffer_output_state_t *state = (buffer_output_state_t *)arg;
    if (state->len - state->used > 0) {
        state->data[state->used] = character;
        state->used += 1;
    }
}

void syslog_initialize() {
    // Init host and port from eeprom
    const MetricsAllow metrics_allow = config_store().metrics_allow.get();
    if ((metrics_allow == MetricsAllow::One || metrics_allow == MetricsAllow::All)
        && config_store().metrics_init.get()) {
        const char *host = config_store().metrics_host.get_c_str();
        const uint16_t port = config_store().syslog_port.get();
        syslog_configure(host, port);
    }
}

void syslog_format_event(log_event_t *event, void (*out_fn)(char character, void *arg), void *arg) {
    const int facility = 1; // user level message
    int severity = log_severity_to_syslog_severity(event->severity);
    int priority = facility * 8 + severity;
    auto hostname = otp_get_mac_address_str();
    const char *appname = "buddy";
    fctprintf(out_fn, arg, "<%i>1 - %s %s %s - - ", priority, hostname.data(), appname, event->component->name);
    vfctprintf(out_fn, arg, event->fmt, *event->args);
}

void syslog_log_event(log_destination_t *destination, log_event_t *event) {
    // do not use syslog in case we are running out of stack or we are within an ISR
    if (xPortIsInsideInterrupt() || log_platform_is_low_on_resources()) {
        return;
    }

    // check that we are not logging from within the LwIP stack
    // as calling LwIP again "from the outside" would cause a deadlock
    if (lock_tcpip_core == 0 || osSemaphoreGetCount(lock_tcpip_core) == 0) {
        return;
    }

    // prevent (infinite) recursion within the syslog
    // handler (for example, the syslog_transport might try to log something)
    if ((intptr_t)pvTaskGetThreadLocalStoragePointer(NULL, THREAD_LOCAL_STORAGE_SYSLOG_IDX) == 1) {
        return;
    }
    vTaskSetThreadLocalStoragePointer(NULL, THREAD_LOCAL_STORAGE_SYSLOG_IDX, (void *)1);

    // prepare the message
    std::unique_lock lock { syslog_buffer_mutex };
    static char buffer[128];
    buffer_output_state_t buffer_state = {
        .data = buffer,
        .len = sizeof(buffer),
        .used = 0,
    };
    destination->log_format_fn(event, buffer_output, &buffer_state);

    syslog_transport.send(buffer, buffer_state.used);

    vTaskSetThreadLocalStoragePointer(NULL, THREAD_LOCAL_STORAGE_SYSLOG_IDX, (void *)0);
}

const char *syslog_get_host() {
    return syslog_transport.get_remote_host();
}

uint16_t syslog_get_port() {
    return syslog_transport.get_remote_port();
}

void syslog_configure(const char *ip, uint16_t port) {
    syslog_transport.reopen(ip, port);
}

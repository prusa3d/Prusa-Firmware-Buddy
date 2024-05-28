#include <logging/log_dest_syslog.hpp>

#include "tcpip.h"
#include <printf/printf.h>
#include <syslog/syslog_transport.hpp>
#include <common/otp.hpp>
#include <config_store/store_instance.hpp>

#include <atomic>

namespace logging {

// Note: These are not required to be in CCMRAM and can be moved to regular RAM if needed.
static __attribute__((section(".ccmram"))) SyslogTransport syslog_transport;

static int log_severity_to_syslog_severity(Severity severity) {
    switch (severity) {
    case logging::Severity::debug:
        return 7;
    case logging::Severity::info:
        return 6;
    case logging::Severity::warning:
        return 4;
    case logging::Severity::error:
        return 3;
    case logging::Severity::critical:
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

void syslog_reconfigure() {
    if (config_store().enable_metrics.get()) {
        const auto host = config_store().metrics_host.get();
        const auto port = config_store().syslog_port.get();
        syslog_transport.reopen(host.data(), port);

    } else {
        syslog_transport.reopen(nullptr, 0);
    }
}

void syslog_format_event(FormattedEvent *event, void (*out_fn)(char character, void *arg), void *arg) {
    const int facility = 1; // user level message
    int severity = log_severity_to_syslog_severity(event->severity);
    int priority = facility * 8 + severity;
    auto hostname = otp_get_mac_address_str();
    const char *appname = "buddy";
    fctprintf(out_fn, arg, "<%i>1 - %s %s %s - - %s", priority, hostname.data(), appname, event->component->name, event->message);
}

void syslog_log_event(FormattedEvent *event) {
    // check that we are not logging from within the LwIP stack
    // as calling LwIP again "from the outside" would cause a deadlock
    if (lock_tcpip_core == 0 || osSemaphoreGetCount(lock_tcpip_core) == 0) {
        return;
    }

    // prepare the message
    static char buffer[128];
    buffer_output_state_t buffer_state = {
        .data = buffer,
        .len = sizeof(buffer),
        .used = 0,
    };
    syslog_format_event(event, buffer_output, &buffer_state);

    syslog_transport.send(buffer, buffer_state.used);
}

} // namespace logging

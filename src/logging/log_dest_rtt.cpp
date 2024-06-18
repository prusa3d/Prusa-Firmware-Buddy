#include <logging/log_dest_rtt.hpp>

#include <logging/log_dest_shared.hpp>
#include "SEGGER_RTT.h"

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

namespace logging {

void rtt_log_event(FormattedEvent *event) {
    initialize_rtt_subsystem();
    log_format_simple(event, rtt_put_char, NULL);
    rtt_put_char('\r', NULL);
    rtt_put_char('\n', NULL);
}

} // namespace logging

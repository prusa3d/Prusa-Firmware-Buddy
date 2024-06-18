#pragma once

#include <logging/log.hpp>

namespace logging {

/// Send the event via Segger's RTT (in a non-blocking way, so data can get lost)
void rtt_log_event(FormattedEvent *event);

} // namespace logging

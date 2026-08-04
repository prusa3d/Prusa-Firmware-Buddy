#pragma once

#include <logging/log.hpp>

extern "C" {
/// Log an event over the Cyphal Record tooling.
///
/// Events logged before the Cyphal node exists are discarded.
///
/// @warning Do not use from the cyphal task or its callbacks.
void cyphal_log_event(logging::FormattedEvent *event);
}

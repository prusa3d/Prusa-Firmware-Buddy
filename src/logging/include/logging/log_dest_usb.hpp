#pragma once

#include <logging/log.hpp>

namespace logging {

/// Send the event via USB CDC
void usb_log_event(FormattedEvent *event);

void usb_log_enable();

void usb_log_disable();

} // namespace logging

#pragma once

#include <logging/log.hpp>

namespace logging {

void log_format_simple(FormattedEvent *event, void (*out_fn)(char character, void *arg), void *arg);
void log_format_honeybee_node(FormattedEvent *event, void (*out_fn)(char character, void *arg), void *arg);

} // namespace logging

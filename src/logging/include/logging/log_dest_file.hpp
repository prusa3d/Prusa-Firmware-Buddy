#pragma once

#include <logging/log.hpp>

namespace logging {

void file_log_event(FormattedEvent *event);

/// Opens a given file for appending and starts logging to it
/// \returns whether the file open was successful
bool file_log_enable(const char *filepath);

void file_log_disable();

bool file_log_is_enabled();

} // namespace logging

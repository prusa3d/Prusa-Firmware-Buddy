#pragma once

#include <logging/log.hpp>

namespace logging {

void log_format_simple(Event *event, void (*out_fn)(char character, void *arg), void *arg);

}

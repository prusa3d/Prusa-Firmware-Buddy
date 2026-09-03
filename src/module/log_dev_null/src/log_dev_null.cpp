#include <logging/log.hpp>

namespace logging {

void _log_event(Severity, const Component *, const char *, ...) {}

void _log_event_valist(Severity, const Component *, const char *, va_list *) {}

} // namespace logging

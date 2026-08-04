#include <logging/log.hpp>

// Sink-only stub for tests of libraries that log but do not assert on log output.
//
// Deliberately not log_dev_null: linking the real logging headers keeps the format strings
// and their arguments type-checked in the host build too, instead of only in the firmware one.
namespace logging {

void _log_event(Severity, const Component *, const char *, ...) {}

void _log_event_valist(Severity, const Component *, const char *, va_list *) {}

} // namespace logging

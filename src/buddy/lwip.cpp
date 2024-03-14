#include "log.h"

LOG_COMPONENT_REF(Network);

extern "C" void lwip_platform_assert(const char *message, const char *file, int line) {
    log_critical(Network, "assertion failed: %s at %s:%d", message, file, line);
}

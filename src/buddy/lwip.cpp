#include "log.h"
#include <lwip/tcpip.h>
#include <semphr.h>
#include <task.h>

LOG_COMPONENT_REF(Network);

extern "C" void lwip_platform_assert(const char *message, const char *file, int line) {
    log_critical(Network, "assertion failed: %s at %s:%d", message, file, line);
}

namespace {
bool sys_mutex_is_held(SemaphoreHandle_t *mutex) {
    return (xSemaphoreGetMutexHolder(*mutex) == xTaskGetCurrentTaskHandle());
}
} // namespace

extern "C" void wui_lwip_assert_core_locked() {
    if (!sys_mutex_is_held(&lock_tcpip_core)) {
        bsod("LWIP_ASSERT_CORE_LOCKED");
    }
}

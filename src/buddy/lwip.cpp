#include <lwip/tcpip.h>
#include <semphr.h>
#include <task.h>
#include <bsod/bsod.h>

extern "C" void lwip_platform_assert(const char *message, const char *file, int line) {
    _bsod("LwIP assert %s", file, line, message);
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

#include <lwip/tcpip.h>
#include <semphr.h>
#include <task.h>
#include <bsod/bsod.h>

/// Disabling interprocedural optimizations prevents compiler from inlining
/// and propagating bsod's noreturn attribute. This gives us distinct address
/// for each callsite for free and we can recover exact assert from crash dump.
extern "C" [[gnu::noipa]] void lwip_platform_assert() {
    const auto *return_address = __builtin_extract_return_addr(__builtin_return_address(0));
    _bsod("LwIP assert %p", "", 0, return_address);
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

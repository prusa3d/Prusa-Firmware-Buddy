#include <tasks.hpp>

#include <bsod/bsod.h>

namespace TaskDeps {

EventGroupHandle_t components_ready;

void components_init() {
    components_ready = xEventGroupCreate();
    debug_assert(components_ready);
}

} // namespace TaskDeps

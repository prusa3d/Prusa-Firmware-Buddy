#include <tasks.hpp>

#include <cassert>

namespace TaskDeps {

EventGroupHandle_t components_ready;

void components_init() {
    components_ready = xEventGroupCreate();
    assert(components_ready);
}

} // namespace TaskDeps

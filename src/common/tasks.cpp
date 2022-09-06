#include "tasks.h"

#include <cassert>

EventGroupHandle_t components_ready;

void components_init() {
    components_ready = xEventGroupCreate();
    assert(components_ready);
}

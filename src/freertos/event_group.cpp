/// @file
#include <freertos/event_group.hpp>

// FreeRTOS.h must be included before event_groups.h
#include <FreeRTOS.h>
#include <event_groups.h>

#include "ticks.hpp"

#include <type_traits>

namespace freertos {

EventGroup::EventGroup() {
    // If these asserts start failing, go fix the constants.
    static_assert(event_group_storage_size == sizeof(StaticEventGroup_t));
    static_assert(event_group_storage_align == alignof(StaticEventGroup_t));
    static_assert(std::is_same_v<Bits, EventBits_t>);

    handle = xEventGroupCreateStatic(reinterpret_cast<StaticEventGroup_t *>(&event_group_storage));
}

EventGroup::~EventGroup() {
    vEventGroupDelete(EventGroupHandle_t(handle));
}

EventGroup::Bits EventGroup::set(Bits bits) {
    return xEventGroupSetBits(EventGroupHandle_t(handle), bits);
}

bool EventGroup::set_from_isr(Bits bits) {
    BaseType_t higher_priority_task_woken = pdFALSE;
    const bool success = xEventGroupSetBitsFromISR(EventGroupHandle_t(handle), bits, &higher_priority_task_woken) == pdTRUE;
    portYIELD_FROM_ISR(higher_priority_task_woken);
    return success;
}

EventGroup::Bits EventGroup::clear(Bits bits) {
    return xEventGroupClearBits(EventGroupHandle_t(handle), bits);
}

bool EventGroup::clear_from_isr(Bits bits) {
    return xEventGroupClearBitsFromISR(EventGroupHandle_t(handle), bits) == pdPASS;
}

EventGroup::Bits EventGroup::get_from_isr() {
    return xEventGroupGetBitsFromISR(EventGroupHandle_t(handle));
}

EventGroup::Bits EventGroup::wait_for_all(Bits bits, bool clear_on_exit) {
    return xEventGroupWaitBits(EventGroupHandle_t(handle), bits, clear_on_exit, pdTRUE, portMAX_DELAY);
}

EventGroup::Bits EventGroup::wait_for_any(Bits bits, bool clear_on_exit) {
    return xEventGroupWaitBits(EventGroupHandle_t(handle), bits, clear_on_exit, pdFALSE, portMAX_DELAY);
}

EventGroup::Bits EventGroup::try_wait_for_all(Bits bits, bool clear_on_exit, uint32_t timeout_ms) {
    return xEventGroupWaitBits(EventGroupHandle_t(handle), bits, clear_on_exit, pdTRUE, ms_to_ticks(timeout_ms));
}

EventGroup::Bits EventGroup::try_wait_for_any(Bits bits, bool clear_on_exit, uint32_t timeout_ms) {
    return xEventGroupWaitBits(EventGroupHandle_t(handle), bits, clear_on_exit, pdFALSE, ms_to_ticks(timeout_ms));
}

} // namespace freertos

#include <logging/log.hpp>

#include <common/timing.h>

#include <FreeRTOS.h> // must appear in source files before include task.h
#include <task.h>

namespace logging {

Timestamp log_platform_timestamp_get() {
    auto timestamp = get_timestamp();
    return { timestamp.sec, timestamp.us };
}

TaskId log_platform_task_id_get() {
    TaskHandle_t task_handle = xTaskGetCurrentTaskHandle();
    return uxTaskGetTaskNumber(task_handle);
}

} // namespace logging

LOG_COMPONENT_REF(Network);
LOG_COMPONENT_REF(USBHost);

extern "C" void lwip_platform_log_error(const char *message) {
    log_error(Network, "%s", message);
}
extern "C" void lwip_platform_log_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _log_event_valist(logging::Severity::info, &LOG_COMPONENT(Network), fmt, &args);
    va_end(args);
}

extern "C" void USBH_UsrLog(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _log_event_valist(logging::Severity::info, &LOG_COMPONENT(USBHost), fmt, &args);
    va_end(args);
}

extern "C" void USBH_ErrLog(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _log_event_valist(logging::Severity::error, &LOG_COMPONENT(USBHost), fmt, &args);
    va_end(args);
}

extern "C" void USBH_DbgLog(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _log_event_valist(logging::Severity::debug, &LOG_COMPONENT(USBHost), fmt, &args);
    va_end(args);
}

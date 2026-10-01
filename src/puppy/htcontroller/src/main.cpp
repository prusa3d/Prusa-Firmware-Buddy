#include "hal.hpp"
#include "logging.hpp"

#include <cstddef>
#include <cstdlib>

#include <FreeRTOS.h>
#include <task.h>
#include <freertos/timing.hpp>
#include <bsod/bsod.h>
#include <logging/log.hpp>

LOG_COMPONENT_DEF(HTController, logging::Severity::info);

// This magical incantation is required for fw_descriptor integration in cmake to work.
[[maybe_unused]] __attribute__((section(".fw_descriptor"), used)) const std::byte fw_descriptor[48] {};

namespace {

void main_task_code(void *) {
    while (true) {
        freertos::delay(1000);
        log_info(HTController, "1s tick alive");
    }
}

constexpr size_t main_task_stack_size = 512;
alignas(32) StackType_t main_task_stack[main_task_stack_size];
StaticTask_t main_task_control_block;

} // namespace

extern "C" int main() {
    hal::init();
    htcontroller::logging_init();
    log_info(HTController, "Starting");

    [[maybe_unused]] TaskHandle_t main_task_handle = xTaskCreateStatic(
        main_task_code,
        "main_task",
        main_task_stack_size,
        nullptr,
        tskIDLE_PRIORITY + 1,
        main_task_stack,
        &main_task_control_block);

    // Start FreeRTOS scheduler and we are done.
    vTaskStartScheduler();
}

extern "C" void vApplicationStackOverflowHook([[maybe_unused]] TaskHandle_t xTask, [[maybe_unused]] char *pcTaskName) {
    std::abort();
}

[[noreturn]] [[gnu::format(printf, 1, 4)]] void _bsod([[maybe_unused]] const char *fmt, [[maybe_unused]] const char *file_name, [[maybe_unused]] int line_number, ...) {
    hal::panic();
}

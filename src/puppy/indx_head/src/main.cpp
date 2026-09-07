#pragma GCC poison printf
#include "bootloader_update.hpp"
#include "modbus.hpp"
#include "hal.hpp"
#include "rtt.hpp"
#include "app.hpp"
#include "spi_task.hpp"

#include <FreeRTOS.h>
#include <freertos/timing.hpp>
#include <task.h>

#include <cstddef>
#include <bsod/bsod.h>

// This magical incantation is required for fw_descriptor integration in cmake to work.
[[maybe_unused]] __attribute__((section(".fw_descriptor"), used)) const std::byte fw_descriptor[48] {};

namespace {
// Modbus task
constexpr const size_t modbus_task_stack_size = 1024 / sizeof(StackType_t);
alignas(32) StackType_t modbus_task_stack[modbus_task_stack_size];
StaticTask_t modbus_task_control_block;

// App task
constexpr const size_t app_task_stack_size = 1024 / sizeof(StackType_t);
alignas(32) StackType_t app_task_stack[app_task_stack_size];
StaticTask_t app_task_control_block;

// SPI task
constexpr const size_t spi_task_stack_size = 1024 / sizeof(StackType_t);
alignas(32) StackType_t spi_task_stack[spi_task_stack_size];
StaticTask_t spi_task_control_block;

// Lazy task
constexpr const size_t lazy_task_stack_size = 1024 / sizeof(StackType_t);
alignas(32) StackType_t lazy_task_stack[lazy_task_stack_size];
StaticTask_t lazy_task_control_block;
} // namespace

extern "C" int main() {
    rtt::init();
    rtt::print("indx_head started\n");

    bootloader_update::run();

    hal::init();

    {
        [[maybe_unused]] TaskHandle_t modbus_task_handle = xTaskCreateStatic(
            [](void *) { modbus::run(); },
            "modbus_task",
            modbus_task_stack_size,
            nullptr,
            tskIDLE_PRIORITY + 2,
            modbus_task_stack,
            &modbus_task_control_block);
    }
    {
        [[maybe_unused]] TaskHandle_t app_task_handle = xTaskCreateStatic(
            [](void *) { app::run(); },
            "app_task",
            app_task_stack_size,
            nullptr,
            tskIDLE_PRIORITY + 1,
            app_task_stack,
            &app_task_control_block);
    }
    {
        [[maybe_unused]] TaskHandle_t spi_task_handle = xTaskCreateStatic(
            [](void *) { spi_task::run(); },
            "spi_task",
            spi_task_stack_size,
            nullptr,
            tskIDLE_PRIORITY + 2, // This tasks processes SPI IRQs - needs higher pripority then app task, but should mostly be idle
            spi_task_stack,
            &spi_task_control_block);
    }
    {
        [[maybe_unused]] TaskHandle_t lazy_task_handle = xTaskCreateStatic(
            [](void *) {
                for (;;) {
                    hal::i2c::trigger_delayed_request();
                    freertos::delay(1);
                }
            },
            "lazy_task",
            lazy_task_stack_size,
            nullptr,
            tskIDLE_PRIORITY, // This task processes some low prio request whenever the system is free to do so
            lazy_task_stack,
            &lazy_task_control_block);
    }

    // Start FreeRTOS scheduler and we are done.
    vTaskStartScheduler();
}

extern "C" void vApplicationStackOverflowHook([[maybe_unused]] TaskHandle_t xTask, [[maybe_unused]] char *pcTaskName) {
    hal::panic(indx_head::errors::FaultStatusMask::stack_overflow);
}

extern "C"
    [[noreturn, gnu::format(__printf__, 1, 4)]] void
    _bsod([[maybe_unused]] const char *fmt, [[maybe_unused]] const char *file_name, [[maybe_unused]] int line_number, ...) {
    hal::panic(indx_head::errors::FaultStatusMask::assert_failed);
}

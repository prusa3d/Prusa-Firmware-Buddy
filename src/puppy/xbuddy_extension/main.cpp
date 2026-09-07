#include "app.hpp"
#include "cyphal_transport.hpp"
#include "cyphal_application.hpp"
#include "FreeRTOS.h"
#include <bsod/bsod.h>
#include <cmsis_gcc.h>
#include "hal.hpp"
#include "hal_memory.hpp"
#include "task.h"
#include <freertos/timing.hpp>

// This magical incantation is required for fw_descriptor integration in cmake to work.
[[maybe_unused]] __attribute__((section(".fw_descriptor"), used)) const std::byte fw_descriptor[48] {};

// The logic is inverted here. We explicitly mark data we don't want to be
// shared and make all the other are shared between tasks.
#define NON_SHARED_DATA __attribute__((section("non_shared_data")))

extern uint32_t __shared_data_start__[];
extern uint32_t __shared_data_end__[];

extern char _isr_stack_start[];
extern char _isr_stack_end[];

constexpr const size_t main_task_stack_size = 512 + 256;
alignas(32) NON_SHARED_DATA StackType_t main_task_stack[main_task_stack_size];
NON_SHARED_DATA StaticTask_t main_task_control_block;
static void main_task_code(void *) {
    app::run();
}

constexpr const size_t hal_task_stack_size = 200;
alignas(32) NON_SHARED_DATA static StackType_t hal_task_stack[hal_task_stack_size];
NON_SHARED_DATA static StaticTask_t hal_task_control_block;
static void hal_task_code(void *) {
    hal::setup();
    for (;;) {
        hal::step();
        freertos::delay(1);
    }
}

constexpr const size_t cyphal_task_stack_size = 512;
alignas(32) NON_SHARED_DATA static StackType_t cyphal_task_stack[cyphal_task_stack_size];
NON_SHARED_DATA static StaticTask_t cyphal_task_control_block;
static void cyphal_task_code(void *) {
    cyphal::transport().run();
}

extern "C" int main() {
    // Set up ISR stack overflow checking
    __set_MSPLIM(reinterpret_cast<uint32_t>(&_isr_stack_start));

    // This has to be called before we yield control to FreeRTOS scheduler
    // because MPU would prevent us from accessing some important registers.
    hal::init();

    // Global objects constructing mutexes have to be created before we yield
    // control to FreeRTOS scheduler because MPU restricts calling them from
    // unprivileged task context.
    (void)cyphal::transport();

    // Tasks have to be created before we yield control to FreeRTOS scheduler
    // because MPU would prevent them from accessing their control blocks.
    {
        const MemoryRegion_t mpu_regions[] {
            {
                // Peripherals - controllable from all threads
                .pvBaseAddress = hal::memory::peripheral_address_region.data(),
                .ulLengthInBytes = hal::memory::peripheral_address_region.size(),
                .ulParameters = tskMPU_REGION_READ_WRITE | tskMPU_REGION_EXECUTE_NEVER | tskMPU_REGION_DEVICE_MEMORY,
            },
            {
                .pvBaseAddress = __shared_data_start__,
                .ulLengthInBytes = uint32_t((char *)__shared_data_end__ - (char *)__shared_data_start__),
                .ulParameters = tskMPU_REGION_READ_WRITE | tskMPU_REGION_EXECUTE_NEVER | tskMPU_REGION_NORMAL_MEMORY,
            },
            {},
        };

        {
            TaskHandle_t main_task_handle = xTaskCreateStatic(
                main_task_code,
                "main_task",
                main_task_stack_size,
                NULL,
                tskIDLE_PRIORITY + 1,
                main_task_stack,
                &main_task_control_block);
            vTaskAllocateMPURegions(main_task_handle, mpu_regions);
        }

        {
            TaskHandle_t hal_task_handle = xTaskCreateStatic(
                hal_task_code,
                "hal_task",
                hal_task_stack_size,
                NULL,
                tskIDLE_PRIORITY + 1,
                hal_task_stack,
                &hal_task_control_block);
            vTaskAllocateMPURegions(hal_task_handle, mpu_regions);
        }

        {
            TaskHandle_t cyphal_task_handle = xTaskCreateStatic(
                cyphal_task_code,
                "cyphal_task",
                cyphal_task_stack_size,
                NULL,
                tskIDLE_PRIORITY + 1,
                cyphal_task_stack,
                &cyphal_task_control_block);
            vTaskAllocateMPURegions(cyphal_task_handle, mpu_regions);
        }
    }

    // Start FreeRTOS scheduler and we are done.
    vTaskStartScheduler();
}

[[noreturn]] void __attribute__((noreturn, format(__printf__, 1, 4)))
_bsod(const char *fmt, const char *file_name, int line_number, ...) {
    (void)fmt, (void)file_name, (void)line_number;
    hal::panic();
}

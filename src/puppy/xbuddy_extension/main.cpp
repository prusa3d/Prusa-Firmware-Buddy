#include "app.hpp"
#include "hal.hpp"
#include "FreeRTOS.h"
#include "task.h"
#include <utility>
#include <freertos/timing.hpp>

// The logic is inverted here. We explicitly mark data we don't want to be
// shared and make all the other are shared between tasks.
#define NON_SHARED_DATA __attribute__((section("non_shared_data")))

extern uint32_t __shared_data_start__[];
extern uint32_t __shared_data_end__[];

MemoryRegion_t regions[3] {
    {
        .pvBaseAddress = (void *)0x40000000,
        .ulLengthInBytes = 0x10000000,
        .ulParameters = tskMPU_REGION_READ_WRITE | tskMPU_REGION_EXECUTE_NEVER | tskMPU_REGION_DEVICE_MEMORY,
    },
    {
        .pvBaseAddress = __shared_data_start__,
        .ulLengthInBytes = uint32_t((char *)__shared_data_end__ - (char *)__shared_data_start__),
        .ulParameters = tskMPU_REGION_READ_WRITE | tskMPU_REGION_EXECUTE_NEVER | tskMPU_REGION_NORMAL_MEMORY,
    },
    {},
};

constexpr const size_t main_task_stack_size = 200;
alignas(32) NON_SHARED_DATA StackType_t main_task_stack[main_task_stack_size];
NON_SHARED_DATA StaticTask_t main_task_control_block;
static void main_task_code(void *) {
    app::run();
}

constexpr const size_t hal_task_stack_size = 100;
alignas(32) NON_SHARED_DATA static StackType_t hal_task_stack[hal_task_stack_size];
NON_SHARED_DATA static StaticTask_t hal_task_control_block;
static void hal_task_code(void *) {
    freertos::delay(200); // is it needed?
    hal::mmu::nreset_pin_set(true);
    for (;;) {
        hal::step();
        freertos::delay(1);
    }
}

extern "C" int main() {
    // This has to be called before we yield control to FreeRTOS scheduler
    // because MPU would prevent us from accessing some important registers.
    hal::init();

    // Tasks have to be created before we yield control to FreeRTOS scheduler
    // because MPU would prevent them from accessing their control blocks.
    {
        TaskHandle_t main_task_handle = xTaskCreateStatic(
            main_task_code,
            "main_task",
            main_task_stack_size,
            NULL,
            tskIDLE_PRIORITY + 1,
            main_task_stack,
            &main_task_control_block);
        vTaskAllocateMPURegions(main_task_handle, regions);
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
        vTaskAllocateMPURegions(hal_task_handle, regions);
    }

    // Start FreeRTOS scheduler and we are done.
    vTaskStartScheduler();
}

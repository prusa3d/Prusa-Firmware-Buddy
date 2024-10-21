#include <stm32h5xx_hal.h>
#include <utility>

extern "C" int main() {
    HAL_Init();

    for (;;)
        ;
    std::unreachable();
}

// Catch those to improve debugging experience
// (which is nearly nonexistent at this point).
#define ISR_HANDLER(name)    \
    extern "C" void name() { \
        for (;;)             \
            ;                \
    }
ISR_HANDLER(NMI_Handler)
ISR_HANDLER(HardFault_Handler)
ISR_HANDLER(MemManage_Handler)
ISR_HANDLER(BusFault_Handler)
ISR_HANDLER(UsageFault_Handler)
ISR_HANDLER(DebugMon_Handler)
ISR_HANDLER(SVC_Handler)
ISR_HANDLER(PendSV_Handler)
ISR_HANDLER(SysTick_Handler)
#undef ISR_HANDLER

///@file
#include "hal.hpp"

#include <stm32h5xx_hal.h>

void hal::init() {
    HAL_Init();
}

void hal::panic() {
    asm volatile("bkpt 0");
    for (;;)
        ;
}

extern "C" void hal_panic() {
    hal::panic();
}

#define ISR_HANDLER(name)    \
    extern "C" void name() { \
        hal::panic();        \
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

//wiring_time.c

#include "Arduino.h"
#include "cmsis_os.h"

uint32_t GetCurrentMicro(void) {
    // copy-paste from original arduino_stm32/clock.c
    /* Ensure COUNTFLAG is reset by reading SysTick control and status register */
    /*((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == (SysTick_CTRL_COUNTFLAG_Msk));
	uint32_t m = HAL_GetTick();
	uint32_t u = SysTick->LOAD - SysTick->VAL;
	if (((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == (SysTick_CTRL_COUNTFLAG_Msk)))
	{
		m = HAL_GetTick();
		u = SysTick->LOAD - SysTick->VAL;
	}
	return (m * 1000 + (u * 1000) / SysTick->LOAD);*/
    // new impl for RTOS with system timer6
    int irq = __get_PRIMASK() & 1;
    if (irq)
        __disable_irq();
    uint32_t u = TIM6->CNT;
    uint32_t m = HAL_GetTick();
    if (irq)
        __enable_irq();
    return (m * 1000 + u);
}

uint32_t millis(void) {
    //return osKernelSysTick();
    return HAL_GetTick();
}

uint32_t micros(void) {
    return GetCurrentMicro();
}

static inline int is_interrupt(void) {
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
}

void delay(uint32_t ms) {
    if (!is_interrupt())
        osDelay(ms);
    //HAL_Delay(ms);
}

void delayMicroseconds(uint32_t usec) {
    if (!is_interrupt()) {
        uint32_t start = GetCurrentMicro();
        while ((start + usec) > GetCurrentMicro())
            ;
    }
}

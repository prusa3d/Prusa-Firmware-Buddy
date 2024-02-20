// wiring_time.c

#include "Arduino.h"
#include "cmsis_os.h"
#include "../../../../src/common/timing.h"

uint32_t millis(void) {
    return ticks_ms();
}

uint32_t micros(void) {
    return ticks_us();
}

static inline int is_interrupt(void) {
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
}

void delay(uint32_t ms) {
    if (!is_interrupt()) {
        osDelay(ms);
    } else {
        abort(); // TODO: add support for delay from IRQ
    }
}

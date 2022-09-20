// bsod_nogui.cpp - blue screen of death
#include "bsod.h"
#include "safe_state.h"
#include "FreeRTOS.h"

void _bsod(const char *fmt, const char *file_name, int line_number, ...) {
    hwio_safe_state();

    // busy wait for wdt
    while (1) {
    }
}

void fatal_error(const char *error, const char *module) {
    bsod(error);
}

void ScreenHardFault(void) {
    bsod("hard fault");
}

#ifdef configCHECK_FOR_STACK_OVERFLOW
    #include "task.h"

extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName) {
    bsod("stack overflow");
}
#endif

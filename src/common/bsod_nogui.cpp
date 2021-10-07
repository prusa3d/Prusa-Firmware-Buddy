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

void general_error(const char *error, const char *module) {
    bsod(error);
}

void temp_error(const char *error, const char *module, float t_noz, float tt_noz, float t_bed, float tt_bed) {
    bsod(error);
}

void temp_error_code(const uint16_t error_code) {
    bsod("temp error");
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

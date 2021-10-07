// bsod.c - blue screen of death
#include "bsod.h"
#include "safe_state.h"

#include "FreeRTOS.h"
#include "task.h"

void _bsod(const char *fmt, const char *file_name, int line_number, ...) {
    hwio_safe_state();

    // busy wait for wdr
    while (1) {
    }
}

void general_error(const char *error, const char *module) {
    bsod(error);
}

void temp_error(const char *error, const char *module, float t_noz, float tt_noz, float t_bed, float tt_bed) {
    bsod(error);
}

void ScreenHardFault(void) {
    bsod("hard fault");
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName) {
    bsod("stack overflow");
}

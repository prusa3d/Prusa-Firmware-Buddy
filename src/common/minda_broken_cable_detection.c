#include "minda_broken_cable_detection.h"
#include "Z_probe.h"
#include "hwio.h"
#include "hwio_a3ides.h"
#include "bsod.h"
#include "FreeRTOS.h"
#include "task.h" //taskENTER_CRITICAL

#ifndef _DEBUG
    #include "stm32f4xx_hal.h"   //HAL_IWDG_Refresh
extern IWDG_HandleTypeDef hiwdg; //watchdog handle
#endif                           //_DEBUG
static uint32_t PRE_XYHOME = 0;
static uint32_t POST_XYHOME = 0;

typedef struct endstop_struct_t {
    union {
        uint8_t i; //to access all at once
        struct {
            uint8_t PRE_XYHOME : 1;
            uint8_t POST_XYHOME : 1;
            uint8_t POST_ZHOME_0 : 1;
            uint8_t POST_ZHOME_1 : 1;
        };
    };

} endstop_struct;

static endstop_struct endstop_status;
void MINDA_BROKEN_CABLE_DETECTION__BEGIN() {
    PRE_XYHOME = 0;
    POST_XYHOME = 0;
    endstop_status.i = 0;
}
void MINDA_BROKEN_CABLE_DETECTION__PRE_XYHOME() {
    endstop_status.PRE_XYHOME = hwio_di_get_val(_DI_Z_MIN);
    PRE_XYHOME = get_Z_probe_endstop_hits();
}
void MINDA_BROKEN_CABLE_DETECTION__POST_XYHOME() {
    endstop_status.POST_XYHOME = hwio_di_get_val(_DI_Z_MIN);
    POST_XYHOME = get_Z_probe_endstop_hits();
}
void MINDA_BROKEN_CABLE_DETECTION__POST_ZHOME_0() {
    endstop_status.POST_ZHOME_0 = hwio_di_get_val(_DI_Z_MIN);
}
void MINDA_BROKEN_CABLE_DETECTION__POST_ZHOME_1() {
    endstop_status.POST_ZHOME_1 = hwio_di_get_val(_DI_Z_MIN);
}
void MINDA_BROKEN_CABLE_DETECTION__END() {

    if (PRE_XYHOME != POST_XYHOME || endstop_status.i) {

        taskENTER_CRITICAL(); //never exit CRITICAL, wanted to use __disable_irq, but it does not work. i do not know why
#ifndef _DEBUG
        HAL_IWDG_Refresh(&hiwdg);                                   //watchdog reset
#endif                                                              //_DEBUG
        general_error("HOMING ERROR", "Please check minda\ncable"); //2 spaces before cable
    }
}

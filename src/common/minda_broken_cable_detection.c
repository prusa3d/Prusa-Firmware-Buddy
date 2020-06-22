#include "minda_broken_cable_detection.h"
#include "Z_probe.h"
#include "bsod.h"
#include "FreeRTOS.h"
#include "task.h"   //taskENTER_CRITICAL
#include <string.h> //memset
#include "MindaRedscreen.h"
#include "wdt.h"
#include "gpio.h"
#include "hwio_pindef.h"

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

static endstop_struct endstop_status = {};
void MINDA_BROKEN_CABLE_DETECTION__BEGIN() {
    PRE_XYHOME = 0;
    POST_XYHOME = 0;
    endstop_status.i = 0;
}
void MINDA_BROKEN_CABLE_DETECTION__PRE_XYHOME() {
    endstop_status.PRE_XYHOME = gpio_get(PIN_Z_MIN);
    PRE_XYHOME = get_Z_probe_endstop_hits();
}
void MINDA_BROKEN_CABLE_DETECTION__POST_XYHOME() {
    endstop_status.POST_XYHOME = gpio_get(PIN_Z_MIN);
    POST_XYHOME = get_Z_probe_endstop_hits();
}
void MINDA_BROKEN_CABLE_DETECTION__POST_ZHOME_0() {
    endstop_status.POST_ZHOME_0 = gpio_get(PIN_Z_MIN);
}
void MINDA_BROKEN_CABLE_DETECTION__POST_ZHOME_1() {
    endstop_status.POST_ZHOME_1 = gpio_get(PIN_Z_MIN);
}
void MINDA_BROKEN_CABLE_DETECTION__END() {

    if (PRE_XYHOME != POST_XYHOME || endstop_status.i) {

        taskENTER_CRITICAL(); //never exit CRITICAL, wanted to use __disable_irq, but it does not work. i do not know why
        wdt_iwdg_refresh();
        general_error("HOMING ERROR", "Please check minda\ncable");
    }
}

typedef struct pre_post_t {
    int pre : 31;
    int pre_lvl : 1;
    int post : 31;
    int post_lvl : 1;
} pre_post;

#define POINTS 16
static pre_post mbl_preposts[POINTS] = {};
static size_t actual_point = 0;
void MINDA_BROKEN_CABLE_DETECTION__MBL_BEGIN() {
    memset(mbl_preposts, 0, sizeof(mbl_preposts));
    actual_point = 0;
}
void MINDA_BROKEN_CABLE_DETECTION__PRE_XYMOVE() {
    mbl_preposts[actual_point].pre_lvl = gpio_get(PIN_Z_MIN);
    mbl_preposts[actual_point].pre = get_Z_probe_endstop_hits();
}
void MINDA_BROKEN_CABLE_DETECTION__POST_XYMOVE() {
    mbl_preposts[actual_point].post_lvl = gpio_get(PIN_Z_MIN);
    mbl_preposts[actual_point].post = get_Z_probe_endstop_hits();
    actual_point = (actual_point + 1) % POINTS;
}
void MINDA_BROKEN_CABLE_DETECTION__MBL_END() {
    uint16_t moves = 0;
    uint16_t points = 0;

    for (actual_point = 0; actual_point < (POINTS - 1); ++actual_point) {
        if (mbl_preposts[actual_point + 1].post != mbl_preposts[actual_point + 1].pre)
            moves |= 1 << actual_point;
        if (mbl_preposts[actual_point].pre_lvl || mbl_preposts[actual_point].post_lvl)
            points |= 1 << actual_point;
    }

    //last point was not set actual_point contains valid value
    if (mbl_preposts[actual_point].pre_lvl || mbl_preposts[actual_point].post_lvl)
        points |= 1 << actual_point;

    if (moves || points) {
        //error moves are not zero
        taskENTER_CRITICAL(); //never exit CRITICAL, wanted to use __disable_irq, but it does not work. i do not know why
        wdt_iwdg_refresh();
        mbl_error(moves, points);
    }
}

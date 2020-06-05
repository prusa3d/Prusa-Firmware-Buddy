// trinamic.cpp
#include "dbg.h"
#include "config.h"
#include "hwio_a3ides.h"
#include "TMCStepper.h"
#include "gpio.h"
#include "hwio_pindef.h"
#include "../Marlin/src/module/stepper.h"

#define DBG _dbg3 //debug level 3
//#define DBG(...)  //disable debug

#if ((MOTHERBOARD == 1823))

extern "C" {

TMC2209Stepper *pStep[4] = { nullptr, nullptr, nullptr, nullptr };

uint16_t tmc_step = 0;
uint8_t tmc_stepper = -1;

uint16_t tmc_sg[4];      // stallguard result for each axis
uint8_t tmc_sg_mask = 7; // stalguard result sampling mask (bit0-x, bit1-y, ...), xyz by default
uint8_t tmc_sg_axis = 0; // current axis for stalguard result sampling (0-x, 1-y, ...)

void tmc_delay(uint16_t time) // delay for switching tmc step pin level
{
    volatile uint16_t tmc_delay;
    for (tmc_delay = 0; tmc_delay < time; time--) {
    }
}

void init_tmc(void) {

    //pointers to TMCStepper instances
    pStep[X_AXIS] = &stepperX;
    pStep[Y_AXIS] = &stepperY;
    pStep[Z_AXIS] = &stepperZ;
    pStep[E_AXIS] = &stepperE0;
    //set TCOOLTHRS
    pStep[X_AXIS]->TCOOLTHRS(400);
    pStep[Y_AXIS]->TCOOLTHRS(400);
    pStep[Z_AXIS]->TCOOLTHRS(400);
    pStep[E_AXIS]->TCOOLTHRS(400);
    //set SGTHRS
    pStep[X_AXIS]->SGTHRS(140);
    pStep[Y_AXIS]->SGTHRS(130);
    pStep[Z_AXIS]->SGTHRS(100);
    pStep[E_AXIS]->SGTHRS(100);
}

// this function performs stallguard sample for single axis
// return value contain bitmask of sampled axis, in case of nonzero return value this call take 5ms
//  Now we are using stepper.axis_is_moving and value is set to zero for stopped motor,
//  right way is reading tstep and comparing it to TCOOLTHRS, but it takes time (read register).
//  Using stepper.axis_is_moving is simple but in some cases we get bad samples (tstep > TCOOLTHRS).
//  Maybe we can improve this by calculating tstep from stepper variables.
uint8_t tmc_sample(void) {
    uint8_t mask = 0;
    if (tmc_sg_mask) {
        while ((tmc_sg_mask & (1 << tmc_sg_axis)) == 0)
            tmc_sg_axis = (tmc_sg_axis + 1) & 0x03;
        if (stepper.axis_is_moving((AxisEnum)tmc_sg_axis)) {
            mask = (1 << tmc_sg_axis);
            tmc_sg[tmc_sg_axis] = pStep[tmc_sg_axis]->SG_RESULT();
        } else
            tmc_sg[tmc_sg_axis] = 0;
        tmc_sg_axis = (tmc_sg_axis + 1) & 0x03;
    }
    return mask;
}

void tmc_set_sgthrs(uint16_t SGT) {
    pStep[0]->SGTHRS(SGT);
    pStep[1]->SGTHRS(SGT);
    pStep[2]->SGTHRS(SGT);
    pStep[3]->SGTHRS(SGT);
}
void tmc_set_mres() {
    pStep[0]->mres(4);
    pStep[1]->mres(4);
    pStep[2]->mres(4);
    pStep[3]->mres(4);
}
uint8_t tmc_get_diag() //0 = X, 2 = Y, 4 = Z, 8 = E
{
    unsigned int diag = 0;
    uint16_t tmp_step;
    uint16_t step = 3500;
    uint8_t step_mask = 15;

    uint32_t gconf = pStep[3]->GCONF();
    gconf &= ~4;
    pStep[3]->GCONF(gconf);
    diag = 0;

    for (tmp_step = 0; tmp_step < step; step--) {
        tmc_delay(1024 * 2);
        if (step_mask & 1)
            gpio_set(PIN_X_STEP, 0);
        if (step_mask & 2)
            gpio_set(PIN_Y_STEP, 0);
        if (step_mask & 4)
            gpio_set(PIN_Z_STEP, 0);
        if (step_mask & 8)
            gpio_set(PIN_E_STEP, 0);
        gpio_set(PIN_X_STEP, 1);
        gpio_set(PIN_Y_STEP, 1);
        gpio_set(PIN_Z_STEP, 1);
        gpio_set(PIN_E_STEP, 1);
        diag |= gpio_get(PIN_E_DIAG) << 3;
        diag |= gpio_get(PIN_X_DIAG);
        diag |= gpio_get(PIN_Y_DIAG) << 1;
        diag |= gpio_get(PIN_Z_DIAG) << 2;

        if (diag == 15)
            break;
    }
    return diag;
}

void tmc_move(uint8_t step_mask, uint16_t step, uint8_t speed) {
    uint16_t tmp_step;
    for (tmp_step = 0; tmp_step < step; step--) {
        if (step_mask & 1)
            gpio_set(PIN_X_STEP, 1);
        if (step_mask & 2)
            gpio_set(PIN_Y_STEP, 1);
        if (step_mask & 4)
            gpio_set(PIN_Z_STEP, 1);
        if (step_mask & 8)
            gpio_set(PIN_E_STEP, 1);
        tmc_delay(1024 * speed);
        gpio_set(PIN_X_STEP, 0);
        gpio_set(PIN_Y_STEP, 0);
        gpio_set(PIN_Z_STEP, 0);
        gpio_set(PIN_E_STEP, 0);
    }
}

void tmc_set_move(uint8_t tmc, uint16_t step, uint8_t dir, uint8_t speed) {
    gpio_set(PIN_X_DIR, dir);
    gpio_set(PIN_Y_DIR, dir);
    gpio_set(PIN_Z_DIR, dir);
    gpio_set(PIN_E_DIR, dir);
    tmc_move(tmc, step, speed);
}

} //extern "C"

#else

    #error "MOTHERBOARD not defined"

#endif

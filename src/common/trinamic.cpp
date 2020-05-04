// trinamic.cpp
#include "dbg.h"
#include "config.h"
#include "hwio_a3ides.h"
#include "TMCStepper.h"
#include "gpio.h"
#include "hwio_pindef.h"

#define DBG _dbg3 //debug level 3
//#define DBG(...)  //disable debug

#if ((MOTHERBOARD == 1823))

extern TMC2209Stepper stepperX;
extern TMC2209Stepper stepperY;
extern TMC2209Stepper stepperZ;
extern TMC2209Stepper stepperE0;

extern "C" {

TMC2209Stepper *pStepX = 0;
TMC2209Stepper *pStepY = 0;
TMC2209Stepper *pStepZ = 0;
TMC2209Stepper *pStepE = 0;

uint16_t tmc_step = 0;
uint8_t tmc_stepper = -1;

void tmc_delay(uint16_t time) // delay for switching tmc step pin level
{
    volatile uint16_t tmc_delay;
    for (tmc_delay = 0; tmc_delay < time; time--) {
    }
}

void init_tmc(void) {

    pStepX = &stepperX;
    pStepY = &stepperY;
    pStepZ = &stepperZ;
    pStepE = &stepperE0;
    //int i = 0;
    pStepX->TCOOLTHRS(400);
    pStepY->TCOOLTHRS(400);
    pStepZ->TCOOLTHRS(400);
    pStepE->TCOOLTHRS(400); //400
    pStepX->SGTHRS(140);
    pStepY->SGTHRS(130);
    pStepZ->SGTHRS(100);
    pStepE->SGTHRS(100);
}

void tmc_sample(void) {
    unsigned int sgx = pStepX->SG_RESULT();
    //unsigned int sgy = pStepY->SG_RESULT();
    unsigned int diag = 0;
    //	diag |= hwio_di_get_val(_DI_Z_DIAG);
    //diag |= hwio_di_get_val(_DI_Y_DIAG) << 1;
    //diag |= hwio_di_get_val(_DI_Z_DIAG) << 2;
    //diag |= hwio_di_get_val(_DI_E0_DIAG) << 3;
    unsigned int tstepx = pStepX->TSTEP();
    //unsigned int tstepy = pStepX->TSTEP();
    //	int sgz = pStepZ->SG_RESULT();
    //	int sge = pStepE->SG_RESULT();
    //	_dbg("sg %d %d %d %d", sgx, sgy, sgz, sge);
    sgx = sgx;       //prevent warning
    tstepx = tstepx; //prevent warning
    diag = diag;     //prevent warning
    DBG("sg %u %u %u", sgx, diag, tstepx);
}

void tmc_set_sgthrs(uint16_t SGT) {
    pStepX->SGTHRS(SGT);
    pStepY->SGTHRS(SGT);
    pStepZ->SGTHRS(SGT);
    pStepE->SGTHRS(SGT);
}
void tmc_set_mres() {
    pStepX->mres(4);
    pStepY->mres(4);
    pStepZ->mres(4);
    pStepE->mres(4);
}
uint8_t tmc_get_diag() //0 = X, 2 = Y, 4 = Z, 8 = E
{
    unsigned int diag = 0;
    uint16_t tmp_step;
    uint16_t step = 3500;
    uint8_t step_mask = 15;

    uint32_t gconf = pStepE->GCONF();
    gconf &= ~4;
    pStepE->GCONF(gconf);
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

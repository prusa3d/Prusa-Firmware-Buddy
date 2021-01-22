// trinamic.cpp
#include "trinamic.h"
#include "dbg.h"
#include "config.h"
#include "TMCStepper.h"
#include "gpio.h"
#include "hwio_pindef.h"
#include "../Marlin/src/module/stepper.h"
#include "bsod.h"

#define DBG _dbg3 //debug level 3
//#define DBG(...)  //disable debug

#if ((MOTHERBOARD == 1823))

using namespace buddy::hw;
static TMC2209Stepper *pStep[4] = { nullptr, nullptr, nullptr, nullptr };

static uint16_t tmc_sg[4];      // stallguard result for each axis
static uint8_t tmc_sg_mask = 7; // stalguard result sampling mask (bit0-x, bit1-y, ...), xyz by default
static uint8_t tmc_sg_axis = 0; // current axis for stalguard result sampling (0-x, 1-y, ...)

static tmc_sg_sample_cb_t *tmc_sg_sample_cb = NULL; // sg sample callback

osMutexDef(tmc_mutex);
osMutexId(tmc_mutex_id);

extern "C" {

uint8_t tmc_get_sg_mask() { return tmc_sg_mask; }
uint8_t tmc_get_sg_axis() { return tmc_sg_axis; }
tmc_sg_sample_cb_t *tmc_get_sg_sample_cb() { return tmc_sg_sample_cb; }

void tmc_set_sg_mask(uint8_t mask) { tmc_sg_mask = mask; }
void tmc_set_sg_axis(uint8_t axis) { tmc_sg_axis = axis; }
void tmc_set_sg_sample_cb(tmc_sg_sample_cb_t *cb) { tmc_sg_sample_cb = cb; }

void tmc_delay(uint16_t time) // delay for switching tmc step pin level
{
    volatile uint16_t tmc_delay;
    for (tmc_delay = 0; tmc_delay < time; time--) {
    }
}

void init_tmc(void) {
    init_tmc_bare_minimum();
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
    pStep[X_AXIS]->SGTHRS(130);
    pStep[Y_AXIS]->SGTHRS(130);
    pStep[Z_AXIS]->SGTHRS(100);
    pStep[E_AXIS]->SGTHRS(100);
}

void init_tmc_bare_minimum(void) {
    tmc_mutex_id = osMutexCreate(osMutex(tmc_mutex));

    //pointers to TMCStepper instances
    pStep[X_AXIS] = &stepperX;
    pStep[Y_AXIS] = &stepperY;
    pStep[Z_AXIS] = &stepperZ;
    pStep[E_AXIS] = &stepperE0;

    pStep[X_AXIS]->SLAVECONF(0x300);
    pStep[Y_AXIS]->SLAVECONF(0x300);
    pStep[Z_AXIS]->SLAVECONF(0x300);
    pStep[E_AXIS]->SLAVECONF(0x300);
}

/// Acquire lock for mutual exclusive access to the trinamic's serial port
///
/// This implements a weak symbol declared within the TMCStepper library
bool tmc_serial_lock_acquire(void) {
    return osMutexWait(tmc_mutex_id, osWaitForever) == osOK;
}

/// Release lock for mutual exclusive access to the trinamic's serial port
///
/// This implements a weak symbol declared within the TMCStepper library
void tmc_serial_lock_release(void) {
    osMutexRelease(tmc_mutex_id);
}

/// Called when an error occurs when communicating with the TMC over serial
///
/// This implements a weak symbol declared within the TMCStepper library
void tmc_communication_error(void) {
    bsod("trinamic communication error");
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
        if (tmc_sg_sample_cb)
            tmc_sg_sample_cb(tmc_sg_axis, tmc_sg[tmc_sg_axis]);
        tmc_sg_axis = (tmc_sg_axis + 1) & 0x03;
    }
    return mask;
}

extern uint16_t tmc_get_last_sg_sample(uint8_t axis) {
    return tmc_sg[axis];
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
        if (step_mask & 1)
            xStep.write(Pin::State::low);
        if (step_mask & 2)
            yStep.write(Pin::State::low);
        if (step_mask & 4)
            zStep.write(Pin::State::low);
        if (step_mask & 8)
            e0Step.write(Pin::State::low);
        tmc_delay(10);
        xStep.write(Pin::State::high);
        yStep.write(Pin::State::high);
        zStep.write(Pin::State::high);
        e0Step.write(Pin::State::high);
        diag |= static_cast<unsigned int>(e0Diag.read()) << 3;
        diag |= static_cast<unsigned int>(xDiag.read());
        diag |= static_cast<unsigned int>(yDiag.read()) << 1;
        diag |= static_cast<unsigned int>(zDiag.read()) << 2;

        if (diag == 15)
            break;
    }
    return diag;
}

static void tmc_move(uint8_t step_mask, uint16_t step, uint8_t speed) {
    uint16_t tmp_step;
    for (tmp_step = 0; tmp_step < step; step--) {
        if (step_mask & 1)
            xStep.write(Pin::State::high);
        if (step_mask & 2)
            yStep.write(Pin::State::high);
        if (step_mask & 4)
            zStep.write(Pin::State::high);
        if (step_mask & 8)
            e0Step.write(Pin::State::high);
        tmc_delay(1024 * speed);
        xStep.write(Pin::State::low);
        yStep.write(Pin::State::low);
        zStep.write(Pin::State::low);
        e0Step.write(Pin::State::low);
    }
}

void tmc_set_move(uint8_t tmc, uint32_t step, uint8_t dir, uint8_t speed) {
    xDir.write(static_cast<Pin::State>(dir));
    yDir.write(static_cast<Pin::State>(dir));
    zDir.write(static_cast<Pin::State>(dir));
    e0Dir.write(static_cast<Pin::State>(dir));
    tmc_move(tmc, step, speed);
}

} //extern "C"

#else

    #error "MOTHERBOARD not defined"

#endif

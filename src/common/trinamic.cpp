// trinamic.cpp
#include "trinamic.h"
#include "config.h"
#include "TMCStepper.h"
#include "gpio.h"
#include "hwio_pindef.h"
#include "../Marlin/src/module/stepper.h"
#include "bsod.h"

#if ((MOTHERBOARD == 1823))

using namespace buddy::hw;
static TMC2209Stepper *pStep[4] = { nullptr, nullptr, nullptr, nullptr };

static uint16_t tmc_sg[4];      // stallguard result for each axis
static uint8_t tmc_sg_mask = 7; // stalguard result sampling mask (bit0-x, bit1-y, ...), xyz by default
static uint8_t tmc_sg_axis = 0; // current axis for stalguard result sampling (0-x, 1-y, ...)

static tmc_sg_sample_cb_t *tmc_sg_sample_cb = NULL; // sg sample callback

osMutexDef(tmc_mutex);
osMutexId tmc_mutex_id;

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

} //extern "C"

#else

    #error "MOTHERBOARD not defined"

#endif

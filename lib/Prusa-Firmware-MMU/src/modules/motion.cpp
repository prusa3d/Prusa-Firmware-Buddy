/// @file motion.cpp
#include "motion.h"
#include "../panic.h"
#include "../debug.h"
#include "permanent_storage.h"

// TODO: use proper timer abstraction
#ifdef __AVR__
#include <avr/interrupt.h>
#include <util/atomic.h>
#else
//#include "../hal/timers.h"
#endif

namespace modules {
namespace motion {

Motion motion;

/// ISR state manipulation
static inline void IsrSetEnabled(bool state) {
#ifdef __AVR__
    // NOTE: ATOMIC_BLOCK is split across branches to split the function into two optimal calls at
    // compile-time
    if (state)
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { TIMSK1 |= (1 << OCIE1A); }
    else
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { TIMSK1 &= ~(1 << OCIE1A); }
#endif
}

static inline bool IsrDisable() {
#ifdef __AVR__
    bool state;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        state = TIMSK1 & (1 << OCIE1A);
        TIMSK1 &= ~(1 << OCIE1A);
    }
    return state;
#else
    return false;
#endif
}

class SuspendIsr {
    bool enabled;

public:
    SuspendIsr() {
        enabled = IsrDisable();
    }

    ~SuspendIsr() {
        IsrSetEnabled(enabled);
    }
};

bool Motion::InitAxis(Axis axis) {
    return InitAxis(axis, axisData[axis].currents);
}

bool Motion::InitAxis(config::Axis axis, MotorCurrents mc) {
    // disable the axis and re-init the driver: this will clear the internal
    // StallGuard data as a result without special handling
    Disable(axis);
    // Init also applies the currently pre-set StallGuard threshold into the TMC driver
    return axisData[axis].drv.Init(axisParams[axis].params, mc, axisParams[axis].mode);
}

void Motion::SetEnabled(Axis axis, bool enabled) {
    if (enabled != axisData[axis].enabled) {
        axisData[axis].drv.SetEnabled(axisParams[axis].params, enabled);
        axisData[axis].enabled = enabled;
    } // else skip unnecessary Enable/Disable operations on an axis if already in the desired state
}

void Motion::SetMode(Axis axis, MotorMode mode) {
    axisData[axis].drv.SetMode(axisParams[axis].params, mode);
}

void Motion::SetMode(MotorMode mode) {
    for (uint8_t i = 0; i != NUM_AXIS; ++i)
        axisData[i].drv.SetMode(axisParams[i].params, mode);
}

bool Motion::StallGuard(Axis axis) {
    return axisData[axis].drv.Stalled();
}

void Motion::StallGuardReset(Axis axis) {
    axisData[axis].drv.ClearStallguard();
}

void Motion::PlanStallGuardThreshold(config::Axis axis, int8_t sg_thrs) {
    mm::axisParams[axis].params.sg_thrs = sg_thrs;
}

void Motion::PlanMoveTo(Axis axis, pos_t pos, steps_t feed_rate, steps_t end_rate) {
    dbg_logic_fP(PSTR("Move axis %d to %u"), axis, pos);

    if (axisData[axis].ctrl.PlanMoveTo(pos, feed_rate, end_rate)) {
        // move was queued, prepare the axis
        if (!axisData[axis].enabled)
            SetEnabled(axis, true);
    } else {
        // queue is full: queue mishandling! trigger a panic
        Panic(ErrorCode::QUEUE_FULL);
    }
}

pos_t Motion::Position(Axis axis) const {
    return axisData[axis].ctrl.Position();
}

pos_t Motion::CurPosition(Axis axis) const {
    auto guard = SuspendIsr();
    return axisData[axis].ctrl.CurPosition();
}

bool Motion::QueueEmpty() const {
    for (uint8_t i = 0; i != NUM_AXIS; ++i)
        if (!axisData[i].ctrl.QueueEmpty())
            return false;
    return true;
}

void Motion::AbortPlannedMoves(Axis axis, bool halt) {
    auto guard = SuspendIsr();
    axisData[axis].ctrl.AbortPlannedMoves(halt);
}

void Motion::AbortPlannedMoves(bool halt) {
    auto guard = SuspendIsr();
    for (uint8_t i = 0; i != NUM_AXIS; ++i)
        AbortPlannedMoves((Axis)i, halt);
}

static inline void Isr() {
#ifdef __AVR__
    st_timer_t next = motion.Step();
    // TODO: use proper timer abstraction
    if (next)
        OCR1A = next;
    else {
        // Idling: plan the next interrupt after 8ms from now.
        OCR1A = 0x4000;
    }
#endif
}

void Init() {
#ifdef __AVR__
    // TODO: use proper timer abstraction

    // waveform generation = 0100 = CTC
    TCCR1B &= ~(1 << WGM13);
    TCCR1B |= (1 << WGM12);
    TCCR1A &= ~(1 << WGM11);
    TCCR1A &= ~(1 << WGM10);

    // output mode = 00 (disconnected)
    TCCR1A &= ~(3 << COM1A0);
    TCCR1A &= ~(3 << COM1B0);

    // Set the timer pre-scaler
    // We use divider of 8, resulting in a 2MHz timer frequency on a 16MHz MCU
    TCCR1B = (TCCR1B & ~(0x07 << CS10)) | (2 << CS10);

    // Plan the first interrupt after 8ms from now.
    OCR1A = 0x4000;
    TCNT1 = 0;
#endif

    // Enable interrupt
    IsrSetEnabled(true);
}

} // namespace motion
} // namespace modules

#ifdef __AVR__
ISR(TIMER1_COMPA_vect) {
    modules::motion::Isr();
}
#endif

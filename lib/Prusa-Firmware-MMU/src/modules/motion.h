/// @file motion.h
#pragma once
#include "../pins.h"
#include "pulse_gen.h"
#include "axisunit.h"

namespace modules {

/// Logic of motor handling
/// Ideally enable stepping of motors under ISR (all timers have higher priority than serial)
namespace motion {

// Import axes definitions
using config::NUM_AXIS;

using namespace hal::tmc2130;
using pulse_gen::st_timer_t;

// Check for configuration invariants
static_assert(
    (1. / (F_CPU / config::stepTimerFrequencyDivider) * config::stepTimerQuantum)
        > (1. / config::maxStepFrequency),
    "stepTimerQuantum must be larger than the maximal stepping frequency interval");

/// Main axis enumeration
struct AxisParams {
    char name;
    MotorParams params;
    MotorCurrents currents;
    MotorMode mode;
    steps_t jerk;
    steps_t accel;
};

/// Return the default motor mode for an Axis
static constexpr MotorMode DefaultMotorMode(const config::AxisConfig &axis) {
    return axis.stealth ? MotorMode::Stealth : MotorMode::Normal;
}

/// Static axis configuration
static AxisParams axisParams[NUM_AXIS] = {
    // Pulley
    {
        .name = 'P',
        .params = { .spi = hal::spi::TmcSpiBus, .idx = Pulley, .dirOn = config::pulley.dirOn, .csPin = PULLEY_CS_PIN, .stepPin = PULLEY_STEP_PIN, .sgPin = PULLEY_SG_PIN, .mRes = config::pulley.mRes, .sg_thrs = config::pulley.sg_thrs, Axis::Pulley },
        .currents = MotorCurrents(config::pulley.iRun, config::pulley.iHold),
        .mode = DefaultMotorMode(config::pulley),
        .jerk = unitToSteps<P_speed_t>(config::pulleyLimits.jerk),
        .accel = unitToSteps<P_accel_t>(config::pulleyLimits.accel),
    },
    // Selector
    {
        .name = 'S',
        .params = { .spi = hal::spi::TmcSpiBus, .idx = Selector, .dirOn = config::selector.dirOn, .csPin = SELECTOR_CS_PIN, .stepPin = SELECTOR_STEP_PIN, .sgPin = SELECTOR_SG_PIN, .mRes = config::selector.mRes, .sg_thrs = config::selector.sg_thrs, Axis::Selector },
        .currents = MotorCurrents(config::selector.iRun, config::selector.iHold),
        .mode = DefaultMotorMode(config::selector),
        .jerk = unitToSteps<S_speed_t>(config::selectorLimits.jerk),
        .accel = unitToSteps<S_accel_t>(config::selectorLimits.accel),
    },
    // Idler
    {
        .name = 'I',
        .params = { .spi = hal::spi::TmcSpiBus, .idx = Idler, .dirOn = config::idler.dirOn, .csPin = IDLER_CS_PIN, .stepPin = IDLER_STEP_PIN, .sgPin = IDLER_SG_PIN, .mRes = config::idler.mRes, .sg_thrs = config::idler.sg_thrs, Axis::Idler },
        .currents = MotorCurrents(config::idler.iRun, config::idler.iHold),
        .mode = DefaultMotorMode(config::idler),
        .jerk = unitToSteps<I_speed_t>(config::idlerLimits.jerk),
        .accel = unitToSteps<I_accel_t>(config::idlerLimits.accel),
    },
};

class Motion {
public:
    inline constexpr Motion() = default;

    /// Init axis driver - @@TODO this should be probably hidden
    /// somewhere deeper ... something should manage the axes and their
    /// state especially when the TMC may get randomly reset (deinited)
    /// @returns true if the init was successful (TMC2130 responded ok)
    bool InitAxis(Axis axis);
    bool InitAxis(Axis axis, MotorCurrents mc);

    /// Return the axis power status.
    bool Enabled(Axis axis) const { return axisData[axis].enabled; }

    /// Set axis power status. One must manually ensure no moves are currently being
    /// performed by calling QueueEmpty().
    void SetEnabled(Axis axis, bool enabled);

    /// Disable axis motor. One must manually ensure no moves are currently being
    /// performed by calling QueueEmpty().
    void Disable(Axis axis) { SetEnabled(axis, false); }

    /// Set mode of TMC/motors operation. One must manually ensure no moves are currently
    /// being performed by calling QueueEmpty().
    void SetMode(Axis axis, MotorMode mode);

    /// Set the same mode of TMC/motors operation for all axes. @see SetMode
    void SetMode(MotorMode mode);

    /// @returns true if a StallGuard event occurred recently on the axis
    bool StallGuard(Axis axis);

    /// clear StallGuard flag reported on an axis
    void StallGuardReset(Axis axis);

    /// Sets (plans) StallGuard threshold for an axis (basically the higher number the lower sensitivity)
    /// The new SGTHRS value gets applied in Init(), it is _NOT_ written into the TMC immediately in this method.
    void PlanStallGuardThreshold(Axis axis, int8_t sg_thrs);

    /// Enqueue a single axis move in steps starting and ending at zero speed with maximum
    /// feedrate. Moves can only be enqueued if the axis is not Full().
    /// @param axis axis affected
    /// @param pos target position
    /// @param feed_rate maximum feedrate
    /// @param end_rate endding feedrate (may not be reached!)
    void PlanMoveTo(Axis axis, pos_t pos, steps_t feed_rate, steps_t end_rate = 0);

    /// Enqueue a single axis move using PlanMoveTo, but using AxisUnit. The Axis needs to
    /// be supplied as the first template argument: PlanMoveTo<axis>(pos, rate).
    /// @see PlanMoveTo, unitToSteps
    template <Axis A>
    constexpr void PlanMoveTo(AxisUnit<pos_t, A, Lenght> pos,
        AxisUnit<steps_t, A, Speed> feed_rate, AxisUnit<steps_t, A, Speed> end_rate = { 0 }) {
        PlanMoveTo(A, pos.v, feed_rate.v, end_rate.v);
    }

    /// Enqueue a single axis move using PlanMoveTo, but using physical units. The Axis
    /// needs to be supplied as the first template argument: PlanMoveTo<axis>(pos, rate).
    /// @see PlanMoveTo, unitToSteps
    template <Axis A, config::UnitBase B>
    constexpr void PlanMoveTo(config::Unit<long double, B, Lenght> pos,
        config::Unit<long double, B, Speed> feed_rate, config::Unit<long double, B, Speed> end_rate = { 0 }) {
        PlanMoveTo<A>(
            unitToAxisUnit<AxisUnit<pos_t, A, Lenght>>(pos),
            unitToAxisUnit<AxisUnit<steps_t, A, Speed>>(feed_rate),
            unitToAxisUnit<AxisUnit<steps_t, A, Speed>>(end_rate));
    }

    /// Enqueue a single axis move in steps starting and ending at zero speed with maximum
    /// feedrate. Moves can only be enqueued if the axis is not Full().
    /// @param axis axis affected
    /// @param delta relative to current position
    /// @param feed_rate maximum feedrate
    /// @param end_rate endding feedrate (may not be reached!)
    void PlanMove(Axis axis, pos_t delta, steps_t feed_rate, steps_t end_rate = 0) {
        PlanMoveTo(axis, Position(axis) + delta, feed_rate, end_rate);
    }

    /// Enqueue a single axis move using PlanMove, but using AxisUnit. The Axis needs to
    /// be supplied as the first template argument: PlanMove<axis>(pos, rate).
    /// @see PlanMove, unitToSteps
    template <Axis A>
    constexpr void PlanMove(AxisUnit<pos_t, A, Lenght> delta,
        AxisUnit<steps_t, A, Speed> feed_rate, AxisUnit<steps_t, A, Speed> end_rate = { 0 }) {
        PlanMove(A, delta.v, feed_rate.v, end_rate.v);
    }

    /// Enqueue a single axis move using PlanMove, but using physical units. The Axis needs to
    /// be supplied as the first template argument: PlanMove<axis>(pos, rate).
    /// @see PlanMove, unitToSteps
    template <Axis A, config::UnitBase B>
    constexpr void PlanMove(config::Unit<long double, B, Lenght> delta,
        config::Unit<long double, B, Speed> feed_rate, config::Unit<long double, B, Speed> end_rate = { 0 }) {
        PlanMove<A>(
            unitToAxisUnit<AxisUnit<pos_t, A, Lenght>>(delta),
            unitToAxisUnit<AxisUnit<steps_t, A, Speed>>(feed_rate),
            unitToAxisUnit<AxisUnit<steps_t, A, Speed>>(end_rate));
    }

    /// @returns head position of an axis (last enqueued position)
    /// @param axis axis affected
    pos_t Position(Axis axis) const;

    /// @returns head position of an axis, but in AxisUnit.
    /// The Axis needs to be supplied as the first template argument: Position<axis>().
    /// @see Position
    template <Axis A>
    constexpr AxisUnit<pos_t, A, Lenght> Position() const {
        return AxisUnit<pos_t, A, Lenght> { Position(A) };
    }

    /// Fetch the current position of the axis while stepping. This function is expensive!
    /// It's necessary only in exceptional cases. For regular usage, Position() should
    /// probably be used instead.
    /// @param axis axis affected
    /// @returns the current position of the axis
    pos_t CurPosition(Axis axis) const;

    /// Fetch the current axis position, but in AxisUnit. This function is expensive!
    /// The Axis needs to be supplied as the first template argument: CurPosition<axis>().
    /// @see CurPosition
    template <Axis A>
    constexpr AxisUnit<pos_t, A, Lenght> CurPosition() const {
        return AxisUnit<pos_t, A, Lenght> { CurPosition(A) };
    }

    /// Set the position of an axis. Should only be called when the queue is empty.
    /// @param axis axis affected
    /// @param x position to set
#if !defined(UNITTEST) || defined(UNITTEST_MOTION)
    void SetPosition(Axis axis, pos_t x) {
        axisData[axis].ctrl.SetPosition(x);
    }
#else
    // Force STUB for testing
    void SetPosition(Axis axis, pos_t x);
#endif

    /// Get current acceleration for the selected axis
    /// @param axis axis affected
    /// @returns acceleration
    steps_t Acceleration(Axis axis) const {
        return axisData[axis].ctrl.Acceleration();
    }

    /// Set acceleration for the selected axis
    /// @param axis axis affected
    /// @param accel acceleration
    void SetAcceleration(Axis axis, steps_t accel) {
        axisData[axis].ctrl.SetAcceleration(accel);
    }

    /// Set acceleration for the selected axis, but using AxisUnit. The Axis needs to
    /// be supplied as the first template argument: SetAcceleration<axis>(accel).
    /// @see SetAcceleration, unitToSteps
    template <Axis A>
    void SetAcceleration(AxisUnit<steps_t, A, Accel> accel) {
        SetAcceleration(A, accel.v);
    }

    /// Set acceleration for the selected axis, but using physical units. The Axis needs to
    /// be supplied as the first template argument: SetAcceleration<axis>(accel).
    /// @tparam A axis affected
    /// @tparam B unit base for the axis
    /// @param accel acceleration
    template <Axis A, config::UnitBase B>
    void SetAcceleration(config::Unit<long double, B, Accel> accel) {
        SetAcceleration<A>(unitToAxisUnit<AxisUnit<steps_t, A, Accel>>(accel));
    }

    /// Get current jerk for the selected axis
    /// @param axis axis affected
    /// @returns jerk
    steps_t Jerk(Axis axis) const {
        return axisData[axis].ctrl.Jerk();
    }

    /// Set maximum jerk for the selected axis
    /// @param axis axis affected
    /// @param max_jerk maximum jerk
    void SetJerk(Axis axis, steps_t max_jerk) {
        return axisData[axis].ctrl.SetJerk(max_jerk);
    }

    /// Fetch the target rate of the last planned segment for the requested axis, or the
    /// current effective rate when the move has been aborted.
    /// @param axis axis affected
    /// @returns last rate
    steps_t Rate(Axis axis) const {
        return axisData[axis].ctrl.Rate();
    }

    /// State machine doing all the planning and stepping. Called by the stepping ISR.
    /// @returns the interval for the next tick
#if !defined(UNITTEST) || defined(UNITTEST_MOTION)
    inline st_timer_t Step() {
        st_timer_t timers[NUM_AXIS];

        // step and calculate interval for each new move
        for (uint8_t i = 0; i != NUM_AXIS; ++i) {
            timers[i] = axisData[i].residual;
            if (timers[i] <= config::stepTimerQuantum) {
                if (timers[i] || !axisData[i].ctrl.QueueEmpty()) {
                    st_timer_t next = axisData[i].ctrl.Step(axisParams[i].params);
                    if (next) {
                        timers[i] += next;

                        // axis has been moved, run the tmc2130 Isr for this axis
                        axisData[i].drv.Isr(axisParams[i].params);
                    } else {
                        // axis finished, reset residual
                        timers[i] = 0;
                    }
                }
            }
        }

        // plan next closest interval
        st_timer_t next = timers[0];
        for (uint8_t i = 1; i != NUM_AXIS; ++i) {
            if (timers[i] && (!next || timers[i] < next))
                next = timers[i];
        }

        // update residuals
        for (uint8_t i = 0; i != NUM_AXIS; ++i) {
            axisData[i].residual = (timers[i] ? timers[i] - next : 0);
        }

        return next;
    }
#else
    // Force STUB for testing
    st_timer_t Step();
#endif

    /// @returns true if all planned moves have been finished for all axes
    bool QueueEmpty() const;

    /// @returns true if all planned moves have been finished for one axis
    /// @param axis requested
#if !defined(UNITTEST) || defined(UNITTEST_MOTION)
    bool QueueEmpty(Axis axis) const {
        return axisData[axis].ctrl.QueueEmpty();
    }
#else
    // Force STUB for testing
    bool QueueEmpty(Axis axis) const;
#endif

#if !defined(UNITTEST) || defined(UNITTEST_MOTION)
    /// @returns number of planned moves on an axis
    uint8_t PlannedMoves(Axis axis) const {
        return axisData[axis].ctrl.PlannedMoves();
    }
#else
    // Force STUB for testing
    uint8_t PlannedMoves(Axis axis) const;
#endif
    /// @returns false if new moves can still be planned for one axis
    /// @param axis axis requested
    bool Full(Axis axis) const { return axisData[axis].ctrl.Full(); }

    /// Stop whatever moves are being done for one axis
    /// @param axis axis requested
    /// @param halt When true, also abruptly stop axis movement.
    void AbortPlannedMoves(Axis axis, bool halt = true);

    /// Stop whatever moves are being done on all axes
    /// @param halt When true, also abruptly stop axis movement.
    void AbortPlannedMoves(bool halt = true);

    /// @returns the TMC213 driver associated with the particular axis
    inline hal::tmc2130::TMC2130 &DriverForAxis(Axis axis) {
        return axisData[axis].drv;
    }

    /// @returns the (non-const) TMC2130 driver associated with the particular axis.
    /// Do not use unless you know exactly what you're doing, (i.e., nothing else can possibly be using
    /// the axis. Currently the only valid usage is in the hw sanity module.
    inline hal::tmc2130::TMC2130 &MMU_NEEDS_ATTENTION_DriverForAxis(Axis axis) {
        return axisData[axis].drv;
    }

    /// @returns the controller associated with the particular axis
    inline const pulse_gen::PulseGen &CtrlForAxis(Axis axis) const {
        return axisData[axis].ctrl;
    }

    inline const MotorCurrents &CurrentsForAxis(Axis axis) const {
        return axisData[axis].currents;
    }
    inline void SetIRunForAxis(Axis axis, uint8_t i) {
        axisData[axis].currents.iRun = i;
    }

private:
    struct AxisData {
        TMC2130 drv; ///< Motor driver
        pulse_gen::PulseGen ctrl; ///< Motor controller
        bool enabled; ///< Axis enabled
        MotorCurrents currents; ///< Axis related currents
        st_timer_t residual; ///< Axis timer residual
    };

    /// Helper to initialize AxisData members
    static AxisData DataForAxis(Axis axis) {
        return {
            .drv = {},
            .ctrl = {
                axisParams[axis].jerk,
                axisParams[axis].accel,
            },
            .enabled = false,
            .currents = axisParams[axis].currents
        };
    }

    /// Dynamic axis data
    AxisData axisData[NUM_AXIS] = {
        DataForAxis(Pulley),
        DataForAxis(Selector),
        DataForAxis(Idler),
    };
};

/// ISR initialization
extern void Init();

extern Motion motion;

} // namespace motion
} // namespace modules

namespace mm = modules::motion;

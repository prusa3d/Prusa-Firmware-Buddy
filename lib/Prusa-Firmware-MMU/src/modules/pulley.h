/// @file pulley.h
#pragma once
#include "../config/config.h"
#include "movable_base.h"
#include "motion.h"

namespace modules {

/// The pulley namespace provides all necessary facilities related to the logical model of the pulley device of the MMU unit.
namespace pulley {

namespace mm = modules::motion;

/// The Pulley model is an analogy to Idler and Selector.
/// It encapsulates the same error handling principles like the other two (motored) modules.
/// On the other hand - the Pulley is much simpler, there is no homing, engage/disengage and slots,
/// but it supports free rotation in either directions and some computation on top of it.
class Pulley : public motion::MovableBase {
public:
    inline constexpr Pulley()
        : MovableBase(mm::Pulley) {}

    /// Performs one step of the state machine according to currently planned operation
    /// @returns true if the pulley is ready to accept new commands (i.e. it has finished the last operation)
    bool Step();

    void PlanMove(mm::P_pos_t delta, mm::P_speed_t feed_rate, mm::P_speed_t end_rate = { 0 });

    // NOTE: always_inline is required here to force gcc <= 7.x to evaluate each call at compile time
    void __attribute__((always_inline)) PlanMove(unit::U_mm delta, unit::U_mm_s feed_rate, unit::U_mm_s end_rate = { 0 }) {
        PlanMove(mm::unitToAxisUnit<mm::P_pos_t>(delta),
            mm::unitToAxisUnit<mm::P_speed_t>(feed_rate),
            mm::unitToAxisUnit<mm::P_speed_t>(end_rate));
    }

    /// @returns rounded current position (rotation) of the Pulley
    /// This exists purely to avoid expensive float (long double) computations of distance traveled by the filament
    int32_t CurrentPosition_mm();

    void InitAxis();
    void Disable();

protected:
    virtual void PrepareMoveToPlannedSlot() override {}
    virtual void PlanHomingMoveForward() override {}
    virtual void PlanHomingMoveBack() override {}
    virtual bool FinishHomingAndPlanMoveToParkPos() override;
    virtual void FinishMove() override {}
};

/// The one and only instance of Pulley in the FW
extern Pulley pulley;

} // namespace pulley
} // namespace modules

namespace mpu = modules::pulley;

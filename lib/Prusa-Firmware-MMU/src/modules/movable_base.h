/// @file movable_base.h
#pragma once
#include <stdint.h>
#include "../config/axis.h"
#include "../hal/tmc2130.h"

namespace modules {
namespace motion {

/// Base class for movable modules - #modules::idler::Idler and #modules::selector::Selector contains the common code
class MovableBase {
public:
    /// Internal states of the state machine
    enum {
        Ready = 0, // intentionally set as zero in order to allow zeroing the Idler structure upon startup -> avoid explicit initialization code
        Moving = 1,
        PlannedHome = 2,
        HomeForward = 3,
        HomeBack = 4,
        TMCFailed = 5,
        HomingFailed = 6,
        OnHold = 0x80, ///< needs to be a separate bit due to homing recovery infrastructure
    };

    /// Operation (Engage/Disengage/MoveToSlot) return values
    enum class OperationResult : uint8_t {
        Accepted, ///< the operation has been successfully started
        Refused, ///< another operation is currently underway, cannot start a new one
        Failed ///< the operation could not been started due to HW issues
    };

    inline constexpr MovableBase(config::Axis axis)
        : state(Ready)
        , plannedSlot(-1)
        , currentSlot(-1)
        , homingValid(false)
        , axis(axis)
        , axisStart(0) {}

    /// virtual ~MovableBase(); intentionally disabled, see description in logic::CommandBase

    /// @returns currently active slot
    /// This state is updated only when a planned move is successfully finished, so it is safe for higher-level
    /// state machines to use this call as a waiting condition for the desired state of the derive class (idler/selector)
    /// While homing, Slot() returns 0xff as the current slot index is invalid.
    inline uint8_t Slot() const { return currentSlot; }

    /// @returns internal state of the state machine
    inline uint8_t State() const { return state; }

    inline hal::tmc2130::ErrorFlags TMCErrorFlags() const { return tmcErrorFlags; }

    /// Invalidates the homing flag - that is now used to inform the movable component (Idler or Selector)
    /// that their current coordinates may have been compromised and a new homing move is to be performed.
    /// Each movable component performs the homing move immediately after it is possible to do so:
    /// - Idler immediately (and then moves to desired slot again)
    /// - Selector once there is no filament stuck in it (and then moves to desired slot again)
    /// Homing procedure therefore becomes completely transparent to upper layers
    /// and it will not be necessary to call it explicitly.
    /// Please note this method does not clear any planned move on the component
    /// - on the contrary - the planned move will be peformed immediately after homing
    /// (which makes homing completely transparent)
    inline void InvalidateHoming() { homingValid = false; }

    /// Prepare a homing move of the axis
    /// @returns true if the move has been planned successfully (i.e. movable is NOT on-hold)
    OperationResult PlanHome();

    inline bool HomingValid() const { return homingValid; }

    inline config::Axis Axis() const { return axis; }

    /// Set TMC2130 iRun current level for this axis
    /// iRun == 0 means set the default from config
    void SetCurrents(uint8_t iRun, uint8_t iHold);

    /// Puts the movable on-hold
    /// Also, disables the axis
    void HoldOn();

    /// @returns true if the movable is on-hold
    bool IsOnHold() const { return state & OnHold; }

    /// Allows the movable to move/home again after begin suspended by HoldOn
    void Resume() { state &= ~OnHold; }

#ifndef UNITTEST
protected:
#endif
    /// internal state of the automaton
    uint8_t state;

    /// planned slot - where to move to
    uint8_t plannedSlot;

    /// current slot
    uint8_t currentSlot;

    /// true if the axis is considered as homed
    bool homingValid;

    /// cached TMC2130 error flags - being read only if the axis is enabled and doing something (moving)
    hal::tmc2130::ErrorFlags tmcErrorFlags;

    config::Axis axis;

    int32_t axisStart;

    virtual void PrepareMoveToPlannedSlot() = 0;
    virtual void PlanHomingMoveForward() = 0;
    virtual void PlanHomingMoveBack() = 0;
    /// @returns true if the measured axis length is within the expected range, false otherwise
    virtual bool FinishHomingAndPlanMoveToParkPos() = 0;
    virtual void FinishMove() = 0;
    /// @returns true if the StallGuard signal is to be considered while homing.
    /// It may sound counterintuitive, but due to SG/homing issues on the Idler,
    /// it needs to avoid processing the SG while rotating over the filament.
    /// The Idler must consider SG signal only when close to its real end stops.
    /// Selector considers the SG signal all the time while homing, therefore the default implementation is empty
    virtual bool StallGuardAllowed(bool forward) const { return true; }

    /// Initializes movement of a movable module.
    /// Beware: this operation reinitializes the axis/TMC driver as well (may introduce axis creep as we have seen on the Idler)
    OperationResult InitMovement();

    /// Initializes movement of a movable module without reinitializing the axis/TMC driver
    OperationResult InitMovementNoReinitAxis();

    void PerformMove();

    void PerformHomeForward();
    void PerformHomeBack();

    void HomeFailed();

    void CheckTMC();

    uint16_t AxisDistance(int32_t curPos) const;
};

} // namespace motion
} // namespace modules

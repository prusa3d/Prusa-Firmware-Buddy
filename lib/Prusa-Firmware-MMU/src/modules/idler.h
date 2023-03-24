/// @file idler.h
#pragma once
#include "../config/config.h"
#include "axisunit.h"
#include "movable_base.h"

namespace modules {

/// The idler namespace provides all necessary facilities related to the logical model of the idler device of the MMU unit.
namespace idler {

namespace mm = modules::motion;

/// The Idler model handles asynchronnous Engaging / Disengaging operations and keeps track of idler's current state.
class Idler : public motion::MovableBase {
public:
    inline constexpr Idler()
        : MovableBase()
        , plannedEngage(false)
        , currentlyEngaged(false) {}

    /// Plan engaging of the idler to a specific filament slot
    /// @param slot index to be activated
    /// @returns #OperationResult
    OperationResult Engage(uint8_t slot);

    /// Plan disengaging of the idler, i.e. parking the idler
    /// @returns #OperationResult
    OperationResult Disengage();

    /// Performs one step of the state machine according to currently planned operation
    /// @returns true if the idler is ready to accept new commands (i.e. it has finished the last operation)
    bool Step();

    /// @returns the current state of idler - engaged / disengaged
    /// this state is updated only when a planned move is successfully finished, so it is safe for higher-level
    /// state machines to use this call as a waiting condition for the desired state of the idler
    inline bool Engaged() const { return currentlyEngaged; }

    /// @returns predefined positions of individual slots
    static constexpr mm::I_pos_t SlotPosition(uint8_t slot) {
        return mm::unitToAxisUnit<mm::I_pos_t>(config::idlerSlotPositions[slot]);
    }

    /// @returns the index of idle position of the idler, usually 5 in case of 0-4 valid indices of filament slots
    inline static constexpr uint8_t IdleSlotIndex() { return config::toolCount; }

    /// Initializes the idler after restart/cold boot
    /// Reads the active slot from the EEPROM and decides if the idler is safe to move (not hold the filament while printing)
    /// - free -> home the idler
    /// - blocked -> set idler's position according to the active filament slot
    void Init();

protected:
    virtual void PrepareMoveToPlannedSlot() override;
    virtual void PlanHomingMove() override;
    virtual void FinishHomingAndPlanMoveToParkPos() override;
    virtual void FinishMove() override;

private:
    /// direction of travel - engage/disengage
    bool plannedEngage;

    /// current state
    bool currentlyEngaged;
};

/// The one and only instance of Idler in the FW
extern Idler idler;

} // namespace idler
} // namespace modules

namespace mi = modules::idler;

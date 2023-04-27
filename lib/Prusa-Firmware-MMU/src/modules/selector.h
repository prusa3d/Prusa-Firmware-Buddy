/// @file selector.h
#pragma once
#include "../config/config.h"
#include "axisunit.h"
#include "movable_base.h"

namespace modules {

/// The selector namespace provides all necessary facilities related to the logical model of the selector device of the MMU unit.
namespace selector {

namespace mm = modules::motion;

/// The selector model - handles asynchronnous move operations between filament individual slots and keeps track of selector's current state.
class Selector : public mm::MovableBase {
public:
    inline constexpr Selector()
        : MovableBase(mm::Selector) {}

    /// Plan move of the selector to a specific filament slot
    /// @param slot index to move to
    /// @returns false in case an operation is already underway
    /// @note if(state==OnHold) all attempts to move the selector are rejected with OperationResult::Rejected
    OperationResult MoveToSlot(uint8_t slot);

    /// Performs one step of the state machine according to currently planned operation.
    /// @returns true if the selector is ready to accept new commands (i.e. it has finished the last operation)
    bool Step();

    /// @returns predefined positions of individual slots
    static constexpr mm::S_pos_t SlotPosition(uint8_t slot) {
        return mm::unitToAxisUnit<mm::S_pos_t>(config::selectorSlotPositions[slot]);
    }

    /// @returns the index of idle position of the selector, usually 5 in case of 0-4 valid indices of filament slots
    inline static constexpr uint8_t IdleSlotIndex() { return config::toolCount; }

    /// Initializes the selector after restart/cold boot
    /// Reads the active slot from the EEPROM and decides if the selector is free or blocked by a piece of filament
    /// - free -> home the selector
    /// - blocked -> set selector's position according to the active filament slot
    void Init();

protected:
    virtual void PrepareMoveToPlannedSlot() override;
    virtual void PlanHomingMoveForward() override;
    virtual void PlanHomingMoveBack() override;
    virtual bool FinishHomingAndPlanMoveToParkPos() override;
    virtual void FinishMove() override;

private:
};

/// The one and only instance of Selector in the FW
extern Selector selector;

} // namespace selector
} // namespace modules

namespace ms = modules::selector;

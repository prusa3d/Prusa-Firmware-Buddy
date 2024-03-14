#pragma once
#include "../../../../src/logic/command_base.h"
#include "../../../../src/modules/globals.h"

extern void main_loop();
extern void ForceReinitAllAutomata();

template <typename SM, typename COND>
bool WhileCondition(SM &sm, COND cond, uint32_t maxLoops = 5000) {
    uint32_t step = 0;
    while (cond(step) && --maxLoops) {
        main_loop();
        sm.Step();
        ++step;
    }
    return maxLoops > 0;
}

template <typename SM>
bool WhileTopState(SM &sm, ProgressCode state, uint32_t maxLoops = 5000) {
    return WhileCondition(
        sm, [&](uint32_t) { return sm.TopLevelState() == state; }, maxLoops);
}

bool EnsureActiveSlotIndex(uint8_t slot, modules::globals::FilamentLoadState loadState);

void SetFINDAStateAndDebounce(bool press);
bool SimulateUnloadToFINDA(uint32_t step, uint32_t fsOff, uint32_t findaOff);
bool SimulateFeedToFINDA(uint32_t step, uint32_t findaOn);
bool SimulateRetractFromFINDA(uint32_t step, uint32_t findaOff);

void PressButtonAndDebounce(logic::CommandBase &cb, uint8_t btnIndex, bool fromPrinter);
void ClearButtons(logic::CommandBase &cb);

void SetMinimalBowdenLength();
void SetFSensorStateAndDebounce(bool press);

// these are recommended max steps for simulated movement of the idler and selector
// - roughly the amount of motion steps from one end to the other + some margin
// ... could be computed in the future from the pre-set number of microsteps and real positions
static constexpr uint32_t idlerEngageDisengageMaxSteps = 40000UL;
static constexpr uint32_t selectorMoveMaxSteps = 40000UL;

void HomeIdlerAndSelector();

void SimulateErrDisengagingIdler(logic::CommandBase &cb, ErrorCode deferredEC);

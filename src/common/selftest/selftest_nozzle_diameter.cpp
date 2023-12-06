#include "selftest_nozzle_diameter.hpp"
#include "i_selftest_part.hpp"
#include "selftest_loop_result.hpp"
#include "selftest_log.hpp"

LOG_COMPONENT_REF(Selftest);

namespace selftest {

SelftestPartNozzleDiameter::SelftestPartNozzleDiameter(IPartHandler &state_machine,
    const SelftestNozzleDiameterConfig &,
    SelftestNozzleDiameterResult &result)
    : state_machine(state_machine)
    , result(result) {
    log_info(Selftest, "%s Started", SelftestNozzleDiameterConfig::part_name);
}

LoopResult SelftestPartNozzleDiameter::statePrepare() {
    IPartHandler::SetFsmPhase(PhasesSelftest::NozzleDiameter_prepare);
    result.selected_diameter = 0.f;
    return LoopResult::RunNext;
}

LoopResult SelftestPartNozzleDiameter::stateAskDefaultNozzleDiameter() {
    IPartHandler::SetFsmPhase(PhasesSelftest::NozzleDiameter_ask_user_for_type);
    auto response = state_machine.GetButtonPressed();
    switch (response) {
    case Response::NozzleDiameter_04:
        result.selected_diameter = 0.4f;
        break;
    case Response::NozzleDiameter_06:
        result.selected_diameter = 0.6f;
        break;
    default:
        return LoopResult::RunCurrent;
    }
    return LoopResult::RunNext;
}

LoopResult SelftestPartNozzleDiameter::stateSaveResultToEeprom() {
    IPartHandler::SetFsmPhase(PhasesSelftest::NozzleDiameter_save_selected_value);
    // "Empty" state to change the screen so user doesn't stay on screen with button.
    // The actual saving is done in phaseNozzleDiameter
    return LoopResult::RunNext;
}

} // namespace selftest

#include "selftest_nozzle_diameter_interface.hpp"
#include "marlin_server.hpp"
#include "selftest_part.hpp"
#include "selftest_result.hpp"
#include "selftest_nozzle_diameter.hpp"
#include <config_store/store_instance.hpp>

namespace selftest {

bool phaseNozzleDiameter(IPartHandler *&pNozzleDiameter) {
    static SelftestNozzleDiameterResult static_result {};
    static SelftestNozzleDiameterConfig config {};

    if (pNozzleDiameter == nullptr) {
        pNozzleDiameter = selftest::Factory::CreateDynamical<SelftestPartNozzleDiameter>(config, static_result,
            &SelftestPartNozzleDiameter::statePrepare,
            &SelftestPartNozzleDiameter::stateAskDefaultNozzleDiameter,
            &SelftestPartNozzleDiameter::stateSaveResultToEeprom);
    }

    auto in_progress = pNozzleDiameter->Loop();
    FSM_CHANGE__LOGGING(IPartHandler::GetFsmPhase());
    if (!in_progress) {
        if (static_result.selected_diameter > 0.0f) {
            // PASSED
            config_store().selftest_result_nozzle_diameter.set(TestResult::TestResult_Passed);

            // TODO: we might not want to set the value for all the hotends!!
            // If so we need to set remember how many tools were configured
            // and at least warn user if they added a tool to the printer.
            for (auto i = 0u; i < HOTENDS; ++i) {
                config_store().set_nozzle_diameter(i, static_result.selected_diameter);
            }
        } else {
            // SOMEHOW FAILED TO SET ANY VALUE??
            bsod("");
        }
        static_result.selected_diameter = -1.0f;

        delete pNozzleDiameter;
        pNozzleDiameter = nullptr;
    }

    return in_progress;
}

} // namespace selftest

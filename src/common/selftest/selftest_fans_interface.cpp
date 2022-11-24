/**
 * @file selftest_fans_interface.cpp
 * @author Radek Vana
 * @date 2021-09-24
 */
#include "selftest_fan.h"
#include "selftest_fans_type.hpp"
#include "marlin_server.hpp"
#include "selftest_part.hpp"
#include "eeprom.h"

namespace selftest {
static SelftestFan_t staticResultPrintFan;
static SelftestFan_t staticResultHeatbreakFan;

// data for both subtests must be sent together
// we could loose some events, so we must be sending entire state of both parts
bool phaseFans(IPartHandler *&pPrintFan, IPartHandler *&pHeatbreakFan, const FanConfig_t &config_print_fan, const FanConfig_t &config_heatbreak_fan) {
    // clang-format off
    pPrintFan = pPrintFan ? pPrintFan :
    selftest::Factory::CreateDynamical<CSelftestPart_Fan>(config_print_fan,
        staticResultPrintFan,
        &CSelftestPart_Fan::stateStart, &CSelftestPart_Fan::stateWaitStopped,
        &CSelftestPart_Fan::stateCycleMark, &CSelftestPart_Fan::stateWaitRpm,
        &CSelftestPart_Fan::stateMeasureRpm);

    pHeatbreakFan = pHeatbreakFan ? pHeatbreakFan :
    selftest::Factory::CreateDynamical<CSelftestPart_Fan>(config_heatbreak_fan,
        staticResultHeatbreakFan,
        &CSelftestPart_Fan::stateStart, &CSelftestPart_Fan::stateWaitStopped,
        &CSelftestPart_Fan::stateCycleMark, &CSelftestPart_Fan::stateWaitRpm,
        &CSelftestPart_Fan::stateMeasureRpm);
    // clang-format on

    bool print_fan_in_progress = pPrintFan->Loop();
    bool heatbreak_fan_in_progress = pHeatbreakFan->Loop();
    SelftestFans_t result(staticResultPrintFan, staticResultHeatbreakFan);
    FSM_CHANGE_WITH_DATA__LOGGING(Selftest, IPartHandler::GetFsmPhase(), result.Serialize());
    if (print_fan_in_progress || heatbreak_fan_in_progress) {
        return true;
    }

    SelftestResultEEprom_t eeres;
    eeres.ui32 = variant8_get_ui32(eeprom_get_var(EEVAR_SELFTEST_RESULT));
    eeres.printFan = uint8_t(pPrintFan->GetResult());
    eeres.heatBreakFan = uint8_t(pHeatbreakFan->GetResult());
    eeprom_set_var(EEVAR_SELFTEST_RESULT, variant8_ui32(eeres.ui32));
    delete pPrintFan;
    pPrintFan = nullptr;
    delete pHeatbreakFan;
    pHeatbreakFan = nullptr;
    return false;
}
} // namespace selftest

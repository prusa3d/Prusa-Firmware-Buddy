/**
 * @file selftest_heaters_interface.cpp
 * @author Radek Vana
 * @date 2021-09-24
 */
#include "selftest_heaters_interface.hpp"
#include "selftest_heater.h"
#include "../../Marlin/src/module/temperature.h"
#include "marlin_server.hpp"
#include "selftest_part.hpp"
#include "selftest_log.hpp"
#include "eeprom.h"

//disable power check, since measurement does not work
#ifdef HAS_ADVANCED_POWER
    #undef HAS_ADVANCED_POWER
#endif

#ifdef HAS_ADVANCED_POWER
    #include "power_check_both.hpp"
#endif
namespace selftest {

static SelftestHeaters_t resultHeaters;

#if HAS_TEMP_HEATBREAK_CONTROL
static void HeatbreakCorrelation(CSelftestPart_Heater &h) {
    assert(h.m_config.type == heater_type_t::Nozzle);
    const uint8_t tool_nr = h.m_config.tool_nr;
    int32_t temp = thermalManager.degHeatbreak(tool_nr);
    if ((temp > h.m_config.heatbreak_max_temp) || (temp < h.m_config.heatbreak_min_temp)) {
        resultHeaters.noz[tool_nr].heatbreak_error = true;
        h.state_machine.Fail();
    }
}
#else
static void HeatbreakCorrelation(CSelftestPart_Heater &h) {}
#endif // HAS_TEMP_HEATBREAK_CONTROL

#ifndef HAS_ADVANCED_POWER
// Dummy class in case there is no advanced power
class PowerCheckBoth {
    constexpr PowerCheckBoth() = default;
    PowerCheckBoth(const PowerCheckBoth &) = delete;

public:
    void Callback() {}

    constexpr void BindNozzle(CSelftestPart_Heater &f) {}
    constexpr void BindBed(CSelftestPart_Heater &f) {}
    constexpr void UnBindNozzle() {}
    constexpr void UnBindBed() {}

    static PowerCheckBoth &Instance() {
        static PowerCheckBoth ret;
        return ret;
    }
};
#endif

void phaseHeaters_noz_ena(std::array<IPartHandler *, HOTENDS> &pNozzles, const std::span<const HeaterConfig_t> config_nozzle) {

    for (size_t i = 0; i < config_nozzle.size(); i++) {
        // reset result
        resultHeaters.noz[i] = SelftestHeater_t(0, SelftestSubtestState_t::undef, SelftestSubtestState_t::undef);

        if (pNozzles[i] == nullptr) {
            // clang-format off
            auto pNoz = selftest::Factory::CreateDynamical<CSelftestPart_Heater>(config_nozzle[i],
                resultHeaters.noz[i],
                &CSelftestPart_Heater::stateStart,
                &CSelftestPart_Heater::stateTakeControlOverFans, &CSelftestPart_Heater::stateFansActivate,
                &CSelftestPart_Heater::stateCooldownInit, &CSelftestPart_Heater::stateCooldown,
                &CSelftestPart_Heater::stateFansDeactivate,
                &CSelftestPart_Heater::stateTargetTemp, &CSelftestPart_Heater::stateWait,
                &CSelftestPart_Heater::stateMeasure);
            // clang-format on

            pNozzles[i] = pNoz;
            //add same hooks for both "states changes" and "does not change"
            pNoz->SetStateChangedHook([](CSelftestPart_Heater &h) {
                HeatbreakCorrelation(h);
                PowerCheckBoth::Instance().Callback();
            });
            pNoz->SetStateRemainedHook([](CSelftestPart_Heater &h) {
                HeatbreakCorrelation(h);
                PowerCheckBoth::Instance().Callback();
            });
            //todo: not working properly for multiple nozzles
            PowerCheckBoth::Instance().BindNozzle(pNoz->GetInstance());
        }
    }
}

void phaseHeaters_bed_ena(IPartHandler *&pBed, const HeaterConfig_t &config_bed) {
    // reset result
    resultHeaters.bed = SelftestHeater_t(0, SelftestSubtestState_t::undef, SelftestSubtestState_t::undef);

    if (pBed == nullptr) {
        // brr unnecessary dynamic allocation .. not my code, I just moved it .. TODO rewrite
        // clang-format off
        auto pBed_ = selftest::Factory::CreateDynamical<CSelftestPart_Heater>(config_bed,
            resultHeaters.bed,
            &CSelftestPart_Heater::stateStart,
            &CSelftestPart_Heater::stateCooldownInit, &CSelftestPart_Heater::stateCooldown,
            &CSelftestPart_Heater::stateTargetTemp, &CSelftestPart_Heater::stateWait,
            &CSelftestPart_Heater::stateMeasure);
        // clang-format on

        pBed = pBed_;
        //add same hooks for both "states changes" and "does not change"
        pBed_->SetStateChangedHook([](CSelftestPart_Heater &h) {
            PowerCheckBoth::Instance().Callback();
        });
        pBed_->SetStateRemainedHook([](CSelftestPart_Heater &h) {
            PowerCheckBoth::Instance().Callback();
        });
        PowerCheckBoth::Instance().BindBed(pBed_->GetInstance());
    }
}

// data for both subtests must be sent together
// we could loose some events, so we must be sending entire state of both parts
bool phaseHeaters(std::array<IPartHandler *, HOTENDS> &pNozzles, IPartHandler *&pBed) {
    // true when nozzle just finished test
    bool just_finished_noz[HOTENDS] = { false };
    for (size_t i = 0; i < HOTENDS; i++) {
        if (pNozzles[i]) {
            just_finished_noz[i] = !pNozzles[i]->Loop();
        }
    }

    // true when just finished nozzle test
    bool just_finished_bed = pBed && !pBed->Loop();

    // change dialog state
    FSM_CHANGE_WITH_EXTENDED_DATA__LOGGING(Selftest, IPartHandler::GetFsmPhase(), resultHeaters);

    // Continue below only if some of the tests just finished, if not, just run this again until some finishes
    if (!just_finished_bed && !std::ranges::any_of(just_finished_noz, [](bool val) { return val; })) {
        return true;
    }

    // just finished noz or bed, it is extremely unlikely they would finish both at same time
    SelftestResult eeres;
    eeprom_get_selftest_results(&eeres);
    HOTEND_LOOP() {
        if (just_finished_noz[e]) {
            eeres.tools[e].nozzle = pNozzles[e]->GetResult();
        }
    }
    if (just_finished_bed) {
        eeres.bed = pBed->GetResult();
    }
    eeprom_set_selftest_results(&eeres);

    for (size_t i = 0; i < HOTENDS; i++) {
        if (just_finished_noz[i]) {
            PowerCheckBoth::Instance().UnBindNozzle();
            delete pNozzles[i];
            pNozzles[i] = nullptr;
        }
    }

    if (just_finished_bed) {
        PowerCheckBoth::Instance().UnBindBed();
        delete pBed;
        pBed = nullptr;
    }

    // if any is still in progress, return true to run this again, otherwise end test
    if (std::ranges::any_of(pNozzles, [](IPartHandler *val) { return val != nullptr; }) || pBed != nullptr) {
        return true;
    }

    return false; // finished
}
} // namespace selftest

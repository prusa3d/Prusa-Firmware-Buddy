/**
 * @file selftest_heaters_interface.cpp
 * @author Radek Vana
 * @date 2021-09-24
 */
#include "selftest_heaters_interface.hpp"
#include "selftest_heater.h"
#include "selftest_hot_end_sock.hpp"
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
static void HeatbreakCorrelation([[maybe_unused]] CSelftestPart_Heater &h) {}
#endif // HAS_TEMP_HEATBREAK_CONTROL

#ifndef HAS_ADVANCED_POWER
// Dummy class in case there is no advanced power
class PowerCheckBoth {
    constexpr PowerCheckBoth() = default;
    PowerCheckBoth(const PowerCheckBoth &) = delete;

public:
    void Callback() {}

    constexpr void BindNozzle([[maybe_unused]] CSelftestPart_Heater &f) {}
    constexpr void BindBed([[maybe_unused]] CSelftestPart_Heater &f) {}
    constexpr void UnBindNozzle() {}
    constexpr void UnBindBed() {}

    static PowerCheckBoth &Instance() {
        static PowerCheckBoth ret;
        return ret;
    }
};
#endif

void phaseHeaters_noz_ena(std::array<IPartHandler *, HOTENDS> &pNozzles, const std::span<const HeaterConfig_t> config_nozzle) {
    resultHeaters.tested_parts |= to_one_hot(SelftestHeaters_t::TestedParts::noz);

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
    resultHeaters.tested_parts |= to_one_hot(SelftestHeaters_t::TestedParts::bed);

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
        pBed_->SetStateChangedHook([]([[maybe_unused]] CSelftestPart_Heater &h) {
            PowerCheckBoth::Instance().Callback();
        });
        pBed_->SetStateRemainedHook([]([[maybe_unused]] CSelftestPart_Heater &h) {
            PowerCheckBoth::Instance().Callback();
        });
        PowerCheckBoth::Instance().BindBed(pBed_->GetInstance());
    }
}

// data for both subtests must be sent together
// we could loose some events, so we must be sending entire state of both parts
bool phaseHeaters(std::array<IPartHandler *, HOTENDS> &pNozzles, IPartHandler *&pBed) {
    // true when nozzle just finished test
    bool just_finished_noz[HOTENDS] {};
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

    resultHeaters.tested_parts = 0; // reset tested parts so they can be set next time again
    return false;                   // finished
}

SelftestHotEndSockType sock_result;
bool retry_heater = false;
bool get_retry_heater() { return retry_heater; }

bool phase_hot_end_sock(IPartHandler *&machine, const HotEndSockConfig &config) {

    machine = machine ? machine : Factory::CreateDynamical<selftest::CSelftestPart_HotEndSock>(
                  // clang-format off
    config, sock_result,
    &CSelftestPart_HotEndSock::stateStart, &CSelftestPart_HotEndSock::stateAskAdjust,
    &CSelftestPart_HotEndSock::stateAskSockInit, &CSelftestPart_HotEndSock::stateAskSock,
    // Disable asking questions about nozzle
   // &CSelftestPart_HotEndSock::stateAskNozzleInit, &CSelftestPart_HotEndSock::stateAskNozzle,
    &CSelftestPart_HotEndSock::stateAskRetryInit, &CSelftestPart_HotEndSock::stateAskRetry);
    // clang-format on
    bool in_progress = machine->Loop();
    FSM_CHANGE_WITH_DATA__LOGGING(Selftest, IPartHandler::GetFsmPhase(), sock_result.Serialize());

    if (in_progress) {
        return true;
    }

    retry_heater = machine->GetResult() != TestResult_Skipped;

    eeprom_set_bool(EEVAR_NOZZLE_SOCK, sock_result.has_sock);
    eeprom_set_ui8(EEVAR_NOZZLE_TYPE, sock_result.prusa_stock_nozzle ? 0 : 1);

    delete machine;
    machine = nullptr;
    return false;
}

} // namespace selftest

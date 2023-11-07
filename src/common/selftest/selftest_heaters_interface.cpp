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
#include <config_store/store_instance.hpp>
#include <printers.h>

/**
 * The following #if override fakes advanced power to be not supported within this file
 * Definitely not a nice solution to disable power check, but it was inherited from original code.
 * Keep in mind that enabling this for printers that will use the PowerCheckBooth (MK4) will cause an
 * untested code path in PowerCheckBoth to be executed.
 */
#if !PRINTER_IS_PRUSA_XL
    // disable power check, since measurement does not work
    #include <option/has_advanced_power.h>
    #if HAS_ADVANCED_POWER()
        #undef HAS_ADVANCED_POWER
        #define HAS_ADVANCED_POWER() 0
    #endif
#endif

#if HAS_ADVANCED_POWER()
    #include "advanced_power.hpp"
    #if !PRINTER_IS_PRUSA_XL
        #include "power_check_both.hpp"
    #endif
#endif

LOG_COMPONENT_REF(Selftest);
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

#if !HAS_ADVANCED_POWER()
// Dummy class in case there is no advanced power
class PowerCheckBoth {
    constexpr PowerCheckBoth() = default;
    PowerCheckBoth(const PowerCheckBoth &) = delete;

public:
    void Callback([[maybe_unused]] CSelftestPart_Heater &part) {}

    constexpr void BindNozzle([[maybe_unused]] CSelftestPart_Heater *) {}
    constexpr void BindBed([[maybe_unused]] CSelftestPart_Heater *) {}
    constexpr void UnBindNozzle() {}
    constexpr void UnBindBed() {}

    static PowerCheckBoth &Instance() {
        static PowerCheckBoth ret;
        return ret;
    }
};
#endif

// Shared check callback
// Splits implementation for printers with independent bed, nozzle measurement and others
static inline void check_callback(CSelftestPart_Heater &part) {
#if !PRINTER_IS_PRUSA_XL
    PowerCheckBoth::Instance().Callback(part);
#else
    part.single_check_callback();
#endif
}

void phaseHeaters_noz_ena(std::array<IPartHandler *, HOTENDS> &pNozzles, const std::span<const HeaterConfig_t> config_nozzle) {
    resultHeaters.tested_parts |= to_one_hot(SelftestHeaters_t::TestedParts::noz);

    for (size_t i = 0; i < config_nozzle.size(); i++) {
        // reset result
        resultHeaters.noz[i] = SelftestHeater_t(0, SelftestSubtestState_t::undef, SelftestSubtestState_t::undef);

        if (pNozzles[i] == nullptr) {
#if HAS_TOOLCHANGER()
            if (!prusa_toolchanger.is_tool_enabled(config_nozzle[i].tool_nr)) {
                continue; // don't test disabled tools
            }
#endif

            auto pNoz = selftest::Factory::CreateDynamical<CSelftestPart_Heater>(config_nozzle[i],
                resultHeaters.noz[i],
                &CSelftestPart_Heater::stateCheckHbrPassed,
                &CSelftestPart_Heater::stateShowSkippedDialog,
                &CSelftestPart_Heater::stateSetup,
                &CSelftestPart_Heater::stateTakeControlOverFans, &CSelftestPart_Heater::stateFansActivate,
                &CSelftestPart_Heater::stateCooldownInit, &CSelftestPart_Heater::stateCooldown,
                &CSelftestPart_Heater::stateFansDeactivate,
                &CSelftestPart_Heater::stateTargetTemp, &CSelftestPart_Heater::stateWait,
                &CSelftestPart_Heater::stateMeasure
#if HAS_ADVANCED_POWER()
                ,
                &CSelftestPart_Heater::stateCheckLoadChecked
#endif
            );

            pNozzles[i] = pNoz;
            // add same hooks for both "states changes" and "does not change"
            pNoz->SetStateChangedHook([](CSelftestPart_Heater &part) {
                HeatbreakCorrelation(part);
                check_callback(part);
            });

            pNoz->SetStateRemainedHook([](CSelftestPart_Heater &part) {
                HeatbreakCorrelation(part);
                check_callback(part);
            });
#if !PRINTER_IS_PRUSA_XL
            PowerCheckBoth::Instance().BindNozzle(&pNoz->GetInstance());
#endif
        }
    }
}

void phaseHeaters_bed_ena(IPartHandler *&pBed, const HeaterConfig_t &config_bed) {
    // Hack: only if we're running both bed and nozzle test together
    // NOTE: yes, terrible, depends on enabling nozzle test first and bed test second
    if (resultHeaters.tested_parts & to_one_hot(SelftestHeaters_t::TestedParts::noz)) {
        // if heatbreak fan hasn't passed, we can't run the nozzle heater
        // check. The decision was to skip heaters check altogether and instead
        // show a dialog saying why we're skipping it. The heater check runs
        // two selftests in parallel: bed & nozzle. If we're showing the
        // dialog, we disable the bed check here (well, we skip enabling it),
        // as the nozzle selftest takes care of showing the dialog and this one
        // running in parallel would just make a mess.
        SelftestTool tool_res = config_store().selftest_result.get().tools[0];
        if (tool_res.heatBreakFan != TestResult_Passed || tool_res.fansSwitched != TestResult_Passed) {
            return;
        }
    }

    // reset result
    resultHeaters.bed = SelftestHeater_t(0, SelftestSubtestState_t::undef, SelftestSubtestState_t::undef);
    resultHeaters.tested_parts |= to_one_hot(SelftestHeaters_t::TestedParts::bed);

    if (pBed == nullptr) {
        // brr unnecessary dynamic allocation .. not my code, I just moved it .. TODO rewrite
        auto pBed_ = selftest::Factory::CreateDynamical<CSelftestPart_Heater>(
            config_bed,
            resultHeaters.bed,
            &CSelftestPart_Heater::stateSetup,
            &CSelftestPart_Heater::stateCooldownInit, &CSelftestPart_Heater::stateCooldown,
            &CSelftestPart_Heater::stateTargetTemp, &CSelftestPart_Heater::stateWait,
            &CSelftestPart_Heater::stateMeasure
#if HAS_ADVANCED_POWER()
            ,
            &CSelftestPart_Heater::stateCheckLoadChecked
#endif
        );

        pBed = pBed_;
        // add same hooks for both "states changes" and "does not change"
        pBed_->SetStateChangedHook(&check_callback);
        pBed_->SetStateRemainedHook(&check_callback);
#if !PRINTER_IS_PRUSA_XL
        PowerCheckBoth::Instance().BindBed(&pBed_->GetInstance());
#endif
    }
}

// data for both subtests must be sent together
// we could loose some events, so we must be sending entire state of both parts
bool phaseHeaters(std::array<IPartHandler *, HOTENDS> &pNozzles, IPartHandler **pBed) {
    // true when nozzle just finished test
    bool just_finished_noz[HOTENDS] {};
    for (size_t i = 0; i < HOTENDS; i++) {
        if (pNozzles[i]) {
            just_finished_noz[i] = !pNozzles[i]->Loop();
        }
    }

    // true when just finished nozzle test
    const bool just_finished_bed = pBed && *pBed && !(*pBed)->Loop();

    // change dialog state
    FSM_CHANGE_WITH_EXTENDED_DATA__LOGGING(Selftest, IPartHandler::GetFsmPhase(), resultHeaters);

    // Continue below only if some of the tests just finished, if not, just run this again until some finishes
    if (!just_finished_bed && !std::ranges::any_of(just_finished_noz, [](bool val) { return val; })) {
        return true;
    }

    // just finished noz or bed, it is extremely unlikely they would finish both at same time
    SelftestResult eeres = config_store().selftest_result.get();
    HOTEND_LOOP() {
        if (just_finished_noz[e]) {
            eeres.tools[e].nozzle = pNozzles[e]->GetResult();
        }
    }
    if (just_finished_bed) {
        assert(pBed && *pBed);
        eeres.bed = (*pBed)->GetResult();
    }
    config_store().selftest_result.set(eeres);

    for (size_t i = 0; i < HOTENDS; i++) {
        if (just_finished_noz[i]) {
#if !PRINTER_IS_PRUSA_XL
            PowerCheckBoth::Instance().UnBindNozzle();
#endif
            delete pNozzles[i];
            pNozzles[i] = nullptr;
        }
    }

    if (just_finished_bed) {
        assert(pBed && *pBed);
#if !PRINTER_IS_PRUSA_XL
        PowerCheckBoth::Instance().UnBindBed();
#endif
        delete *pBed;
        *pBed = nullptr;
    }

    // if any is still in progress, return true to run this again, otherwise end test
    if (std::ranges::any_of(pNozzles, [](IPartHandler *val) { return val != nullptr; }) || *pBed != nullptr) {
        return true;
    }

    resultHeaters.tested_parts = 0; // reset tested parts so they can be set next time again
    return false; // finished
}

SelftestHotEndSockType sock_result;
bool retry_heater = false;
bool get_retry_heater() { return retry_heater; }

bool phase_hot_end_sock(IPartHandler *&machine, const HotEndSockConfig &config) {
    if (!machine) {
        machine = Factory::CreateDynamical<selftest::CSelftestPart_HotEndSock>(
            config,
            sock_result,
            &CSelftestPart_HotEndSock::stateStart,
            &CSelftestPart_HotEndSock::stateAskAdjust,
            &CSelftestPart_HotEndSock::stateAskSockInit,
            &CSelftestPart_HotEndSock::stateAskSock,
            // Disable asking questions about nozzle
            // &CSelftestPart_HotEndSock::stateAskNozzleInit, &CSelftestPart_HotEndSock::stateAskNozzle,
            &CSelftestPart_HotEndSock::stateAskRetryInit,
            &CSelftestPart_HotEndSock::stateAskRetry);
    }
    bool in_progress = machine->Loop();
    FSM_CHANGE_WITH_DATA__LOGGING(Selftest, IPartHandler::GetFsmPhase(), sock_result.Serialize());

    if (in_progress) {
        return true;
    }

    retry_heater = machine->GetResult() != TestResult_Skipped;

    config_store().nozzle_sock.set(sock_result.has_sock);
    config_store().nozzle_type.set(sock_result.prusa_stock_nozzle ? 0 : 1);

    delete machine;
    machine = nullptr;
    return false;
}

} // namespace selftest

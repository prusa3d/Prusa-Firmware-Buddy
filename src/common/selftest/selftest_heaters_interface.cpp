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
#include "configuration_store.hpp"

//disable power check, since measurement does not work
#ifdef HAS_ADVANCED_POWER
    #undef HAS_ADVANCED_POWER
#endif

#ifdef HAS_ADVANCED_POWER
    #include "power_check_both.hpp"
#endif
namespace selftest {
static SelftestHeater_t staticResultNoz;
static SelftestHeater_t staticResultBed;
static bool nozzle_result_valid = false;
static bool bed_result_valid = false;

static void HeatbreakCorrelation(CSelftestPart_Heater &h) {}

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

void phaseHeaters_noz_ena(IPartHandler *&pNozzle, const HeaterConfig_t &config_nozzle) {
    if (!pNozzle) {
        // brr unnecessary dynamic allocation .. not my code, I just moved it .. TODO rewrite
        // clang-format off
        auto pNoz = selftest::Factory::CreateDynamical<CSelftestPart_Heater>(config_nozzle,
            staticResultNoz,
            &CSelftestPart_Heater::stateStart,
            &CSelftestPart_Heater::stateTakeControlOverFans, &CSelftestPart_Heater::stateFansActivate,
            &CSelftestPart_Heater::stateCooldownInit, &CSelftestPart_Heater::stateCooldown,
            &CSelftestPart_Heater::stateFansDeactivate,
            &CSelftestPart_Heater::stateTargetTemp, &CSelftestPart_Heater::stateWait,
            &CSelftestPart_Heater::stateMeasure);
        // clang-format on

        pNozzle = pNoz;
        //add same hooks for both "states changes" and "does not change"
        pNoz->SetStateChangedHook([](CSelftestPart_Heater &h) {
            HeatbreakCorrelation(h);
            PowerCheckBoth::Instance().Callback();
        });
        pNoz->SetStateRemainedHook([](CSelftestPart_Heater &h) {
            HeatbreakCorrelation(h);
            PowerCheckBoth::Instance().Callback();
        });
        PowerCheckBoth::Instance().BindNozzle(pNoz->GetInstance());
    }
}

void phaseHeaters_bed_ena(IPartHandler *&pBed, const HeaterConfig_t &config_bed) {
    if (!pBed) {
        // brr unnecessary dynamic allocation .. not my code, I just moved it .. TODO rewrite
        // clang-format off
        auto pBed_ = selftest::Factory::CreateDynamical<CSelftestPart_Heater>(config_bed,
            staticResultBed,
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
bool phaseHeaters(IPartHandler *&pNozzle, IPartHandler *&pBed) {
    //nothing to test
    if ((pNozzle == nullptr) && (pBed == nullptr))
        return false;

    // show result even after one part of selftest ended
    if (pNozzle)
        nozzle_result_valid = true;
    if (pBed)
        bed_result_valid = true;

    bool in_progress_noz = pNozzle && (pNozzle->Loop());
    bool in_progress_bed = pBed && (pBed->Loop());
    bool finished_noz = pNozzle && (!in_progress_noz);
    bool finished_bed = pBed && (!in_progress_bed);

    // change dialog state
    // in case pNozzle/pBed is nullptr its result is undefined, use default one instead
    SelftestHeaters_t result(nozzle_result_valid ? staticResultNoz : SelftestHeater_t(), bed_result_valid ? staticResultBed : SelftestHeater_t());
    fsm_change(ClientFSM::Selftest, IPartHandler::GetFsmPhase(), result.Serialize());

    // just finished noz or bed, it is extremely unlikely they would finish both at same time
    SelftestResultEEprom_t eeres;
    eeres.ui32 = config_store().selftest_result.get();
    if (finished_noz) {
        eeres.nozzle = uint8_t(pNozzle->GetResult());
    }
    if (finished_bed) {
        eeres.bed = uint8_t(pBed->GetResult());
    }
    config_store().selftest_result.set(eeres.ui32);

    if (finished_noz) {
        PowerCheckBoth::Instance().UnBindNozzle();
        delete pNozzle;
        pNozzle = nullptr;
    }

    if (finished_bed) {
        PowerCheckBoth::Instance().UnBindBed();
        delete pBed;
        pBed = nullptr;
    }

    if (in_progress_noz || in_progress_bed) {
        return true;
    }

    // must clear this flags, so result is shown properly next run
    nozzle_result_valid = false;
    bed_result_valid = false;

    return false; // finished
}
} // namespace selftest

/// @file

#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_reporting.h"
#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_mk4.cpp"
#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/buttons.h"
#include "../common/marlin_server.hpp"
#include <common/marlin_vars.hpp>
#include "../common/sound.hpp"
#include "mmu2_error_converter.h"
#include "mmu2_fsm.hpp"
#include "mmu2_reporter.hpp"
#include "fail_bucket.hpp"
#include "pause_stubbed.hpp"
#include <logging/log.hpp>
#include <config_store/store_instance.hpp>
#include <odometer.hpp>
#include <filament_to_load.hpp>
#include <M70X.hpp>

LOG_COMPONENT_REF(MMU2);

namespace MMU2 {

void CheckErrorScreenUserInput() {
    // A "temporary" workaround:
    // Save a button if any - we cannot poll the FSM infrastructure for buttons (responses)
    // because each get_response_from_phase is a destructive read (behaves more like a pop from a stack).
    // For the same reason this piece of code cannot be refactored into a direct call of SetButtonResponse
    // - after extracting one valid button, any repeated calls to ReportErrorHook would overwrite it with a "none" button
    //
    // The only problem with this location is the fact, that the ReportErrorHook is called roughly once per second, which may introduce some GUI lag.
    // However, on the 8-bit, the solution is the same, so probably not a big deal...

    const bool is_err_active = marlin_vars().peek_fsm_states([](const fsm::States &states) {
        return states.is_active(ClientFSM::Load_unload) && states[ClientFSM::Load_unload]->GetPhase() == std::to_underlying(PhasesLoadUnload::MMU_ERRWaitingForUser);
    });
    if (!is_err_active) {
        return;
    }

    const Response rsp = marlin_server::get_response_from_phase(PhasesLoadUnload::MMU_ERRWaitingForUser);
    if (rsp != Response::_none) {
        SetButtonResponse(ResponseToButtonOperations(rsp));
    }
}

void ReportErrorHook(ErrorData d) {
    if (d.errorCode == ErrorCode::OK || d.errorCode == ErrorCode::RUNNING) {
        return;
    }

    // An error always causes one specific screen to occur
    // Its content is given by the error code translated into Prusa-Error-Codes MMU
    // That needs to be coded into the context data passed to the screen
    // - in this case the raw pointer to error description
    if (d.errorCode != ErrorCode::MMU_NOT_RESPONDING) {
        log_debug(MMU2, "Error report: CIP=%" PRIu8 " ec=%u es=%u", d.rawCommandInProgress, static_cast<unsigned>(d.errorCode), static_cast<unsigned>(d.errorSource));
        Fsm::Instance().reporter.SetReport(d);
    } else {
        log_error(MMU2, "Error report: CIP=%" PRIu8 " ec=%u es=%u - cannot be done, fsm closed", static_cast<unsigned>(d.rawCommandInProgress), static_cast<unsigned>(d.errorCode), static_cast<unsigned>(d.errorSource));
    }
}

void ReportProgressHook(ProgressData d) {
    if (Fsm::Instance().IsActive()) { // prevent accidental FSM change reports if there is no MMU progress/error dialog shown
        log_debug(MMU2, "Report: CIP=%" PRIu8 " pc=%" PRIu8, d.rawCommandInProgress, d.rawProgressCode);
        Fsm::Instance().reporter.SetReport(d);
    } else {
        log_warning(MMU2, "Report: CIP=%" PRIu8 " pc=%" PRIu8 " - cannot be done, fsm closed", d.rawCommandInProgress, d.rawProgressCode);
    }
}

void BeginReport([[maybe_unused]] ProgressData d) {
    Fsm::Instance().Activate();
}

void EndReport([[maybe_unused]] ProgressData d) {
    Fsm::Instance().Deactivate();
}

TryLoadUnloadReporter::TryLoadUnloadReporter(float /*delta_mm*/) {
    // @@TODO
}

void TryLoadUnloadReporter::Progress(bool /*sensorState*/) {
    // @@TODO
}

void TryLoadUnloadReporter::Render(uint8_t /*col*/, bool /*sensorState*/) {
    // @@TODO
}

void TryLoadUnloadReporter::DumpToSerial() {
    // @@TODO
}

/// @returns true if the MMU is communicating and available
/// can change at runtime
bool MMUAvailable() { return true; }

// has global flag in FW/EEPROM
// changed by user from menu
bool UseMMU() { return true; }

/// Disables MMU in EEPROM
void DisableMMUInSettings() {
    filament_gcodes::mmu_off();
}

void MakeSound(SoundType s) {
    return; // @@TODO currently broken
    switch (s) {
    case Confirm:
        sound::play(::SoundType::single_beep);
        break;
    case Prompt:
        sound::play(::SoundType::standard_prompt);
        break;
    default:
        sound::play(::SoundType::standard_alert);
        break;
    }
}

void FullScreenMsgCut([[maybe_unused]] uint8_t slot) {
    // @@TODO
}
void FullScreenMsgTest([[maybe_unused]] uint8_t slot) {
    // @@TODO
}
void FullScreenMsgEject([[maybe_unused]] uint8_t slot) {
    // @@TODO
}
void FullScreenMsgLoad([[maybe_unused]] uint8_t slot) {
    // @@TODO
}

void ScreenUpdateEnable() {
    // @@TODO
}

void ReportErrorHookDynamicRender() {
    // not required on MK4
}

void ReportErrorHookSensorLineRender() {
    // @@TODO
}

void FullScreenMsgRestoringTemperature() {
    // @@TODO
}

void ScreenClear() {
    // @@TODO
}

void IncrementLoadFails() {
    auto &store = config_store();
    auto transaction = store.get_backend().transaction_guard();
    store.mmu2_load_fails.set(store.mmu2_load_fails.get() + 1);
    store.mmu2_total_load_fails.set(store.mmu2_total_load_fails.get() + 1);
    FailLeakyBucket::instance.add_failure();
}

void TrackMaintenance(const ErrorCode error) {
    // All MMU error codes that are relevant to extruder mainplate
    if (error == ErrorCode::FSENSOR_DIDNT_SWITCH_ON || error == ErrorCode::LOAD_TO_EXTRUDER_FAILED || error == ErrorCode::FSENSOR_DIDNT_SWITCH_OFF || error == ErrorCode::FSENSOR_TOO_EARLY) {
        FailLeakyBucket::instance.add_failure();
    }
}

void IncrementMMUFails() {
    auto &store = config_store();
    auto transaction = store.get_backend().transaction_guard();
    store.mmu2_fails.set(store.mmu2_fails.get() + 1);
    store.mmu2_total_fails.set(store.mmu2_total_fails.get() + 1);
}

void IncrementMMUChanges() {
    Odometer_s::instance().add_mmu_change();
    const auto total_successes = Odometer_s::instance().get_mmu_changes();
    FailLeakyBucket::instance.success(total_successes);
}

bool cutter_enabled() {
    return config_store().mmu2_cutter.get();
}

bool TuneMenuEntered() {
    // @@TODO
    return false;
}

void tuneIdlerStallguardThreshold() {
    // @@TODO
}

void UpdateCurrentFilamentType(uint8_t slot) {
    if (slot < VirtualToolIndex::count) {
        filament::set_type_to_load(config_store().get_filament_type(slot));
    } // ignore out-of-bounds slot indices (uknown filament types)
}

} // namespace MMU2

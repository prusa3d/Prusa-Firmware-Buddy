/// @file

#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_reporting.h"
#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_mk4.cpp"
#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/buttons.h"
#include "../../lib/Prusa-Firmware-MMU/src/logic/progress_codes.h"
#include "../../lib/Prusa-Firmware-MMU/src/logic/error_codes.h"
#include "../common/marlin_server.hpp"
#include "../common/sound.hpp"
#include "mmu2_error_converter.h"
#include "mmu2_fsm.hpp"
#include "mmu2_reporter.hpp"
#include "pause_stubbed.hpp"
#include "log.h"
#include <config_store/store_instance.hpp>

LOG_COMPONENT_REF(MMU2);

namespace MMU2 {

void ReportErrorHook(CommandInProgress cip, uint16_t ec, uint8_t es) {
    // An error always causes one specific screen to occur
    // Its content is given by the error code translated into Prusa-Error-Codes MMU
    // That needs to be coded into the context data passed to the screen
    // - in this case the raw pointer to error description
    if (ec != (uint16_t)ErrorCode::MMU_NOT_RESPONDING) {
        log_error(MMU2, "Error report: CIP=%" PRIu8 " ec=% " PRIu16 " es=% " PRIu8, cip, ec, es);
        Fsm::Instance().reporter.Change(cip, ErrorCode(ec), MMU2::ErrorSource(es));
    } else {
        log_error(MMU2, "Error report: CIP=%" PRIu8 " ec=% " PRIu16 " es=% " PRIu16 " - cannot be done, fsm closed", cip, ec, es);
    }

    // A temporary workaround:
    // Save a button if any - we cannot poll the FSM infrastructure for buttons (responses)
    // because each GetResponse is a destructive read (behaves more like a pop from a stack).
    // For the same reason this piece of code cannot be refactored into a direct call of SetButtonResponse
    // - after extracting one valid button, any repeated calls to ReportErrorHook would overwrite it with a "none" button
    //
    // The only problem with this location is the fact, that the ReportErrorHook is called roughly once per second, which may introduce some GUI lag.
    // However, on the 8-bit, the solution is the same, so probably not a big deal...
    Response rsp = Fsm::Instance().GetResponse();
    if (rsp != Response::_none) {
        SetButtonResponse(ResponseToButtonOperations(rsp));
    }
}

void ReportProgressHook(CommandInProgress cip, uint16_t ec) {
    if (Fsm::Instance().IsActive()) { // prevent accidental FSM change reports if there is no MMU progress/error dialog shown
        log_info(MMU2, "Report: CIP=%" PRIu8 " ec=% " PRIu16, cip, ec);
        Fsm::Instance().reporter.Change(cip, ProgressCode(ec));
    } else {
        log_warning(MMU2, "Report: CIP=%" PRIu8 " ec=% " PRIu16 " - cannot be done, fsm closed", cip, ec);
    }
}

void BeginReport([[maybe_unused]] CommandInProgress cip, [[maybe_unused]] uint16_t ec) {
    Fsm::Instance().Activate();
}

void EndReport([[maybe_unused]] CommandInProgress cip, [[maybe_unused]] uint16_t ec) {
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

/// @returns true if the MMU is communicating and available
/// can change at runtime
bool MMUAvailable() { return true; }

// has global flag in FW/EEPROM
// changed by user from menu
bool UseMMU() { return true; }

void MakeSound(SoundType s) {
    return; // @@TODO currently broken
    switch (s) {
    case Confirm:
        Sound_Play(eSOUND_TYPE::SingleBeep);
        break;
    case Prompt:
        Sound_Play(eSOUND_TYPE::StandardPrompt);
        break;
    default:
        Sound_Play(eSOUND_TYPE::StandardAlert);
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
    //    eeprom_increment_byte((uint8_t *)EEPROM_MMU_LOAD_FAIL);
    //    eeprom_increment_word((uint16_t *)EEPROM_MMU_LOAD_FAIL_TOT);
}

void IncrementMMUFails() {
    //    eeprom_increment_byte((uint8_t *)EEPROM_MMU_FAIL);
    //    eeprom_increment_word((uint16_t *)EEPROM_MMU_FAIL_TOT);
}

bool cutter_enabled() {
    return config_store().mmu2_cutter.get();
}

} // namespace MMU2

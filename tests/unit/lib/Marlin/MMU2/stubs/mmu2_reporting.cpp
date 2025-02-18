#include <mmu2_reporting.h>
#include "stub_interfaces.h"

namespace MMU2 {

// beware:
// cip (command in progress) is an enum coded with letters representing the currently running command
// therefore it should be safe to do '(char)cip'
#define mockLog_RecordFnCipEc(cip, ec) mockLog.Record(std::string { mockLog.MethodName(__PRETTY_FUNCTION__) } + "(" + (char)(cip ? cip : 'x') + ", " + std::to_string((uint16_t)ec) + ")")

/// Called at the begin of every MMU operation
void BeginReport(ProgressData d) {
    mockLog_RecordFnCipEc(d.rawCommandInProgress, d.rawProgressCode);
}

/// Called at the end of every MMU operation
void EndReport(ProgressData d) {
    mockLog_RecordFnCipEc(d.rawCommandInProgress, d.rawProgressCode);
}

void CheckErrorScreenUserInput() {
    // Do nothing
}

/// @brief Called when the MMU or MK3S sends operation error (even repeatedly).
/// Render MMU error screen on the LCD. This must be non-blocking
/// and allow the MMU and printer to communicate with each other.
/// @param[in] ec error code
/// @param[in] es error source
void ReportErrorHook(ErrorData d) {
    mockLog_RecordFnCipEc(d.rawCommandInProgress, d.errorCode);
}

/// Called when the MMU sends operation progress update
void ReportProgressHook(ProgressData d) {
    mockLog_RecordFnCipEc(d.rawCommandInProgress, d.rawProgressCode);
}

TryLoadUnloadReporter::TryLoadUnloadReporter(float delta_mm) {
    mockLog_RecordFn();
}

void TryLoadUnloadReporter::Progress(bool sensorState) {
    mockLog_RecordFn();
}

void TryLoadUnloadReporter::Render(uint8_t col, bool sensorState) {
    mockLog_RecordFn();
}

void TryLoadUnloadReporter::DumpToSerial() {
    mockLog_RecordFn();
}

/// Remders the sensor status line. Also used by the "resume temperature" screen.
void ReportErrorHookDynamicRender() {
    mockLog_RecordFn();
}

/// Renders the static part of the sensor state line. Also used by "resuming temperature screen"
void ReportErrorHookSensorLineRender() {
    mockLog_RecordFn();
}

/// @returns true if the MMU is communicating and available
/// report to the outer world, not important for the unit tests
bool MMUAvailable() { return false; }

/// Global Enable/Disable use MMU (to be stored in EEPROM)
/// report to the outer world, not important for the unit tests
bool UseMMU() { return false; }

void DisableMMUInSettings() {
    // Do nothing
}

/// Increments EEPROM cell - number of failed loads into the nozzle
/// Note: technically, this is not an MMU error but an error of the printer.
void IncrementLoadFails() {
    mockLog_RecordFn();
}

/// Increments EEPROM cell - number of MMU errors
void IncrementMMUFails() {
    mockLog_RecordFn();
}

/// Increments EEPROM cell - number of MMU errors
void TrackMaintenance(const ErrorCode error) {
    mockLog_RecordFn();
}

void IncrementMMUChanges() {
    mockLog_RecordFn();
}

/// @returns true when Cutter is enabled in the menus
bool cutter_enabled() { return true; }

void MakeSound(SoundType s) {
    mockLog_RecordFn();
}

void FullScreenMsgCut(uint8_t slot) {
    mockLog_RecordFn();
}
void FullScreenMsgEject(uint8_t slot) {
    mockLog_RecordFn();
}
void FullScreenMsgTest(uint8_t slot) {
    mockLog_RecordFn();
}
void FullScreenMsgLoad(uint8_t slot) {
    mockLog_RecordFn();
}
void FullScreenMsgRestoringTemperature() {
    mockLog_RecordFn();
}

void ScreenUpdateEnable() {
    mockLog_RecordFn();
}
void ScreenClear() {
    mockLog_RecordFn();
}

bool TuneMenuEntered() {
    // @@TODO
    return false;
}

void tuneIdlerStallguardThreshold() {
    // @@TODO
}

} // namespace MMU2

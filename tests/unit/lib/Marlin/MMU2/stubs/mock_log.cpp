#include "stub_interfaces.h"
#include <new> // bring in placement new
#include <mmu2_mk4.h>
#include <algorithm>

bool MockLog::Matches(std::initializer_list<std::string_view> msgs) const {
    return std::equal(log.begin(), log.end(), msgs.begin());
}

bool MockLog::MatchesExpected() const {
    return std::equal(log.cbegin(), log.cend(), expected.cbegin());
}

void MockLog::Record(std::string_view s) {
    log.emplace_back(std::string { s });
}

void MockLog::Clear() {
    log.clear();
    expected.clear();
}

void MockLog::DeduplicateLog() {
    log.erase(std::unique(log.begin(), log.end()), log.end());
}

void MockLog::DeduplicateExpected() {
    expected.erase(std::unique(expected.begin(), expected.end()), expected.end());
}

MockLog mockLog;

void InitEnvironment(MMU2::FilamentState fsensor) {
    mockLog.Clear();
    mmu2SerialSim.rxbuff.clear();
    mmu2SerialSim.txbuffQ.clear();
    marlinLogSim.log.clear();
    ioSim.clear();
    ioSimI = ioSim.cend();
    SetMillis(0);
    SetHotendTargetTemp(0);
    SetHotendCurrentTemp(0);
    MMU2::fs = fsensor;

    // reset the mmu instance
    new (&MMU2::mmu2) MMU2::MMU2();
}

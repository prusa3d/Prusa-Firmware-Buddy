#include "stub_interfaces.h"
#include <new> // bring in placement new
#include <mmu2_mk4.h>

bool MockLog::Matches(std::initializer_list<std::string_view> msgs) const {
    return std::equal(log.begin(), log.end(), msgs.begin());
}

void MockLog::Record(std::string_view s) {
    log.emplace_back(std::string { s });
}

void MockLog::Clear() {
    log.clear();
}

MockLog mockLog;

void InitEnvironment() {
    mockLog.Clear();
    mmu2SerialSim.rxbuff.clear();
    mmu2SerialSim.txbuffQ.clear();
    marlinLogSim.log.clear();

    // reset the mmu instance
    new (&MMU2::mmu2) MMU2::MMU2();
}

#include "stub_interfaces.h"
#include <mmu2_serial.h>
#include <typeinfo>
#include "catch2/catch.hpp"

MMU2SerialSim mmu2SerialSim;

IOSim::const_iterator ioSimI;
IOSim ioSim;
size_t ioSimMockCheckIndex = 0;

void IOSimStart(std::initializer_list<IOSimRec> init) {
    ioSim = init;
    ioSimI = ioSim.cbegin();
    ioSimMockCheckIndex = 0;
    mmu2SerialSim.runIOSimOnNextRead = true;
    mmu2SerialSim.SetAutomagic();
}

void IOSimCheck() {
    // This hook is called whenever the mmu2Serial is about to read the first byte of the next rx message - aka MMU is trying to respond
    // This is the best spot to perform some magic
    // - check the mmu2Serial tx buffer - what has been transmitted to the MMU
    // - check the mockLog - what has been called meanwhile
    // - check the marlinLog - what has been printed on the serial line - may be that's an overkill
    // - advance to the next record - take the next rx and submit it to the mmu2Serial for input
    if (ioSimI == ioSim.cend()) {
        return;
    }

    const IOSimRec &r = *ioSimI;

    //    if( r.tx.empty() ){
    //        CHECK(mmu2SerialSim.txbuffQ.empty());
    //    } else {
    //        REQUIRE_FALSE(mmu2SerialSim.txbuffQ.empty());
    //        CHECK(AppendCRC(r.tx) == mmu2SerialSim.txbuffQ.back() );
    //    }

    if (!r.mock.empty()) {
        //        CHECK(mockLog.log.empty());
        //    } else {
        //        CHECK(std::equal(mockLog.log.cbegin() + ioSimMockCheckIndex, mockLog.log.cend(), r.mock.cbegin()));
        //        ioSimMockCheckIndex = mockLog.log.size();
        // whatever is expected to be produced around this call, put it into the mock log for later evaluation
        std::for_each(r.mock.cbegin(), r.mock.cend(), [](std::string s) { mockLog.expected.emplace_back(s); });
    }

    //    if( r.marlin.empty() ){
    //        CHECK(marlinLogSim.log.empty());
    //    } else {
    //        CHECK(std::equal(marlinLogSim.log.begin(), marlinLogSim.log.end(), r.marlin.begin()));
    //    }

    if (r.work) {
        r.work();
    }

    // sumbit new "received data" from the MMU
    if (!r.rx.empty()) {
        mmu2SerialSim.rxbuff = AppendCRC(r.rx);
    }
    SetTimeoutCountdown(r.incMs);

    // advance to next record
    ++ioSimI;
    mmu2SerialSim.txbuffQ.clear();
}

void MMU2SerialSim::SetRxBuffCRC(std::string_view s) {
    rxbuff = s.empty() ? s : AppendCRC(s);
}

namespace MMU2 {

MMU2Serial mmu2Serial;

void MMU2Serial::begin() {
    mockLog_RecordFn();
}

int MMU2Serial::read() {
    if (mmu2SerialSim.runIOSimOnNextRead) {
        IOSimCheck();
        mmu2SerialSim.runIOSimOnNextRead = false;
    }

    if (mmu2SerialSim.rxbuff.empty()) {
        if (mmu2SerialSim.automagic) {
            mmu2SerialSim.runIOSimOnNextRead = true;
        }
        return -1;
    }

    int c = (unsigned char)mmu2SerialSim.rxbuff.front();
    mmu2SerialSim.rxbuff.erase(0, 1);
    return c;
}

size_t MMU2Serial::write(const uint8_t *buffer, size_t size) {
    mmu2SerialSim.txbuffQ.emplace_back(std::string(buffer, buffer + size));
    return size;
}

void MMU2Serial::flush() {
    mockLog_RecordFn();
    mmu2SerialSim.txbuffQ.clear();
}

void MMU2Serial::close() {
    mockLog_RecordFn();
    mmu2SerialSim.txbuffQ.clear();
}

void MMU2Serial::check_recovery() {
}

} // namespace MMU2

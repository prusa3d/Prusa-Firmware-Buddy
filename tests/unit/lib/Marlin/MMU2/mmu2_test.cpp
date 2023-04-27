#include "catch2/catch.hpp"

#include "protocol_logic.h"
#include "mmu2_fsensor.h"

#include <deque>
#include <regex>
#include <string>

#define supportedMmuFWVersionMajor    2
#define supportedMmuFWVersionMinor    1
#define supportedMmuFWVersionRevision 9
#define supportedMmuFWVersionBuild    700

#define xstr(s) Str(s)
#define Str(s)  #s

static constexpr uint8_t MMU2_TOOL_CHANGE_LOAD_LENGTH = 30; // mm
static constexpr uint8_t PULLEY_SLOW_FEED_RATE = 20;        // mm/s

// stubbed external interfaces to play with
extern "C" {

static uint32_t ms = 0;
uint32_t millis(void) { return ms; }

// a simple way of playing with timeouts
void SetMillis(uint32_t m) { ms = m; }
}

std::string rxbuff;
std::deque<std::string> txbuffQ;

namespace MMU2 {

void MMU2Serial::begin(uint32_t baud) {}

int MMU2Serial::read() {
    if (rxbuff.empty())
        return -1;

    int c = (unsigned char)rxbuff.front();
    rxbuff.erase(0, 1);
    return c;
}

size_t MMU2Serial::write(const uint8_t *buffer, size_t size) {
    txbuffQ.push_back(std::string(buffer, buffer + size));
    return size;
}

void MMU2Serial::flush() {
    txbuffQ.clear();
}

MMU2Serial mmu2Serial;

FilamentState fs = FilamentState::NOT_PRESENT;

FilamentState WhereIsFilament() {
    return fs;
}

} // namespace MMU2

using PST = MMU2::ProtocolLogic::State;
using PSC = MMU2::ProtocolLogic::Scope;

// extracted from the unit tests of the protocol itself
std::string AppendCRC(const std::string_view s, uint8_t crc) {
    char tmp[5]; // *ffn are 4 bytes + \0 at the end
    snprintf(tmp, 5, "*%x\n", (unsigned)crc);
    return std::string(s) + std::string(tmp);
}

std::string AppendCRCWrite(const std::string_view src) {
    uint8_t crc = modules::crc::CRC8::CCITT_updateCX(0, src[0]);
    // scan hex value
    size_t charsRead = 0;
    uint8_t rqValue = std::stoul(src.data() + 1, &charsRead, 16);
    crc = modules::crc::CRC8::CCITT_updateCX(crc, rqValue);
    if (src[2 + charsRead] == 'A') {
        // response confirmation
        crc = modules::crc::CRC8::CCITT_updateW(crc, 0);
        crc = modules::crc::CRC8::CCITT_updateCX(crc, src[2 + charsRead]); // param code
        crc = modules::crc::CRC8::CCITT_updateW(crc, 0);
        return AppendCRC(src, crc);
    } else {
        uint16_t paramValue = std::stoul(src.data() + 2 + charsRead, nullptr, 16);
        crc = modules::crc::CRC8::CCITT_updateW(crc, paramValue);
        return AppendCRC(src, crc);
    }
}

std::string AppendCRC(const std::string_view src) {
    // this code basically needs parsing of the input text and compute the CRC from the parsed data
    REQUIRE(src.size() > 1);
    // code
    if (src[0] == 'W')
        return AppendCRCWrite(src);

    size_t charsRead = 0;
    uint8_t crc = modules::crc::CRC8::CCITT_updateCX(0, src[0]);
    // scan hex value
    uint8_t rqValue = std::stoul(src.data() + 1, &charsRead, 16);
    crc = modules::crc::CRC8::CCITT_updateCX(crc, rqValue);
    crc = modules::crc::CRC8::CCITT_updateW(crc, 0);
    if (!src[1 + charsRead])
        return AppendCRC(src, crc); // eof
    // [2] is a space
    REQUIRE(src.size() > 2 + charsRead);
    crc = modules::crc::CRC8::CCITT_updateCX(crc, src[2 + charsRead]); // param code
    if (!src[3 + charsRead]) {
        crc = modules::crc::CRC8::CCITT_updateW(crc, 0);
        return AppendCRC(src, crc); // eof
    }
    REQUIRE(src.size() > 3 + charsRead);
    uint16_t paramValue = std::stoul(src.data() + 3 + charsRead, nullptr, 16);
    crc = modules::crc::CRC8::CCITT_updateW(crc, paramValue);
    return AppendCRC(src, crc);
}

//std::string AppendCRCWrite(const std::string_view src) {
//    // this code basically needs parsing of the input text and compute the CRC from the parsed data
//    REQUIRE(src.size() > 3);
//    // code
//    uint8_t crc = modules::crc::CRC8::CCITT_updateCX(0, src[0]);
//    // scan hex value
//    uint8_t rqValue = std::stoul(src.data() + 1, nullptr, 16);
//    crc = modules::crc::CRC8::CCITT_updateCX(crc, rqValue);
//    // [2] is a space
//    // value follows immediately
//    uint16_t paramValue = std::stoul(src.data() + 3, nullptr, 16);
//    crc = modules::crc::CRC8::CCITT_updateW(crc, paramValue);
//    return AppendCRC(src, crc);
//}

bool StepAndCheckState(MMU2::ProtocolLogic &pl, const std::string_view rxContent, PST plState, PSC plScope, MMU2::StepStatus stepStatus, const std::string_view txContent) {
    rxbuff = rxContent.empty() ? rxContent : AppendCRC(rxContent);
    auto sr = pl.Step();
    CHECKED_ELSE(pl.state == plState) {
        return false;
    }
    CHECKED_ELSE(sr == stepStatus) {
        return false;
    }
    CHECKED_ELSE(pl.currentScope == plScope) {
        return false;
    }

    if (txContent.empty()) {
        CHECKED_ELSE(txbuffQ.empty()) {
            return false;
        }
    } else {
        CHECKED_ELSE(!txbuffQ.empty()) {
            return false;
        }
        CHECKED_ELSE(txbuffQ.back() == AppendCRC(txContent)) {
            return false;
        }
    }
    txbuffQ.clear();
    return true;
}

bool StepAndCheckState2(MMU2::ProtocolLogic &pl, const std::string_view rxContent, PST plState, MMU2::StepStatus stepStatus, const char *txContent0, const char *txContent1) {
    rxbuff = AppendCRC(rxContent);
    auto sr = pl.Step();
    CHECKED_ELSE(pl.state == plState) {
        return false;
    }
    CHECKED_ELSE(sr == stepStatus) {
        return false;
    }
    CHECKED_ELSE(txbuffQ.size() == 2) {
        return false;
    }
    CHECKED_ELSE(txbuffQ[0] == AppendCRC(txContent0)) {
        return false;
    }
    CHECKED_ELSE(txbuffQ[1] == AppendCRC(txContent1)) {
        return false;
    }

    txbuffQ.clear();
    return true;
}

void InitCommunication2(MMU2::ProtocolLogic &pl, std::string command);

void InitCommunication(MMU2::ProtocolLogic &pl) {
    using namespace MMU2;

    rxbuff.clear();
    txbuffQ.clear();

    // as it starts, the communication shall be disabled
    REQUIRE(pl.state == PST::Stopped);

    // stepping the state machine should keep it in Stopped state
    // and step result shall be ??? Processing?
    for (uint8_t i = 0; i < 10; ++i) {
        REQUIRE(StepAndCheckState(pl, "", PST::Stopped, PSC::Stopped, Processing, ""));
    }

    // enable the state machine
    pl.Start();
    REQUIRE(StepAndCheckState(pl, "", PST::InitSequence, PSC::StartSeq, Processing, "S0"));

    // step the machine a couple of times, waiting for response from the MMU
    for (uint8_t i = 0; i < 10; ++i) {
        REQUIRE(StepAndCheckState(pl, "", PST::InitSequence, PSC::StartSeq, Processing, ""));
        ++ms; // pretend one millisecond elapsed
    }

    InitCommunication2(pl, "X0");
}

template <typename T>
std::string RegisterReq(const MMU2::ProtocolLogic &pl, const T *arrayOfIndices, uint8_t i) {
    std::ostringstream s;
    s << "R" << std::hex << static_cast<uint32_t>(*(arrayOfIndices + i));
    std::string sx = s.str();
    return sx; // .str();
}

std::string RegisterReq8(const MMU2::ProtocolLogic &pl, uint8_t i) {
    return RegisterReq(pl, pl.regs8Addrs, i);
}

std::string RegisterReq16(const MMU2::ProtocolLogic &pl, uint8_t i) {
    return RegisterReq(pl, pl.regs16Addrs, i);
}

void QueryRegisters(MMU2::ProtocolLogic &pl, PSC scope = PSC::Idle, PSC terminalScope = PSC::Idle, MMU2::StepStatus terminalStatus = MMU2::Finished, const char *terminalMsg = "") {
    using namespace MMU2;
    // 8bit
    for (uint8_t i = 1; i < pl.regs8Count; ++i) {
        StepAndCheckState(pl, RegisterReq8(pl, i - 1) + " A0", PST::Running, scope, Processing, RegisterReq8(pl, i));
    }

    // transition between 8bit and 16bit regs
    StepAndCheckState(pl, RegisterReq8(pl, pl.regs8Count - 1) + " A0", PST::Running, scope, Processing, RegisterReq16(pl, 0));

    // 16 bit
    for (uint8_t i = 1; i < pl.regs16Count; ++i) {
        StepAndCheckState(pl, RegisterReq16(pl, i - 1) + " A0", PST::Running, scope, Processing, RegisterReq16(pl, i));
    }

    // finished
    StepAndCheckState(pl, RegisterReq16(pl, pl.regs16Count - 1) + " A0", PST::Running, terminalScope, terminalStatus, terminalMsg);
}

std::string SetRegister8(const MMU2::ProtocolLogic &pl, uint8_t i, uint8_t value) {
    std::ostringstream s;
    s << "W" << std::hex << (uint32_t)pl.initRegs8Addrs[i] << " " << std::hex << (uint32_t)value;
    return s.str();
}

std::string SetRegister8Ack(const MMU2::ProtocolLogic &pl, uint8_t i) {
    std::ostringstream s;
    s << "W" << std::hex << (uint32_t)pl.initRegs8Addrs[i] << " A";
    return s.str();
}

void InitialSetupRegisters(MMU2::ProtocolLogic &pl) {
    using namespace MMU2;
    // when adding a new init register it is necessary to add a StepAndCheckState line + set the register's correct value
    StepAndCheckState(pl, SetRegister8Ack(pl, 0), PST::InitSequence, PSC::StartSeq, Processing, SetRegister8(pl, 1, PULLEY_SLOW_FEED_RATE));
    // transition to fsensor
    StepAndCheckState(pl, SetRegister8Ack(pl, 1), PST::InitSequence, PSC::StartSeq, Processing, "f0");
}

void InitCommunication2(MMU2::ProtocolLogic &pl, std::string command) {
    using namespace MMU2;
    // perform the init sequence
    StepAndCheckState(pl, "S0 A" xstr(supportedMmuFWVersionMajor), PST::InitSequence, PSC::StartSeq, Processing, "S1");
    ++ms;
    StepAndCheckState(pl, "S1 A" xstr(supportedMmuFWVersionMinor), PST::InitSequence, PSC::StartSeq, Processing, "S2");
    ++ms;
    StepAndCheckState(pl, "S2 A" xstr(supportedMmuFWVersionRevision), PST::InitSequence, PSC::StartSeq, Processing, "S3");
    ++ms;
    StepAndCheckState(pl, "S3 A" xstr(supportedMmuFWVersionBuild), PST::InitSequence, PSC::StartSeq, Processing, SetRegister8(pl, 0, MMU2_TOOL_CHANGE_LOAD_LENGTH));

    InitialSetupRegisters(pl);

    ++ms;
    StepAndCheckState(pl, "f0 A0", PST::Running, PSC::Idle, Processing, "Q0");
    ++ms;
    StepAndCheckState(pl, command + " F0", PST::Running, PSC::Idle, Processing, RegisterReq8(pl, 0));
    QueryRegisters(pl);
    // we have successfully completed the init sequence
}

void IdleOperation(MMU2::ProtocolLogic &pl) {
    using namespace MMU2;
    // check for repeated queries while idle (protocol heartbeat)

    // FINDA query + response follows immediately
    ms += heartBeatPeriod;
    REQUIRE(StepAndCheckState(pl, "", PST::Running, PSC::Idle, Processing, "Q0"));
    REQUIRE(StepAndCheckState(pl, "X0 F0", PST::Running, PSC::Idle, Processing, RegisterReq8(pl, 0)));
    QueryRegisters(pl);

    ms += heartBeatPeriod;

    // try several times more with longer timing
    for (int i = 0; i < 10; ++i) {
        REQUIRE(StepAndCheckState(pl, "", PST::Running, PSC::Idle, Processing, "Q0"));

        // receive a response after 100ms - should reset the uart timeout
        ms += MMU2::heartBeatPeriod / 3;
        // FINDA query + response follows immediately
        REQUIRE(StepAndCheckState(pl, "X0 F0", PST::Running, PSC::Idle, Processing, RegisterReq8(pl, 0)));
        QueryRegisters(pl);

        // waiting + not sending anything
        ms += MMU2::heartBeatPeriod / 3;
        REQUIRE(StepAndCheckState(pl, "", PST::Running, PSC::Idle, Finished, ""));

        // waiting + not sending anything
        ms += MMU2::heartBeatPeriod / 3;
        REQUIRE(StepAndCheckState(pl, "", PST::Running, PSC::Idle, Finished, ""));

        ms += MMU2::heartBeatPeriod / 3 + 3; // avoid rounding errors
    }
}

void CommandOperation(MMU2::ProtocolLogic &pl, std::string cmdRq) {
    using namespace MMU2;

    REQUIRE_FALSE(txbuffQ.empty());
    REQUIRE(txbuffQ.back() == AppendCRC(cmdRq));
    txbuffQ.clear();
    ++ms;

    // receive a response
    REQUIRE(StepAndCheckState(pl, cmdRq + " A", PST::Running, PSC::Command, Processing, ""));

    ms += MMU2::heartBeatPeriod;

    // query command state
    REQUIRE(StepAndCheckState(pl, "", PST::Running, PSC::Command, Processing, "Q0"));
    ++ms;

    // receive a response and report fsensor state immediately
    REQUIRE(StepAndCheckState(pl, cmdRq + " P1", PST::Running, PSC::Command, Processing, "f0"));

    // receive a response and ask for register status immediately
    REQUIRE(StepAndCheckState(pl, "f0 A", PST::Running, PSC::Command, Processing, RegisterReq8(pl, 0)));
    // receive stats
    QueryRegisters(pl, PSC::Command, PSC::Command, Processing);

    ms += MMU2::heartBeatPeriod;

    // query state
    REQUIRE(StepAndCheckState(pl, "", PST::Running, PSC::Command, Processing, "Q0"));

    ++ms;

    // receive a finish response
    REQUIRE(StepAndCheckState(pl, cmdRq + " F0", PST::Running, PSC::Idle, Finished, ""));
}

/// Verify the protocol_logic layer - sitting idle
TEST_CASE("Marlin::MMU2::ProtocolLogic Idle", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial, MMU2_TOOL_CHANGE_LOAD_LENGTH, PULLEY_SLOW_FEED_RATE);

    InitCommunication(pl);
    IdleOperation(pl);
}

/// Verify the protocol_logic layer - command
TEST_CASE("Marlin::MMU2::ProtocolLogic Command", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial, MMU2_TOOL_CHANGE_LOAD_LENGTH, PULLEY_SLOW_FEED_RATE);

    InitCommunication(pl);

    // cycle through all slots to simulate regular behavior
    for (uint8_t slot = 0; slot < 5; ++slot) {
        // send a command
        pl.ToolChange(slot);
        char expectedMsgPrefix[3] = "T_";
        expectedMsgPrefix[1] = slot + '0';
        CommandOperation(pl, expectedMsgPrefix);
        // continue with an Idle state
        IdleOperation(pl);
    }
}

void ResponseTimeout(MMU2::ProtocolLogic &pl) {
    using namespace MMU2;

    // cause a timeout waiting for a response
    ms += MMU2::dataLayerTimeout * 2;
    REQUIRE(StepAndCheckState(pl, "", PST::InitSequence, PSC::StartSeq, Processing, "S0"));

    // we should end up in a Processing state while having a first occurrence of CommunicationTimeout recorded
    // and the state machine immediately tries to start the communication again by sending the InitSequence.
    for (uint8_t i = 0; i < MMU2::DropOutFilter::maxOccurrences - 2; ++i) {
        ms += MMU2::linkLayerTimeout; // let's simulate no response
        REQUIRE(StepAndCheckState(pl, "", PST::InitSequence, PSC::StartSeq, Processing, "S0"));
    }
    ms += MMU2::linkLayerTimeout;
    // now we shall report the error
    REQUIRE(StepAndCheckState(pl, "", PST::InitSequence, PSC::StartSeq, CommunicationTimeout, "S0"));
}

/// Verify the protocol_logic layer - timeouts waiting for a response msg
TEST_CASE("Marlin::MMU2::ProtocolLogic TimeoutResponse", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial, MMU2_TOOL_CHANGE_LOAD_LENGTH, PULLEY_SLOW_FEED_RATE);

    InitCommunication(pl);

    // verify timeout multiple times
    for (uint8_t i = 0; i < 10; ++i) {
        pl.ToolChange(0);
        ResponseTimeout(pl);
        InitCommunication2(pl, "T0");
    }
}

void SimProtocolError(MMU2::ProtocolLogic &pl) {
    using namespace MMU2;
    ++ms;
    // receive a response
    rxbuff = "DeadBeef\n";
    auto sr = pl.Step();

    // we should end up in a Processing state while having a first occurrence of ProtocolError recorded
    // and the state machine waits for dataLayerTimeout to restart the communication
    REQUIRE(sr == Processing);
    REQUIRE(pl.currentScope == PSC::DelayedRestart);

    // let's simulate no response - should end up in start seq
    ms += MMU2::linkLayerTimeout;
    sr = pl.Step();

    for (uint8_t i = 0; i < MMU2::DropOutFilter::maxOccurrences - 1; ++i) {
        REQUIRE(sr == Processing);
        REQUIRE(pl.currentScope == PSC::StartSeq);

        // let's simulate no response
        ms += MMU2::linkLayerTimeout;
        sr = pl.Step();
    }
    // now we shall report the original error (i.e. ProtocolError instead of CommunicationTimeout)
    REQUIRE(sr == ProtocolError);
    REQUIRE(pl.currentScope == PSC::StartSeq);
}

void SimProtocolErrorStreamOfBadBytes(MMU2::ProtocolLogic &pl) {
    using namespace MMU2;
    ++ms;
    // receive a response
    rxbuff = "Beef\n";
    auto sr = pl.Step();

    // we should end up in a Processing state while having a first occurrence of ProtocolError recorded
    // and the state machine waits for dataLayerTimeout to restart the communication
    for (uint8_t i = 0; i < MMU2::DropOutFilter::maxOccurrences - 1; ++i) {
        REQUIRE(sr == Processing);
        REQUIRE(pl.currentScope == PSC::DelayedRestart);

        // let's simulate the waiting timeout for a restart attempt
        ms += MMU2::heartBeatPeriod;
        sr = pl.Step();

        REQUIRE(sr == Processing);
        REQUIRE(pl.currentScope == PSC::StartSeq);
        REQUIRE(rxbuff.empty());
        REQUIRE_FALSE(txbuffQ.empty());
        REQUIRE(txbuffQ.back() == AppendCRC("S0"));
        // put some bad bytes back into the buffer - should end up in protocol error again
        rxbuff = "Cafe\n";
        sr = pl.Step();
    }
    // now we shall report the original error (i.e. ProtocolError instead of CommunicationTimeout)
    REQUIRE(sr == ProtocolError);
    REQUIRE(pl.currentScope == PSC::DelayedRestart);
    // prepare for a correct init seq test
    ms += MMU2::heartBeatPeriod;
    pl.Step(); // changes to startSeq
}

/// Verify the protocol_logic layer - protocol error
TEST_CASE("Marlin::MMU2::ProtocolLogic ProtocolError", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial, MMU2_TOOL_CHANGE_LOAD_LENGTH, PULLEY_SLOW_FEED_RATE);
    InitCommunication(pl);
    pl.ToolChange(0);
    SimProtocolError(pl);
    InitCommunication2(pl, "T0");
}

TEST_CASE("Marlin::MMU2::ProtocolLogic ProtocolErrorBadByteStream", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial, MMU2_TOOL_CHANGE_LOAD_LENGTH, PULLEY_SLOW_FEED_RATE);
    InitCommunication(pl);
    pl.ToolChange(0);
    SimProtocolErrorStreamOfBadBytes(pl);
    InitCommunication2(pl, "T0");
}

void SimCommandRejected(MMU2::ProtocolLogic &pl) {
    using namespace MMU2;
    ++ms;
    // receive a response
    rxbuff = AppendCRC("T0 R");
    auto sr = pl.Step();

    // we should end up in a CommandRejected state
    // and the state machine immediately tries to retry the command
    REQUIRE(sr == CommandRejected);
    REQUIRE(pl.currentScope == PSC::Command);
}

/// Verify the protocol_logic layer - command rejected
TEST_CASE("Marlin::MMU2::ProtocolLogic CommandRejected", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial, MMU2_TOOL_CHANGE_LOAD_LENGTH, PULLEY_SLOW_FEED_RATE);

    InitCommunication(pl);

    pl.ToolChange(0);

    SimCommandRejected(pl);

    ++ms;

    // receive a response
    rxbuff = AppendCRC("T0 A");
    REQUIRE(pl.Step() == Processing);
}

void SimCommandError(MMU2::ProtocolLogic &pl, std::string cmdRq) {
    using namespace MMU2;
    ++ms;
    txbuffQ.clear();
    // receive a response
    REQUIRE(StepAndCheckState(pl, cmdRq + " A", PST::Running, PSC::Command, Processing, ""));
    ms += MMU2::heartBeatPeriod;

    // query state
    REQUIRE(StepAndCheckState(pl, "", PST::Running, PSC::Command, Processing, "Q0"));

    // simulate 5x error reports
    for (uint8_t i = 0; i < 5; ++i) {
        ++ms;

        // receive a response - Error
        // we should end up in a CommandError state
        // and the state machine is awaiting further instructions from the upper layer
        REQUIRE(StepAndCheckState(pl, cmdRq + " E1", PST::Running, PSC::Command, CommandError, "f0"));
        // report filament sensor
        REQUIRE(StepAndCheckState(pl, "f0 A", PST::Running, PSC::Command, Processing, RegisterReq8(pl, 0)));
        QueryRegisters(pl, PSC::Command, PSC::Command, Processing);

        ms += MMU2::heartBeatPeriod;

        // query state
        REQUIRE(StepAndCheckState(pl, "", PST::Running, PSC::Command, Processing, "Q0"));
    }
    ++ms;
    // receive a finish response - presumably the reported error has been fixed
    REQUIRE(StepAndCheckState(pl, cmdRq + " F0", PST::Running, PSC::Idle, Finished, ""));
}

/// Verify the protocol_logic layer - command error
TEST_CASE("Marlin::MMU2::ProtocolLogic CommandError", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial, MMU2_TOOL_CHANGE_LOAD_LENGTH, PULLEY_SLOW_FEED_RATE);

    InitCommunication(pl);

    // try multiple times
    for (uint8_t i = 0; i < 10; ++i) {
        pl.ToolChange(0);
        SimCommandError(pl, "T0");
    }
}

void SimVersionMismatch(MMU2::ProtocolLogic &pl, int stage) {
    using namespace MMU2;

    pl.Start();
    REQUIRE(StepAndCheckState(pl, "", PST::InitSequence, PSC::StartSeq, Processing, "S0"));

    {
        StepStatus sr;
        for (uint8_t retries = 0; retries < ProtocolLogic::maxRetries; ++retries) {
            rxbuff = AppendCRC(stage == 0 ? "S0 A1" : "S0 A" xstr(supportedMmuFWVersionMajor)); // place a response from the MMU into rxbuff
            sr = pl.Step();                                                                     // response should be processed in one step and another request shall appear in the txbuff
            ++ms;
        }
        REQUIRE(pl.state == PST::InitSequence);
        if (stage == 0) {
            REQUIRE(sr == VersionMismatch);
            return;
        } else {
            REQUIRE(sr == Processing);
            REQUIRE(txbuffQ.back() == AppendCRC("S1"));
            txbuffQ.clear();
        }
    }
    {
        StepStatus sr;
        for (uint8_t retries = 0; retries < ProtocolLogic::maxRetries; ++retries) {
            rxbuff = AppendCRC(stage == 1 ? "S1 Af" : "S1 A" xstr(supportedMmuFWVersionMinor));
            sr = pl.Step();
            ++ms;
        }
        REQUIRE(pl.state == PST::InitSequence);
        if (stage == 1) {
            REQUIRE(sr == VersionMismatch);
            return;
        } else {
            REQUIRE(sr == Processing);
            REQUIRE(txbuffQ.back() == AppendCRC("S2"));
            txbuffQ.clear();
        }
    }
    {
        StepStatus sr;
        for (uint8_t retries = 0; retries < ProtocolLogic::maxRetries; ++retries) {
            rxbuff = AppendCRC(stage == 2 ? "S2 Af" : "S2 A" xstr(supportedMmuFWVersionRevision));
            sr = pl.Step();
            ++ms;
        }
        if (stage == 2) {
            REQUIRE(pl.state == PST::InitSequence);
            REQUIRE(sr == VersionMismatch);
            return;
        } else {
            REQUIRE(sr == Finished);
            REQUIRE(txbuffQ.empty());
        }
    }

    // we have successfully completed the init sequence
    REQUIRE(pl.state == PST::Running);
}

/// Verify the protocol_logic layer - major version mismatch
TEST_CASE("Marlin::MMU2::ProtocolLogic VersionMismatch0", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial, MMU2_TOOL_CHANGE_LOAD_LENGTH, PULLEY_SLOW_FEED_RATE);
    SimVersionMismatch(pl, 0);
}

/// Verify the protocol_logic layer - minor version mismatch
TEST_CASE("Marlin::MMU2::ProtocolLogic VersionMismatch1", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial, MMU2_TOOL_CHANGE_LOAD_LENGTH, PULLEY_SLOW_FEED_RATE);
    SimVersionMismatch(pl, 1);
}

/// Verify the protocol_logic layer - revision version mismatch
TEST_CASE("Marlin::MMU2::ProtocolLogic VersionMismatch2", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial, MMU2_TOOL_CHANGE_LOAD_LENGTH, PULLEY_SLOW_FEED_RATE);
    SimVersionMismatch(pl, 2);
}

//void SendReceiveCheck(MMU2::ProtocolLogic &pl, std::string_view rx, std::string_view expectedTX,
//    MMU2::PST expectedPLS, MMU2::StepStatus expectedSR)
//{
//    txbuffQ.clear(); // pretend we sent the data
//    rxbuff = rx;
//    auto sr = pl.Step();
//    ++ms;

//    // the automaton must retry sending S0
//    REQUIRE(pl.state == expectedPLS);
//    REQUIRE(sr == expectedSR);
//    if( ! expectedTX.empty() ){
//        REQUIRE(txbuffQ.back() == expectedTX);
//    }
//}

void SendReceiveCheckTimeout(MMU2::ProtocolLogic &pl, uint32_t timeout) {
    using namespace MMU2;
    txbuffQ.clear(); // pretend we sent the data
    for (; timeout; --timeout) {
        REQUIRE(pl.state == PST::InitSequence);
        /*auto sr =*/pl.Step();
        //        REQUIRE(sr == );
        ++ms;
    }

    REQUIRE(pl.state == PST::InitSequence);
    //    REQUIRE(sr == expectedSR);
    //    REQUIRE(txbuffQ.back() == expectedTX);
}

// Sim init sequence corrupted and immediate transfer into some command state
void SimInitSeqCorrupted(MMU2::ProtocolLogic &pl) {
    using namespace MMU2;

    pl.Start();
    REQUIRE(pl.state == PST::InitSequence);
    REQUIRE(txbuffQ.back() == AppendCRC("S0"));

    // pretend we get something else (UART dropped 1 byte and merged with previous failed response - which happens on MK3 and nobody knows why)
    StepAndCheckState(pl, "T4 25\nS0 A2\n", PST::InitSequence, PSC::DelayedRestart, Processing, "");

    // this should cause a protocol error and the state machine shall now wait 1x heartbeat timeout and then issue S0 again
    ms += heartBeatPeriod;

    StepAndCheckState(pl, "", PST::InitSequence, PSC::StartSeq, Processing, "S0");

    // now respond correctly
    StepAndCheckState(pl, "S0 A" xstr(supportedMmuFWVersionMajor), PST::InitSequence, PSC::StartSeq, Processing, "S1");

    // now pretend we get a repeated S0 response (which happened as well)
    StepAndCheckState(pl, "S0 A" xstr(supportedMmuFWVersionMajor), PST::InitSequence, PSC::StartSeq, Processing, "S1");

    // now respond correctly
    StepAndCheckState(pl, "S1 A" xstr(supportedMmuFWVersionMinor), PST::InitSequence, PSC::StartSeq, Processing, "S2");

    StepAndCheckState(pl, "S2 A" xstr(supportedMmuFWVersionRevision), PST::InitSequence, PSC::StartSeq, Processing, "S3");

    StepAndCheckState(pl, "S3 A700", PST::InitSequence, PSC::StartSeq, Processing, SetRegister8(pl, 0, MMU2_TOOL_CHANGE_LOAD_LENGTH));
    InitialSetupRegisters(pl);

    // we have successfully completed the init sequence
    StepAndCheckState(pl, "f0 A", PST::Running, PSC::Idle, Processing, "Q0");

    StepAndCheckState(pl, "T0 P6", PST::Running, PSC::Command, Processing, "f0");
    StepAndCheckState(pl, "f0 A", PST::Running, PSC::Command, Processing, RegisterReq8(pl, 0));
    QueryRegisters(pl, PSC::Command, PSC::Command, Processing);
    ms += MMU2::heartBeatPeriod;
    StepAndCheckState(pl, "", PST::Running, PSC::Command, Processing, "Q0");
    ++ms;
    StepAndCheckState(pl, "T0 F", PST::Running, PSC::Idle, Finished, "");

    IdleOperation(pl);
}

TEST_CASE("Marlin::MMU2::ProtocolLogic InitSeqCorrupted", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial, MMU2_TOOL_CHANGE_LOAD_LENGTH, PULLEY_SLOW_FEED_RATE);
    SimInitSeqCorrupted(pl);
}

enum InitStageNext : uint_fast8_t {
    NextStage,
    RepeatSameQuery,
    ProtocolError,
    VersionMismatch
};

void InitSeqPatchStrings(std::string &rx, std::string &tx, uint8_t stage) {
    char stageChar = (char)stage + '0';
    char stage_1Char = stageChar + 1;
    int stageVersionNrs[] = { supportedMmuFWVersionMajor, supportedMmuFWVersionMinor, supportedMmuFWVersionRevision };

    // patch the strings ;)
    std::replace(rx.begin(), rx.end(), 'x', stageChar);   // replace all 'x' with the current stage number
    std::replace(rx.begin(), rx.end(), 'y', stage_1Char); // replace all 'y' with the next stage
    rx = std::regex_replace(rx, std::regex("v"), (stage < 3 ? std::to_string(stageVersionNrs[stage]) : "0"));
    rx = std::regex_replace(rx, std::regex("S4.*"), "Wb A");

    std::replace(tx.begin(), tx.end(), 'x', stageChar); // replace all 'x' with the current stage number
    std::replace(tx.begin(), tx.end(), 'y', stage_1Char);
    tx = std::regex_replace(tx, std::regex("S4"), "Wb 1e"); // there is no stage3 but a 'Wb 1e' follows
}

void CheckInitSeqStage(MMU2::ProtocolLogic &pl) {
    using namespace MMU2;

    struct Stage {
        std::string rx, expectedTX;
        PST expectedPLS;
        PSC expectedPSC;
        MMU2::StepStatus expectedSR;
        InitStageNext expectedResult;
        uint8_t stageDefIndex = 0;
    };

    Stage stageDefs[] = {
        // place errorneous responses first
        Stage { "T4 25\nSx Av\n", "", PST::InitSequence, PSC::DelayedRestart, Processing, InitStageNext::ProtocolError },
        Stage { "Sy Av", "Sx", PST::InitSequence, PSC::StartSeq, Processing, InitStageNext::RepeatSameQuery },
        Stage { "Ty Pv", "Sx", PST::InitSequence, PSC::StartSeq, Processing, InitStageNext::RepeatSameQuery },
        Stage { "Sx Av", "Sy", PST::InitSequence, PSC::StartSeq, Processing, InitStageNext::NextStage }, // the correct response in the stage
    };

    constexpr size_t stageDefsCount = sizeof(stageDefs) / sizeof(Stage);

    // set initial run of all stages to the same def
    Stage stage[4] = { stageDefs[0], stageDefs[0], stageDefs[0], stageDefs[0] };

    for (uint8_t i = 0; i < 4; ++i) {
        InitSeqPatchStrings(stage[i].rx, stage[i].expectedTX, i);
    }

    auto NextStage = [&](uint8_t i) {
        size_t sdi = stage[i].stageDefIndex + 1;
        if (sdi >= stageDefsCount)
            sdi = stageDefsCount - 1; // prevent out-bounds-access
        stage[i] = stageDefs[sdi];    // this assignment clear the stagedefindex
        stage[i].stageDefIndex = sdi; // restore the index to intended value
        InitSeqPatchStrings(stage[i].rx, stage[i].expectedTX, i);
    };

    for (uint8_t i = 0; i < 4; /* nothing */) {
        StepAndCheckState(pl, stage[i].rx, stage[i].expectedPLS, stage[i].expectedPSC, stage[i].expectedSR, stage[i].expectedTX);
        switch (stage[i].expectedResult) {
        case InitStageNext::NextStage:
            ++i; // proceed to next stage
            break;
        case InitStageNext::ProtocolError:
            if (i > 0)
                NextStage(i); // shift the stage to next record for next time
            i = 0;            // returning to the beginning, but pick the next record in stage0
            NextStage(i);

            // handle delayed restart
            ms += heartBeatPeriod;
            pl.Step(); // should move to S0
            break;
        case InitStageNext::RepeatSameQuery: // repeat the same query - pick next stage record
            NextStage(i);
            break;
        case InitStageNext::VersionMismatch: // sequence completed but perform a retest
            pl.Start();
            REQUIRE(pl.state == PST::InitSequence);
            REQUIRE(txbuffQ.back() == AppendCRC("S0"));
        }
    }
}

TEST_CASE("Marlin::MMU2::ProtocolLogic InitSeqCorruptedExtendedTest", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial, MMU2_TOOL_CHANGE_LOAD_LENGTH, PULLEY_SLOW_FEED_RATE);

    // What do we need:
    // correct response - next stage
    // S response, but different index - repeat same query
    // response to something completely different (e.g. T0 P25) - repeat same query
    // malformed response (T0 25) - protocol error, restart querying from S0
    // combine all the scenarios into one test
    pl.Start();
    REQUIRE(StepAndCheckState(pl, "", PST::InitSequence, PSC::StartSeq, Processing, "S0"));

    CheckInitSeqStage(pl);
}

/// Verify the protocol_logic layer - correctly handle an incoming command while waiting for a response from the MMU in Idle state
TEST_CASE("Marlin::MMU2::ProtocolLogic AsyncCommandRqQuery", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial, MMU2_TOOL_CHANGE_LOAD_LENGTH, PULLEY_SLOW_FEED_RATE);
    InitCommunication(pl);

    ms += MMU2::heartBeatPeriod;
    REQUIRE(StepAndCheckState(pl, "", PST::Running, PSC::Idle, Processing, "Q0"));

    // now shoot an async command
    pl.ToolChange(1);
    // the state machine must not change, just record a new request
    REQUIRE(StepAndCheckState(pl, "", PST::Running, PSC::Idle, Processing, ""));

    // add a response
    ++ms;

    // it should process the response and after finishing the message sequence,
    // transfer into a Command state + send the command
    REQUIRE(StepAndCheckState(pl, "X0 F0", PST::Running, PSC::Idle, Processing, RegisterReq8(pl, 0)));
    QueryRegisters(pl, PSC::Idle, PSC::Command, Processing, "T1");
    REQUIRE(StepAndCheckState(pl, "T1 A", PST::Running, PSC::Command, Processing, ""));
}

/// Verify the protocol_logic layer - correctly handle an incoming command while waiting for a response from the MMU in Command state
TEST_CASE("Marlin::MMU2::ProtocolLogic AsyncCommandRqCommand", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial, MMU2_TOOL_CHANGE_LOAD_LENGTH, PULLEY_SLOW_FEED_RATE);
    InitCommunication(pl);

    // start doing a command
    pl.ToolChange(1);

    REQUIRE(StepAndCheckState(pl, "", PST::Running, PSC::Command, Processing, "T1"));
    ++ms;

    // receive a response
    REQUIRE(StepAndCheckState(pl, "T1 A", PST::Running, PSC::Command, Processing, ""));

    ms += MMU2::heartBeatPeriod;
    // query command state
    REQUIRE(StepAndCheckState(pl, "", PST::Running, PSC::Command, Processing, "Q0"));

    ++ms;
    // simulate an error
    REQUIRE(StepAndCheckState(pl, "T1 E32771", PST::Running, PSC::Command, CommandError, "f0"));
    REQUIRE(StepAndCheckState(pl, "f0 A", PST::Running, PSC::Command, Processing, RegisterReq8(pl, 0)));
    QueryRegisters(pl, PSC::Command, PSC::Command, Processing);

    ms += MMU2::heartBeatPeriod;
    // query command state and meanwhile push a button
    REQUIRE(StepAndCheckState(pl, "", PST::Running, PSC::Command, Processing, "Q0"));

    pl.Button(2);

    ++ms;
    REQUIRE(StepAndCheckState(pl, "", PST::Running, PSC::Command, Processing, ""));

    REQUIRE(StepAndCheckState(pl, "T1 E32771", PST::Running, PSC::Command, CommandError, "f0"));
    REQUIRE(StepAndCheckState(pl, "f0 A", PST::Running, PSC::Command, Processing, RegisterReq8(pl, 0)));
    QueryRegisters(pl, PSC::Command, PSC::Command, Processing);
    // the next step shall be our async request
    // @@TODO this is not nice, ideally the B2 request shall be emitted immediately...
    // but it is nothing serious, 1ms delay poses no problem.
    REQUIRE(StepAndCheckState(pl, "", PST::Running, PSC::Command, Processing, "B2"));
    ++ms;
    REQUIRE(StepAndCheckState(pl, "B2 A", PST::Running, PSC::Command, Processing, "f0"));
}

TEST_CASE("Marlin::MMU2::ProtocolLogic AsyncCommandRqCommandReset", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial, MMU2_TOOL_CHANGE_LOAD_LENGTH, PULLEY_SLOW_FEED_RATE);
    InitCommunication(pl);

    ms += MMU2::heartBeatPeriod;
    // query command state and meanwhile push a button
    REQUIRE(StepAndCheckState(pl, "", PST::Running, PSC::Idle, Processing, "Q0"));
    // simulate MMU reset
    REQUIRE(StepAndCheckState(pl, "X0 E8087", PST::Running, PSC::Idle, CommandError, RegisterReq8(pl, 0)));
    // we should be getting an auto-retry button
    pl.Button(1);
    QueryRegisters(pl, PSC::Idle, PSC::Idle, Finished, "B1");
    ++ms;

    // Here was the error - upon reception of B1 A we sent P0 but transferred into Finished.
    // We must remain in Processing...
    REQUIRE(StepAndCheckState(pl, "B1 A", PST::Running, PSC::Idle, Processing, RegisterReq8(pl, 0)));
    QueryRegisters(pl);
}

/// Verify the protocol_logic layer - recovery of a running command after a communication dropout
TEST_CASE("Marlin::MMU2::ProtocolLogic Recover Command", "[Marlin][MMU2]") {
    using namespace MMU2;
    // @@TODO
}

TEST_CASE("Marlin::MMU2::ProtocolLogic repeated comm link attempts", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial, MMU2_TOOL_CHANGE_LOAD_LENGTH, PULLEY_SLOW_FEED_RATE);

    rxbuff.clear();
    txbuffQ.clear();

    pl.Start();
    REQUIRE(txbuffQ.back() == AppendCRC("S0"));
    pl.Step();

    for (uint8_t i = 0; i < 10; ++i) {
        txbuffQ.clear(); // pretend we sent the data
        ms += MMU2::linkLayerTimeout;
        pl.Step();
        REQUIRE(txbuffQ.back() == AppendCRC("S0"));
    }
}

TEST_CASE("Marlin::MMU2::ProtocolLogic Previous command finished while a new one issued", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial, MMU2_TOOL_CHANGE_LOAD_LENGTH, PULLEY_SLOW_FEED_RATE);
    InitCommunication(pl);

    // start doing a command
    pl.ToolChange(1);

    REQUIRE(StepAndCheckState(pl, "", PST::Running, PSC::Command, Processing, "T1"));
    ++ms;

    // receive a response
    REQUIRE(StepAndCheckState(pl, "T1 A", PST::Running, PSC::Command, Processing, ""));
    ms += MMU2::heartBeatPeriod;
    REQUIRE(StepAndCheckState(pl, "", PST::Running, PSC::Command, Processing, "Q0"));
    ++ms;
    // T1 finished
    REQUIRE(StepAndCheckState(pl, "T1 F", PST::Running, PSC::Idle, Finished, ""));
    ms += MMU2::heartBeatPeriod;
    REQUIRE(StepAndCheckState(pl, "", PST::Running, PSC::Idle, Processing, "Q0"));
    REQUIRE(StepAndCheckState(pl, "T1 F", PST::Running, PSC::Idle, Processing, RegisterReq8(pl, 0)));
    QueryRegisters(pl);
    ms += MMU2::heartBeatPeriod;
    REQUIRE(StepAndCheckState(pl, "", PST::Running, PSC::Idle, Processing, "Q0"));

    pl.ToolChange(0); // now issue another command

    // do a couple of steps just as if the response to Q0 still hasn't arrived
    for (uint8_t i = 0; i < 10; ++i) {
        ++ms;
        REQUIRE(StepAndCheckState(pl, "", PST::Running, PSC::Idle, Processing, ""));
    }

    // MMU finally responded
    REQUIRE(StepAndCheckState(pl, "T1 F", PST::Running, PSC::Idle, Processing, RegisterReq8(pl, 0)));
    ++ms;
    // Previously, this has caused the caller to think we have finished the planned command!
    // Now fixed and the transition from Idle to Command is seamless and we remain in the Processing state
    // It should post the T0 command right away.
    QueryRegisters(pl, PSC::Idle, PSC::Command, Processing, "T0");
    // continues as a normal command
    REQUIRE(StepAndCheckState(pl, "T0 A", PST::Running, PSC::Command, Processing, ""));
}

TEST_CASE("Marlin::MMU2::ProtocolLogic ReadRegister", "[Marlin][MMU2]") {
    // At this moment ReadRegister can only occur in Idle mode.
    // It is not a limitation of the protocol but a safety precaution of the higher level of G-codes processing.
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial, MMU2_TOOL_CHANGE_LOAD_LENGTH, PULLEY_SLOW_FEED_RATE);
    InitCommunication(pl);
    IdleOperation(pl);

    // it should plan a generic request and serve it accordingly
    pl.ReadRegister(0);
    REQUIRE(!txbuffQ.empty());
    REQUIRE(txbuffQ[0] == AppendCRC("R0"));
    txbuffQ.clear();
    // step again and receive some response
    REQUIRE(StepAndCheckState(pl, "R0 A2", PST::Running, PSC::Idle, Finished, ""));

    IdleOperation(pl);
}

TEST_CASE("Marlin::MMU2::ProtocolLogic WriteRegister", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial, MMU2_TOOL_CHANGE_LOAD_LENGTH, PULLEY_SLOW_FEED_RATE);
    InitCommunication(pl);
    IdleOperation(pl);

    // it should plan a generic request and serve it accordingly
    pl.WriteRegister(9, 10);
    REQUIRE(!txbuffQ.empty());
    REQUIRE(txbuffQ[0] == AppendCRCWrite("W9 a"));
    txbuffQ.clear();
    // step again and receive some response
    REQUIRE(StepAndCheckState(pl, "W9 A", PST::Running, PSC::Idle, Finished, ""));

    IdleOperation(pl);
}

TEST_CASE("Marlin::MMU2::ProtocolLogic Interrupted", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial, MMU2_TOOL_CHANGE_LOAD_LENGTH, PULLEY_SLOW_FEED_RATE);
    InitCommunication(pl);
    IdleOperation(pl);

    pl.ToolChange(0);
    std::string cmdRq("T0");

    REQUIRE_FALSE(txbuffQ.empty());
    REQUIRE(txbuffQ.back() == AppendCRC(cmdRq));
    txbuffQ.clear();
    ++ms;
    REQUIRE(StepAndCheckState(pl, cmdRq + " A", PST::Running, PSC::Command, Processing, ""));
    ms += MMU2::heartBeatPeriod;
    REQUIRE(StepAndCheckState(pl, "", PST::Running, PSC::Command, Processing, "Q0"));
    ++ms;
    REQUIRE(StepAndCheckState(pl, cmdRq + " P1", PST::Running, PSC::Command, Processing, "f0"));
    REQUIRE(StepAndCheckState(pl, "f0 A", PST::Running, PSC::Command, Processing, RegisterReq8(pl, 0)));
    QueryRegisters(pl, PSC::Command, PSC::Command, Processing);
    ms += MMU2::heartBeatPeriod;
    StepAndCheckState(pl, "", PST::Running, PSC::Command, Processing, "Q0");

    // cause a timeout
    ms += MMU2::dataLayerTimeout * 2;
    REQUIRE(StepAndCheckState(pl, "", PST::InitSequence, PSC::StartSeq, Processing, "S0"));
    StepAndCheckState(pl, "S0 A" xstr(supportedMmuFWVersionMajor), PST::InitSequence, PSC::StartSeq, Processing, "S1");
    ++ms;
    StepAndCheckState(pl, "S1 A" xstr(supportedMmuFWVersionMinor), PST::InitSequence, PSC::StartSeq, Processing, "S2");
    ++ms;
    StepAndCheckState(pl, "S2 A" xstr(supportedMmuFWVersionRevision), PST::InitSequence, PSC::StartSeq, Processing, "S3");
    ++ms;
    StepAndCheckState(pl, "S3 A" xstr(supportedMmuFWVersionBuild), PST::InitSequence, PSC::StartSeq, Processing, SetRegister8(pl, 0, MMU2_TOOL_CHANGE_LOAD_LENGTH));
    InitialSetupRegisters(pl);
    ++ms;
    StepAndCheckState(pl, "f0 A0", PST::Running, PSC::Idle, Processing, "Q0");
    ++ms;
    StepAndCheckState(pl, "X0 F0", PST::Running, PSC::Idle, Interrupted, "");

    // even though "interrupted" the state machine should continue querying the MMU like usually
    ms += MMU2::heartBeatPeriod;
    StepAndCheckState(pl, "", PST::Running, PSC::Idle, Processing, "Q0");
}

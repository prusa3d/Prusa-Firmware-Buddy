#include "catch2/catch.hpp"

#include "protocol_logic.h"
#include "mmu2_fsensor.h"

#include <deque>

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

bool StepAndCheckState(MMU2::ProtocolLogic &pl, MMU2::ProtocolLogic::State plState, MMU2::StepStatus stepStatus, const char *txContent) {
    auto sr = pl.Step();
    CHECKED_ELSE(pl.state == plState) {
        return false;
    }
    CHECKED_ELSE(sr == stepStatus) {
        return false;
    }
    CHECKED_ELSE(txbuffQ.back() == txContent) {
        return false;
    }

    txbuffQ.clear();
    return true;
}

bool StepAndCheckState2(MMU2::ProtocolLogic &pl, MMU2::ProtocolLogic::State plState, MMU2::StepStatus stepStatus, const char *txContent0, const char *txContent1) {
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
    CHECKED_ELSE(txbuffQ[0] == txContent0) {
        return false;
    }
    CHECKED_ELSE(txbuffQ[1] == txContent1) {
        return false;
    }

    txbuffQ.clear();
    return true;
}

bool StepAndCheckStateEmptyTX(MMU2::ProtocolLogic &pl, MMU2::ProtocolLogic::State plState, MMU2::StepStatus stepStatus) {
    auto sr = pl.Step();
    CHECKED_ELSE(pl.state == plState) {
        return false;
    }
    CHECKED_ELSE(sr == stepStatus) {
        return false;
    }
    CHECKED_ELSE(txbuffQ.empty()) {
        return false;
    }

    txbuffQ.clear();
    return true;
}

void InitCommunication2(MMU2::ProtocolLogic &pl);

void InitCommunication(MMU2::ProtocolLogic &pl) {
    using namespace MMU2;

    rxbuff.clear();
    txbuffQ.clear();

    // as it starts, the communication shall be disabled
    REQUIRE(pl.state == ProtocolLogic::State::Stopped);

    // stepping the state machine should keep it in Stopped state
    // and step result shall be ??? Processing?
    for (uint8_t i = 0; i < 10; ++i) {
        REQUIRE(StepAndCheckStateEmptyTX(pl, ProtocolLogic::State::Stopped, Processing));
    }

    // enable the state machine
    pl.Start();
    REQUIRE(pl.state == ProtocolLogic::State::InitSequence);
    REQUIRE(txbuffQ.back() == "S0\n");
    txbuffQ.clear(); // pretend we sent the data

    // step the machine a couple of times, waiting for response from the MMU
    for (uint8_t i = 0; i < 10; ++i) {
        REQUIRE(StepAndCheckStateEmptyTX(pl, ProtocolLogic::State::InitSequence, Processing));
        ++ms; // pretend one millisecond elapsed
    }

    InitCommunication2(pl);
}

void InitCommunication2(MMU2::ProtocolLogic &pl) {
    using namespace MMU2;
    // perform the init sequence
    {
        rxbuff = "S0 A2\n"; // place a response from the MMU into rxbuff
        StepAndCheckState(pl, ProtocolLogic::State::InitSequence, Processing, "S1\n");
        ++ms;
    }
    {
        rxbuff = "S1 A0\n";
        StepAndCheckState(pl, ProtocolLogic::State::InitSequence, Processing, "S2\n");
        ++ms;
    }
    {
        rxbuff = "S2 A0\n";
        StepAndCheckState(pl, ProtocolLogic::State::InitSequence, Processing, "f0\n");
        ++ms;
    }
    {
        rxbuff = "f0 A0\n";
        StepAndCheckStateEmptyTX(pl, ProtocolLogic::State::Running, Finished);
        ++ms;
    }
    // we have successfully completed the init sequence
}

void IdleOperation(MMU2::ProtocolLogic &pl) {
    using namespace MMU2;
    // check for repeated queries while idle (protocol heartbeat)
    {
        ms += MMU2::heartBeatPeriod;
        REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "Q0\n"));

        rxbuff = "X0 F\n";
        // FINDA query + response follows immediately
        REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "P0\n"));
        rxbuff = "P0 A0\n";
        REQUIRE(StepAndCheckStateEmptyTX(pl, ProtocolLogic::State::Running, Finished));

        ms += MMU2::heartBeatPeriod;

        // try several times more with longer timing
        for (int i = 0; i < 10; ++i) {
            REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "Q0\n"));

            // receive a response after 100ms - should reset the uart timeout
            ms += MMU2::heartBeatPeriod / 3;
            rxbuff = "X0 F\n"; // prepare a response
            // FINDA query + response follows immediately
            REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "P0\n"));
            rxbuff = "P0 A0\n";
            REQUIRE(StepAndCheckStateEmptyTX(pl, ProtocolLogic::State::Running, Finished));

            // waiting + not sending anything
            ms += MMU2::heartBeatPeriod / 3;
            REQUIRE(StepAndCheckStateEmptyTX(pl, ProtocolLogic::State::Running, Finished));

            // waiting + not sending anything
            ms += MMU2::heartBeatPeriod / 3;
            REQUIRE(StepAndCheckStateEmptyTX(pl, ProtocolLogic::State::Running, Finished));

            ms += MMU2::heartBeatPeriod / 3 + 3; // avoid rounding errors
        }
    }
}

void CommandOperation(MMU2::ProtocolLogic &pl, std::string cmdRq) {
    using namespace MMU2;

    REQUIRE(txbuffQ.back() == cmdRq + "\n");
    txbuffQ.clear();

    ++ms;

    // receive a response
    rxbuff = cmdRq + " A\n";
    REQUIRE(StepAndCheckStateEmptyTX(pl, ProtocolLogic::State::Running, Processing));

    ms += MMU2::heartBeatPeriod;

    // query command state
    REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "Q0\n"));

    ++ms;

    // receive a response and report fsensor state immediately
    rxbuff = cmdRq + " P1\n";
    REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "f0\n"));

    // receive a response and ask for FINDA status immediately
    rxbuff = "f0 A\n";
    REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "P0\n"));

    // receive FINDA status
    rxbuff = "P0 A0\n";
    REQUIRE(StepAndCheckStateEmptyTX(pl, ProtocolLogic::State::Running, Processing));

    ms += MMU2::heartBeatPeriod;

    // query state
    REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "Q0\n"));

    ++ms;

    // receive a finish response
    rxbuff = cmdRq + " F\n";
    REQUIRE(StepAndCheckStateEmptyTX(pl, ProtocolLogic::State::Running, Finished));
}

/// Verify the protocol_logic layer - sitting idle
TEST_CASE("Marlin::MMU2::ProtocolLogic Idle", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial);

    InitCommunication(pl);
    IdleOperation(pl);
}

/// Verify the protocol_logic layer - command
TEST_CASE("Marlin::MMU2::ProtocolLogic Command", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial);

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
    auto sr = pl.Step();

    // we should end up in a Processing state while having a first occurrence of CommunicationTimeout recorded
    // and the state machine immediately tries to start the communication again by sending the InitSequence.
    for (uint8_t i = 0; i < MMU2::DropOutFilter::maxOccurrences - 1; ++i) {
        REQUIRE(sr == Processing);
        REQUIRE(pl.currentState == &pl.startSeq);

        // let's simulate no response
        ms += MMU2::linkLayerTimeout;
        sr = pl.Step();
    }
    // now we shall report the error
    REQUIRE(sr == CommunicationTimeout);
    REQUIRE(pl.currentState == &pl.startSeq);
}

/// Verify the protocol_logic layer - timeouts waiting for a response msg
TEST_CASE("Marlin::MMU2::ProtocolLogic TimeoutResponse", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial);

    InitCommunication(pl);

    // verify timeout multiple times
    for (uint8_t i = 0; i < 10; ++i) {
        pl.ToolChange(0);
        ResponseTimeout(pl);
        InitCommunication2(pl);
    }
}

void SimProtocolError(MMU2::ProtocolLogic &pl) {
    using namespace MMU2;
    ++ms;
    // receive a response
    rxbuff = "DeadBeef\n";
    auto sr = pl.Step();

    // we should end up in a Processing state while having a first occurrence of ProtocolError recorded
    // and the state machine immediately tries to start the communication again by sending the InitSequence.
    for (uint8_t i = 0; i < MMU2::DropOutFilter::maxOccurrences - 1; ++i) {
        REQUIRE(sr == Processing);
        REQUIRE(pl.currentState == &pl.startSeq);

        // let's simulate no response
        ms += MMU2::linkLayerTimeout;
        sr = pl.Step();
    }
    // now we shall report the error
    REQUIRE(sr == ProtocolError);
    REQUIRE(pl.currentState == &pl.startSeq);
}

/// Verify the protocol_logic layer - protocol error
TEST_CASE("Marlin::MMU2::ProtocolLogic ProtocolError", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial);

    InitCommunication(pl);

    pl.ToolChange(0);

    SimProtocolError(pl);

    InitCommunication2(pl);
}

void SimCommandRejected(MMU2::ProtocolLogic &pl) {
    using namespace MMU2;
    ++ms;
    // receive a response
    rxbuff = "T0 R\n";
    auto sr = pl.Step();

    // we should end up in a CommandRejected state
    // and the state machine immediately tries to retry the command
    REQUIRE(sr == CommandRejected);
    REQUIRE(pl.currentState == &pl.command);
}

/// Verify the protocol_logic layer - command rejected
TEST_CASE("Marlin::MMU2::ProtocolLogic CommandRejected", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial);

    InitCommunication(pl);

    pl.ToolChange(0);

    SimCommandRejected(pl);

    ++ms;

    // receive a response
    rxbuff = "T0 A\n";
    REQUIRE(pl.Step() == Processing);
}

void SimCommandError(MMU2::ProtocolLogic &pl, std::string cmdRq) {
    using namespace MMU2;
    ++ms;
    // receive a response
    rxbuff = cmdRq + " A\n";
    REQUIRE(pl.Step() == Processing);

    ms += MMU2::heartBeatPeriod;

    // query state
    auto sr = pl.Step();
    REQUIRE(txbuffQ.back() == "Q0\n");

    // simulate 5x error reports
    for (uint8_t i = 0; i < 5; ++i) {
        ++ms;

        // receive a response - Error
        rxbuff = cmdRq + " E1\n";
        // we should end up in a CommandError state
        // and the state machine is awaiting further instructions from the upper layer
        sr = pl.Step();
        REQUIRE(pl.currentState == &pl.command);
        REQUIRE(sr == CommandError);

        // report filament sensor
        sr = pl.Step();
        REQUIRE(txbuffQ.back() == "f0\n");
        REQUIRE(sr == Processing);

        // verify response and request FINDA state
        rxbuff = "f0 A\n";
        sr = pl.Step();
        REQUIRE(txbuffQ.back() == "P0\n");
        REQUIRE(sr == Processing);

        // verify response and query state
        rxbuff = "P0 A0\n";
        sr = pl.Step();
        REQUIRE(sr == Processing);

        ms += MMU2::heartBeatPeriod;

        // query state
        auto sr = pl.Step();
        REQUIRE(txbuffQ.back() == "Q0\n");
    }

    ++ms;

    // receive a finish response - presumably the reported error has been fixed
    rxbuff = cmdRq + " F\n";
    sr = pl.Step();
    REQUIRE(sr == Finished);
}

/// Verify the protocol_logic layer - command error
TEST_CASE("Marlin::MMU2::ProtocolLogic CommandError", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial);

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
    REQUIRE(pl.state == ProtocolLogic::State::InitSequence);
    REQUIRE(txbuffQ.back() == "S0\n");
    txbuffQ.clear(); // pretend we sent the data

    {
        rxbuff = stage == 0 ? "S0 A1\n" : "S0 A2\n"; // place a response from the MMU into rxbuff
        auto sr = pl.Step();                         // response should be processed in one step and another request shall appear in the txbuff
        ++ms;
        if (stage == 0) {
            REQUIRE(pl.state == ProtocolLogic::State::Stopped);
            REQUIRE(sr == VersionMismatch);
            return;
        } else {
            REQUIRE(pl.state == ProtocolLogic::State::InitSequence);
            REQUIRE(sr == Processing);
            REQUIRE(txbuffQ.back() == "S1\n");
            txbuffQ.clear();
        }
    }
    {
        rxbuff = stage == 1 ? "S1 A1\n" : "S1 A0\n";
        auto sr = pl.Step();
        ++ms;
        if (stage == 1) {
            REQUIRE(pl.state == ProtocolLogic::State::Stopped);
            REQUIRE(sr == VersionMismatch);
            return;
        } else {
            REQUIRE(pl.state == ProtocolLogic::State::InitSequence);
            REQUIRE(sr == Processing);
            REQUIRE(txbuffQ.back() == "S2\n");
            txbuffQ.clear();
        }
    }
    {
        rxbuff = stage == 2 ? "S2 A1\n" : "S2 A0\n";
        auto sr = pl.Step();
        ++ms;
        if (stage == 2) {
            REQUIRE(pl.state == ProtocolLogic::State::Stopped);
            REQUIRE(sr == VersionMismatch);
            return;
        } else {
            REQUIRE(sr == Finished);
            REQUIRE(txbuffQ.empty());
        }
    }

    // we have successfully completed the init sequence
    REQUIRE(pl.state == ProtocolLogic::State::Running);
}

/// Verify the protocol_logic layer - major version mismatch
TEST_CASE("Marlin::MMU2::ProtocolLogic VersionMismatch0", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial);
    SimVersionMismatch(pl, 0);
}

/// Verify the protocol_logic layer - minor version mismatch
TEST_CASE("Marlin::MMU2::ProtocolLogic VersionMismatch1", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial);
    SimVersionMismatch(pl, 1);
}

/// Verify the protocol_logic layer - revision version mismatch
TEST_CASE("Marlin::MMU2::ProtocolLogic VersionMismatch2", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial);
    SimVersionMismatch(pl, 2);
}

/// Verify the protocol_logic layer - correctly handle an incoming command while waiting for a response from the MMU in Idle state
TEST_CASE("Marlin::MMU2::ProtocolLogic AsyncCommandRqQuery", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial);
    InitCommunication(pl);

    ms += MMU2::heartBeatPeriod;
    REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "Q0\n"));

    // now shoot an async command
    pl.ToolChange(1);
    // the state machine must not change, just record a new request
    REQUIRE(StepAndCheckStateEmptyTX(pl, ProtocolLogic::State::Running, Processing));

    // add a response
    rxbuff = "X0 F\n";
    ++ms;

    // it should process the response and after finishing the message sequence,
    // transfer into a Command state + send the command
    REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "P0\n"));
    rxbuff = "P0 A1\n";
    REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "T1\n"));

    rxbuff = "T1 A\n";
    REQUIRE(StepAndCheckStateEmptyTX(pl, ProtocolLogic::State::Running, Processing));
}

/// Verify the protocol_logic layer - correctly handle an incoming command while waiting for a response from the MMU in Command state
TEST_CASE("Marlin::MMU2::ProtocolLogic AsyncCommandRqCommand", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial);
    InitCommunication(pl);

    // start doing a command
    pl.ToolChange(1);

    REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "T1\n"));
    ++ms;

    // receive a response
    rxbuff = "T1 A\n";
    REQUIRE(StepAndCheckStateEmptyTX(pl, ProtocolLogic::State::Running, Processing));

    ms += MMU2::heartBeatPeriod;
    // query command state
    REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "Q0\n"));

    ++ms;
    rxbuff = "T1 E32771\n"; // simulate an error
    REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, CommandError, "f0\n"));
    rxbuff = "f0 A\n";
    REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "P0\n"));
    rxbuff = "P0 A1\n";
    REQUIRE(StepAndCheckStateEmptyTX(pl, ProtocolLogic::State::Running, Processing));

    ms += MMU2::heartBeatPeriod;
    // query command state and meanwhile push a button
    REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "Q0\n"));

    pl.Button(2);

    ++ms;
    REQUIRE(StepAndCheckStateEmptyTX(pl, ProtocolLogic::State::Running, Processing));

    rxbuff = "T1 E32771\n";
    REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, CommandError, "f0\n"));
    rxbuff = "f0 A\n";
    REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "P0\n"));
    rxbuff = "P0 A1\n";
    REQUIRE(StepAndCheckStateEmptyTX(pl, ProtocolLogic::State::Running, Processing));
    // the next step shall be our async request
    // @@TODO this is not nice, ideally the B2 request shall be emitted immediately...
    // but it is nothing serious, 1ms delay poses no problem.
    REQUIRE(StepAndCheckState2(pl, ProtocolLogic::State::Running, Processing, "B2\n", "Q0\n"));

    // add a response
    rxbuff = "T1 P1\n";
    ++ms;
    REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "f0\n"));
}

/// Verify the protocol_logic layer - recovery of a running command after a communication dropout
TEST_CASE("Marlin::MMU2::ProtocolLogic Recover Command", "[Marlin][MMU2]") {
    using namespace MMU2;
    // @@TODO
}

TEST_CASE("Marlin::MMU2::ProtocolLogic repeated comm link attempts", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial);

    rxbuff.clear();
    txbuffQ.clear();

    pl.Start();
    REQUIRE(txbuffQ.back() == "S0\n");
    pl.Step();

    for (uint8_t i = 0; i < 10; ++i) {
        txbuffQ.clear(); // pretend we sent the data
        ms += MMU2::linkLayerTimeout;
        pl.Step();
        REQUIRE(txbuffQ.back() == "S0\n");
    }
}

TEST_CASE("Marlin::MMU2::ProtocolLogic Previous command finished while a new one issued", "[Marlin][MMU2]") {
    using namespace MMU2;
    ProtocolLogic pl(&mmu2Serial);
    InitCommunication(pl);

    // start doing a command
    pl.ToolChange(1);

    REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "T1\n"));
    ++ms;

    // receive a response
    rxbuff = "T1 A\n";
    REQUIRE(StepAndCheckStateEmptyTX(pl, ProtocolLogic::State::Running, Processing));

    ms += MMU2::heartBeatPeriod;
    REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "Q0\n"));

    ++ms;
    rxbuff = "T1 F\n"; // T1 finished
    REQUIRE(StepAndCheckStateEmptyTX(pl, ProtocolLogic::State::Running, Finished));

    ms += MMU2::heartBeatPeriod;
    REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "Q0\n"));
    rxbuff = "T1 F\n";
    REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "P0\n"));
    rxbuff = "P0 A1\n";
    REQUIRE(StepAndCheckStateEmptyTX(pl, ProtocolLogic::State::Running, Finished));
    ms += MMU2::heartBeatPeriod;
    REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "Q0\n"));

    pl.ToolChange(0); // now issue another command

    // do a couple of steps just as if the response to Q0 still hasn't arrived
    for (uint8_t i = 0; i < 10; ++i) {
        ++ms;
        REQUIRE(StepAndCheckStateEmptyTX(pl, ProtocolLogic::State::Running, Processing));
    }

    rxbuff = "T1 F\n"; // MMU finally responded
    REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "P0\n"));
    ++ms;
    rxbuff = "P0 A1\n";
    // Previously, this has caused the caller to think we have finished the planned command!
    // Now fixed and the transition from Idle to Command is seamless and we remain in the Processing state
    // It should post the T0 command right away.
    REQUIRE(StepAndCheckState(pl, ProtocolLogic::State::Running, Processing, "T0\n"));

    // receive a response
    rxbuff = "T0 A\n";
    // continues as a normal command
    REQUIRE(StepAndCheckStateEmptyTX(pl, ProtocolLogic::State::Running, Processing));
}

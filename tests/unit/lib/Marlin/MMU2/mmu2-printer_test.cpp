#include "catch2/catch.hpp"

#include <mmu2_mk4.h>
#include <inttypes.h>
#include "stubs/stub_interfaces.h"

void SimulateCommStart(MMU2::MMU2 &mmu) {
    // what appeared on the mmu-serial
    CHECK(mmu2SerialSim.TxBuffMatchesCRC("S0"));

    // mmu response on the mmu-serial
    mmu2SerialSim.SetRxBuffCRC("S0 A2");
    mmu2SerialSim.txbuffQ.clear();
    mmu.mmu_loop();
    mmu2SerialSim.SetRxBuffCRC("S1 A1");
    mmu2SerialSim.txbuffQ.clear();
    mmu.mmu_loop();
    mmu2SerialSim.SetRxBuffCRC("S2 A9");
    mmu2SerialSim.txbuffQ.clear();
    mmu.mmu_loop();
    mmu2SerialSim.SetRxBuffCRC("S3 A345");
    mmu2SerialSim.txbuffQ.clear();
    mmu.mmu_loop();
    mmu2SerialSim.SetRxBuffCRC("Wb A");
    mmu2SerialSim.txbuffQ.clear();
    mmu.mmu_loop();
    mmu2SerialSim.SetRxBuffCRC("W14 A");
    mmu2SerialSim.txbuffQ.clear();
    mmu.mmu_loop();
    mmu2SerialSim.SetRxBuffCRC("f0 A");
    mmu2SerialSim.txbuffQ.clear();
    mmu.mmu_loop();
}

constexpr std::string ToHex(uint16_t c) {
    char tmp[16];
    snprintf(tmp, sizeof(tmp), "%x", c);
    return std::string(tmp);
}

constexpr std::string FormatReportProgressHook(char command, ProgressCode pc) {
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "ReportProgressHook(%c, %" PRIu16 ")", command, (uint16_t)pc);
    return std::string(tmp);
}

constexpr std::string FormatReportErrorHook(char command, ErrorCode pc) {
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "ReportErrorHook(%c, %" PRIu16 ")", command, (uint16_t)pc);
    return std::string(tmp);
}

constexpr std::string FormatBeginReport(char command) {
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "BeginReport(%c, 1)", command);
    return std::string(tmp);
}

constexpr IOSimRec MakeQueryResponseProgress(const char *command, ProgressCode pc, IOSimRec::WorkFunc w = nullptr) {
    return { "Q0", { FormatReportProgressHook(command[0], pc) }, {}, std::string(command) + " P" + ToHex((uint16_t)pc), 1, w };
}

constexpr IOSimRec MakeQueryResponseError(const char *command, ErrorCode pc, IOSimRec::WorkFunc w = nullptr) {
    return { "Q0", { "IncrementMMUFails", "ButtonAvailable", FormatReportErrorHook(command[0], pc) }, {}, std::string(command) + " E" + ToHex((uint16_t)pc), 1, w };
}

constexpr IOSimRec MakeQueryResponseErrorButton(const char *command, ErrorCode pc, uint16_t button, IOSimRec::WorkFunc w = nullptr) {
    return { "Q0", { FormatReportErrorHook(command[0], pc) }, {}, std::string(command) + " B" + ToHex(button), 1, w };
}

constexpr IOSimRec MakeAcceptButton(uint16_t button, IOSimRec::WorkFunc w = nullptr) {
    return { std::string("B") + ToHex(button), {}, {}, std::string("B") + ToHex(button) + " A", 1, w };
}

constexpr IOSimRec MakeQueryResponseFinished(const char *command, IOSimRec::WorkFunc w = nullptr) {
    return { "Q0", {}, {}, std::string(command) + " F0", 1, w };
}

constexpr IOSimRec MakeFSensorAccepted(uint8_t state, IOSimRec::WorkFunc w = nullptr) {
    if (state) {
        return { "f1", {}, {}, "f1 A", 1, w };
    } else {
        return { "f0", {}, {}, "f0 A", 1, w };
    }
}

constexpr IOSimRec MakeQueryResponseReadRegister(uint8_t addr, uint16_t value, uint32_t timeout = 1, IOSimRec::WorkFunc w = nullptr) {
    return { std::string("R") + ToHex(addr), {}, {}, std::string("R") + ToHex(addr) + " A" + ToHex((uint16_t)value), timeout, w };
}

constexpr IOSimRec MakeCommandAccepted(const char *command, const char *fullScreenMsg, IOSimRec::WorkFunc w = nullptr) {
    return { command, { fullScreenMsg, FormatBeginReport(command[0]) }, { "" }, std::string(command) + " A1", MMU2::heartBeatPeriod + 1, w };
}

uint8_t selectorSlot = 5, idlerSlot = 5;

#define MakeRegistersQuery(finda, selector, idler, errors, pulley, timeout) \
    MakeQueryResponseReadRegister(0x08, finda),                             \
        MakeQueryResponseReadRegister(0x1b, selector),                      \
        MakeQueryResponseReadRegister(0x1c, idler),                         \
        MakeQueryResponseReadRegister(0x04, errors),                        \
        MakeQueryResponseReadRegister(0x1a, pulley, timeout)

#define MakeRegistersCommand(fsensor, finda, selector, idler, errors, pulley, timeout) \
    MakeFSensorAccepted(fsensor),                                                      \
        MakeQueryResponseReadRegister(0x08, finda),                                    \
        MakeQueryResponseReadRegister(0x1b, selector),                                 \
        MakeQueryResponseReadRegister(0x1c, idler),                                    \
        MakeQueryResponseReadRegister(0x04, errors),                                   \
        MakeQueryResponseReadRegister(0x1a, pulley, timeout)

void SimulateQuery(MMU2::MMU2 &mmu) {
    mockLog.Clear();
    IOSimStart({
        { "Q0", {}, {}, "X0 F0", 1 },
        MakeRegistersQuery(0, 5, 5, 0, 0, 1),
    });

    while (ioSimI != ioSim.cend()) {
        mmu.mmu_loop();
    }
}

TEST_CASE("Marlin::MMU2::MMU2 start", "[Marlin][MMU2][.]") {
    InitEnvironment();
    MMU2::MMU2 mmu;
    mmu.Start();
    // functions that were mandatory to execute
    CHECK(mockLog.Matches({ "MMU2Serial::begin", "power_on", "MMU2Serial::flush" }));

    // what appeared in the serial log
    CHECK(marlinLogSim.log.empty());

    // mmu automaton internal state
    CHECK(mmu.State() == MMU2::xState::Connecting);

    SimulateCommStart(mmu);

    // query
    SimulateQuery(mmu);
}

void MMU2StartTimeoutStep(MMU2::MMU2 &mmu) {
    IncMillis(MMU2::linkLayerTimeout);
    mmu.mmu_loop();
    CHECK(mmu2SerialSim.TxBuffMatchesCRC("S0"));
    mmu2SerialSim.txbuffQ.clear();
}

TEST_CASE("Marlin::MMU2::MMU2 start timeout", "[Marlin][MMU2][.]") {
    InitEnvironment();
    MMU2::MMU2 mmu;
    mmu.Start();

    mockLog.Clear();
    for (size_t i = 0; i < MMU2::DropOutFilter::maxOccurrences - 1; ++i) {
        MMU2StartTimeoutStep(mmu);
        REQUIRE(mockLog.log.size() == i + 1U);
    }
    // the last step should report an error - which has been broken before the unit tests btw. :)
    MMU2StartTimeoutStep(mmu);

    // we should have an error screen reported
    REQUIRE(mockLog.log.size() >= MMU2::DropOutFilter::maxOccurrences + 1); // @@TODO currently, there is a minor bug that the ReportErrorHook gets called 2x
    // @@TODO we have a problem - BeginReport hasn't been called (it should have, especially on MK4)
    CHECK(mockLog.log.back() == "ReportErrorHook(0, 31814)");

    // it should continue trying though
    MMU2StartTimeoutStep(mmu);
}

TEST_CASE("Marlin::MMU2::MMU2 preload", "[Marlin][MMU2][.]") {
    InitEnvironment();
    MMU2::mmu2.Start();
    SimulateCommStart(MMU2::mmu2);
    SimulateQuery(MMU2::mmu2);

    // issue a Preload command
    // since this is a blocking call, we need to prepare all the stuff around it
    IOSimStart({
        MakeCommandAccepted("L0", "FullScreenMsgLoad"),
        MakeQueryResponseProgress("L0", ProgressCode::EngagingIdler),
        MakeRegistersCommand(0, 0, 5, 5, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress("L0", ProgressCode::FeedingToFinda),
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress("L0", ProgressCode::RetractingFromFinda),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseFinished("L0"),
    });
    MMU2::mmu2.load_filament(0);
    INFO("finished");
}

TEST_CASE("Marlin::MMU2::MMU2 unload", "[Marlin][MMU2][.]") {
    InitEnvironment();
    MMU2::mmu2.Start();
    SimulateCommStart(MMU2::mmu2);
    SimulateQuery(MMU2::mmu2);

    // issue a Preload command
    // since this is a blocking call, we need to prepare all the stuff around it
    IOSimStart({
        MakeCommandAccepted("U0", "FullScreenMsgLoad"),
        MakeQueryResponseProgress("U0", ProgressCode::EngagingIdler),
        MakeRegistersCommand(1, 1, 5, 5, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress("U0", ProgressCode::UnloadingToFinda),
        MakeRegistersCommand(1, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress("U0", ProgressCode::UnloadingToFinda),
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress("U0", ProgressCode::RetractingFromFinda),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress("U0", ProgressCode::DisengagingIdler),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseFinished("U0"),
    });
    MMU2::mmu2.unload();
    INFO("finished");
}

TEST_CASE("Marlin::MMU2::MMU2 unload failed", "[Marlin][MMU2]") {
    InitEnvironment();
    MMU2::fs = MMU2::FilamentState::AT_FSENSOR;
    MMU2::mmu2.Start();
    SimulateCommStart(MMU2::mmu2);
    SimulateQuery(MMU2::mmu2);

    // issue a Preload command
    // since this is a blocking call, we need to prepare all the stuff around it
    IOSimStart({
        MakeCommandAccepted("U0", "MakeSound"),
        MakeQueryResponseProgress("U0", ProgressCode::EngagingIdler),
        MakeRegistersQuery(1, 5, 5, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress("U0", ProgressCode::UnloadingToFinda, []() { MMU2::fs = MMU2::FilamentState::NOT_PRESENT; }),
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress("U0", ProgressCode::UnloadingToFinda),
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress("U0", ProgressCode::ERRDisengagingIdler),
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseError("U0", ErrorCode::FINDA_DIDNT_SWITCH_OFF),
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),

        // press middle button and resolve the error - we can fake it through MMU communication
        MakeQueryResponseErrorButton("U0", ErrorCode::FINDA_DIDNT_SWITCH_OFF, 1),
        // after button gets received from the MMU, registers are queried
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        // next, the button gets sent back to the MMU - need to ack-it
        MakeAcceptButton(1),
        // after the button, registers are queried again
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),

        MakeQueryResponseProgress("U0", ProgressCode::ERREngagingIdler),
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),

        // let it finish normally
        MakeQueryResponseProgress("U0", ProgressCode::UnloadingToFinda),
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress("U0", ProgressCode::RetractingFromFinda),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress("U0", ProgressCode::DisengagingIdler),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseFinished("U0"),
    });
    // @@TODO bugs:
    // - ReportErrorHook is being called 3x after the error occurs
    // - Unload calls MakeSound before processing the request - do we want that?

    // nutno doresit
    // - error conversion, at mame komplet prevod chyb na texty
    // - marlin log - at makra logujou, at vidime, co se sype na
    MMU2::mmu2.unload();

    CHECK(std::equal(mockLog.log.cbegin(), mockLog.log.cend(), mockLog.expected.cbegin()));

    INFO("finished");
}

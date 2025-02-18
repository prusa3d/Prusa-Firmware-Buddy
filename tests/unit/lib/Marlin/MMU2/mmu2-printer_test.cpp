#include "catch2/catch.hpp"

#include <mmu2_mk4.h>
#include <inttypes.h>
#include "mmu2_supported_version.h"
#include "stubs/stub_interfaces.h"

// @@TODO bugs:
// [ ] ReportErrorHook is being called 3x after the error occurs and even more often while the error is active
//     - ideally, the error hook shall be called only upon change
//     - but - MK3 abuses the error hooks for keeping the UI responsive
// [ ] Unload calls MakeSound before processing the request - do we want that?
// [x] Retry via a Button makes some strange call sequence:
//     - MMU button pushed (OK)
//     - Cooldown flag cleared (OK)
//     - Button (OK)
//     - Cooling timer stopped (OK)
//     - Saving and parking (WTF?) - it's because of a default buttonpressed handling - need an ugly hack to prevent this
//     - Heater cooldown pending (WTF?)
//     - Cooling timeout started (WTF?)
//
// TODO:
// [ ] heating in general - Check heating timeout - if the recovery heats up and proceeds correctly
// [ ] Autoretry procedure (should be the same like an ordinary button
// [ ] Command error
// [ ] Communication error + recovery

// workaround for unsupported constexpr constructors for basic_string and other STL containers in gcc < 12.x
#if __GNUC_PREREQ(12, 0)
// If  gcc_version >= 12.0
    #define constexpr_workaround constexpr
#else
    #define constexpr_workaround /* */
#endif

void SimulateCommStart(MMU2::MMU2 &mmu) {
    mmu.Start();
    // functions that were mandatory to execute
    REQUIRE(mockLog.Matches({ "SetHotendTargetTemp(0)", "SetHotendCurrentTemp(0)", "MMU2Serial::begin", "power_on", "MMU2Serial::flush" }));

    // make sure the calls get into the "expected" sequence for later evaluation
    // it's a bit of a hack but these records must happen every time the MMU is starting
    mockLog.expected = mockLog.log;
    CHECK(mmu.State() == MMU2::xState::Connecting);

    // what appeared on the mmu-serial
    CHECK(mmu2SerialSim.TxBuffMatchesCRC("S0"));

    // mmu response on the mmu-serial
    mmu2SerialSim.SetRxBuffCRC("S0 A3");
    static_assert(MMU2::mmuVersionMajor == 3);
    mmu2SerialSim.txbuffQ.clear();
    mmu.mmu_loop();
    mmu2SerialSim.SetRxBuffCRC("S1 A0");
    static_assert(MMU2::mmuVersionMinor == 0);
    mmu2SerialSim.txbuffQ.clear();
    mmu.mmu_loop();
    mmu2SerialSim.SetRxBuffCRC("S2 A3");
    static_assert(MMU2::mmuVersionPatch == 3);
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

constexpr_workaround std::string ToHex(uint16_t c) {
    char tmp[16];
    snprintf(tmp, sizeof(tmp), "%x", c);
    return std::string(tmp);
}

constexpr_workaround std::string FormatReportProgressHook(char command, ProgressCode pc) {
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "ReportProgressHook(%c, %" PRIu16 ")", command, (uint16_t)pc);
    return std::string(tmp);
}

constexpr_workaround std::string FormatReportErrorHook(char command, ErrorCode pc) {
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "ReportErrorHook(%c, %" PRIu16 ")", command, (uint16_t)pc);
    return std::string(tmp);
}

constexpr_workaround std::string FormatBeginReport(char command) {
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "BeginReport(%c, 1)", command);
    return std::string(tmp);
}

constexpr_workaround std::string FormatEndReport(char command) {
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "EndReport(%c, 0)", command);
    return std::string(tmp);
}

constexpr_workaround IOSimRec MakeQueryResponseProgressM(const char *command, ProgressCode pc, std::vector<std::string> s, IOSimRec::WorkFunc w = nullptr) {
    return { "Q0", s, {}, std::string(command) + " P" + ToHex((uint16_t)pc), 1, w };
}

constexpr_workaround IOSimRec MakeQueryResponseProgress(const char *command, ProgressCode pc, IOSimRec::WorkFunc w = nullptr) {
    //    return { "Q0", { FormatReportProgressHook(command[0], pc) }, {}, std::string(command) + " P" + ToHex((uint16_t)pc), 1, w };
    return MakeQueryResponseProgressM(command, pc, { FormatReportProgressHook(command[0], pc) }, w);
}

constexpr_workaround IOSimRec MakeLogEntry(const char *logEntry) {
    return { .mock = { logEntry } };
}

constexpr_workaround IOSimRec MakeCommandWithoutBeginReport(const char *command, IOSimRec::WorkFunc w = nullptr) {
    return { command, {}, { "" }, std::string(command) + " A1", MMU2::heartBeatPeriod + 1, w };
}

constexpr_workaround IOSimRec MakeQueryResponseError(const char *command, ErrorCode pc, IOSimRec::WorkFunc w = nullptr) {
    return { "Q0", { "IncrementMMUFails", "ButtonAvailable", FormatReportErrorHook(command[0], pc) }, {}, std::string(command) + " E" + ToHex((uint16_t)pc), 1, w };
}

constexpr_workaround IOSimRec MakeQueryResponseErrorWithMaintenance(const char *command, ErrorCode pc, IOSimRec::WorkFunc w = nullptr) {
    return { "Q0", { "TrackMaintenance", "IncrementMMUFails", "ButtonAvailable", FormatReportErrorHook(command[0], pc) }, {}, std::string(command) + " E" + ToHex((uint16_t)pc), 1, w };
}

constexpr_workaround IOSimRec MakeQueryResponseErrorNoIncMMUFails(const char *command, ErrorCode pc, IOSimRec::WorkFunc w = nullptr) {
    return { "Q0", { "ButtonAvailable", FormatReportErrorHook(command[0], pc) }, {}, std::string(command) + " E" + ToHex((uint16_t)pc), 1, w };
}

constexpr_workaround IOSimRec MakeQueryResponseErrorButton(const char *command, ErrorCode pc, uint16_t button, IOSimRec::WorkFunc w = nullptr) {
    return { "Q0", { FormatReportErrorHook(command[0], pc) }, {}, std::string(command) + " B" + ToHex(button), 1, w };
}

constexpr_workaround IOSimRec MakeAcceptButton(uint16_t button, IOSimRec::WorkFunc w = nullptr) {
    return { std::string("B") + ToHex(button), {}, {}, std::string("B") + ToHex(button) + " A", 1, w };
}

constexpr_workaround IOSimRec MakeQueryResponseFinished(const char *command, IOSimRec::WorkFunc w = nullptr) {
    return { "Q0", { "MakeSound", FormatEndReport(command[0]), "ScreenUpdateEnable" }, {}, std::string(command) + " F0", 1, w };
}

constexpr_workaround IOSimRec MakeQueryResponseToolChangeFinished(const char *command, IOSimRec::WorkFunc w = nullptr) {
    return { "Q0", { "TryLoadUnloadReporter::TryLoadUnloadReporter", "planner_any_moves", "TryLoadUnloadReporter::DumpToSerial", "IncrementMMUChanges", FormatEndReport(command[0]) }, {}, std::string(command) + " F0", 1, w };
}

constexpr_workaround IOSimRec MakeFSensorAccepted(uint8_t state, IOSimRec::WorkFunc w = nullptr) {
    if (state) {
        return { "f1", {}, {}, "f1 A", 1, w };
    } else {
        return { "f0", {}, {}, "f0 A", 1, w };
    }
}

constexpr_workaround IOSimRec MakeQueryResponseReadRegister(uint8_t addr, uint16_t value, uint32_t timeout = 1, IOSimRec::WorkFunc w = nullptr) {
    return { std::string("R") + ToHex(addr), {}, {}, std::string("R") + ToHex(addr) + " A" + ToHex((uint16_t)value), timeout, w };
}

constexpr_workaround IOSimRec MakeCommandAccepted(const char *command, const char *fullScreenMsg, IOSimRec::WorkFunc w = nullptr) {
    return { command, { fullScreenMsg, FormatBeginReport(command[0]) }, { "" }, std::string(command) + " A1", MMU2::heartBeatPeriod + 1, w };
}

constexpr_workaround IOSimRec MakeCommandAccepted(const char *command, IOSimRec::WorkFunc w = nullptr) {
    return { command, { FormatBeginReport(command[0]) }, { "" }, std::string(command) + " A1", MMU2::heartBeatPeriod + 1, w };
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

#define MakeInitialQuery(timeout) \
    { "Q0", {}, {}, "X0 F0", 1 }, \
        MakeRegistersQuery(0, 5, 5, 0, 0, timeout)

void SimulateQuery(MMU2::MMU2 &mmu) {
    IOSimStart({ MakeInitialQuery(1) });

    while (ioSimI != ioSim.cend()) {
        mmu.mmu_loop();
    }
}

TEST_CASE("Marlin::MMU2::MMU2 start", "[Marlin][MMU2]") {
    InitEnvironment(MMU2::FilamentState::NOT_PRESENT);
    MMU2::MMU2 mmu;
    SimulateCommStart(mmu);
    SimulateQuery(mmu);
}

void MMU2StartTimeoutStep(MMU2::MMU2 &mmu) {
    IncMillis(MMU2::linkLayerTimeout);
    mmu.mmu_loop();
    CHECK(mmu2SerialSim.TxBuffMatchesCRC("S0"));
    mmu2SerialSim.txbuffQ.clear();
}

TEST_CASE("Marlin::MMU2::MMU2 start timeout", "[Marlin][MMU2]") {
    InitEnvironment(MMU2::FilamentState::NOT_PRESENT);
    MMU2::MMU2 mmu;
    mmu.Start();

    mockLog.Clear();
    for (size_t i = 0; i < MMU2::DropOutFilter::maxOccurrences - 1; ++i) {
        MMU2StartTimeoutStep(mmu);
        mockLog.expected.push_back("MMU2Serial::flush");
        REQUIRE(mockLog.log.size() == i + 1U);
    }
    // the last step should report an error - which has been broken before the unit tests btw. :)
    MMU2StartTimeoutStep(mmu);
    mockLog.expected.push_back("ButtonAvailable");
    static constexpr char reh[] = "ReportErrorHook(x, 32814)";
    mockLog.expected.push_back(reh);
    mockLog.expected.push_back(reh); // @TODO deduplicate error reporting

    // we should have an error screen reported
    REQUIRE(mockLog.log.size() >= MMU2::DropOutFilter::maxOccurrences + 1);
    // Bugs:
    // [ ] We have a problem - BeginReport hasn't been called (it should have, especially on MK4)
    // [ ] Currently, there is a minor bug that the ReportErrorHook gets called 2x (or even worse - many times in some scenarios)

    // Temporarily, deduplicate mockLog.log and expected until error reporting gets fixed (on 8bit primarily!)
    mockLog.DeduplicateLog();
    mockLog.DeduplicateExpected();

    CHECK(mockLog.MatchesExpected());

    // it should continue trying though
    MMU2StartTimeoutStep(mmu);
}

TEST_CASE("Marlin::MMU2::MMU2 preload", "[Marlin][MMU2]") {
    InitEnvironment(MMU2::FilamentState::NOT_PRESENT);
    SimulateCommStart(MMU2::mmu2);

    // issue a Preload command
    // since this is a blocking call, we need to prepare all the stuff around it
    static constexpr char cmd[] = "L0";
    IOSimStart({
        MakeInitialQuery(1),

        MakeCommandAccepted(cmd, "FullScreenMsgLoad"),
        MakeQueryResponseProgress(cmd, ProgressCode::EngagingIdler),
        MakeRegistersCommand(0, 0, 5, 5, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::FeedingToFinda),
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::RetractingFromFinda),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseFinished(cmd),
    });
    MMU2::mmu2.load_filament(0);
    CHECK(mockLog.MatchesExpected());
}

TEST_CASE("Marlin::MMU2::MMU2 unload", "[Marlin][MMU2]") {
    InitEnvironment(MMU2::FilamentState::AT_FSENSOR);
    SimulateCommStart(MMU2::mmu2);
    static constexpr char cmd[] = "U0";
    IOSimStart({
        MakeInitialQuery(1),

        MakeCommandAccepted(cmd),
        MakeLogEntry("MakeSound"),

        MakeQueryResponseProgress(cmd, ProgressCode::EngagingIdler),
        MakeRegistersCommand(1, 1, 5, 5, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::UnloadingToFinda),
        MakeRegistersCommand(1, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::UnloadingToFinda, []() { MMU2::fs = MMU2::FilamentState::NOT_PRESENT; }),
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::RetractingFromFinda),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::DisengagingIdler),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseFinished(cmd),
    });
    MMU2::mmu2.unload();
    // ReportErrorHook would have been called multiple times if the code didn't have a protection against it
    // -> remove duplicit consecutive records in the expected sequence ;)
    mockLog.DeduplicateExpected();
    mockLog.DeduplicateLog();
    CHECK(mockLog.MatchesExpected());
}

TEST_CASE("Marlin::MMU2::MMU2 unload failed", "[Marlin][MMU2]") {
    InitEnvironment(MMU2::FilamentState::AT_FSENSOR);
    SimulateCommStart(MMU2::mmu2);

    static constexpr char cmd[] = "U0";
    IOSimStart({
        MakeInitialQuery(1),

        MakeCommandAccepted(cmd),
        MakeLogEntry("MakeSound"),

        MakeQueryResponseProgress(cmd, ProgressCode::EngagingIdler),
        MakeRegistersQuery(1, 5, 5, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::UnloadingToFinda, []() { MMU2::fs = MMU2::FilamentState::NOT_PRESENT; }),
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::UnloadingToFinda),
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::ERRDisengagingIdler),
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseErrorWithMaintenance(cmd, ErrorCode::FINDA_DIDNT_SWITCH_OFF),
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),

        // press middle button and resolve the error - we can fake it through MMU communication
        MakeQueryResponseErrorButton(cmd, ErrorCode::FINDA_DIDNT_SWITCH_OFF, 1),
        // after button gets received from the MMU, registers are queried
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        // next, the button gets sent back to the MMU - need to ack-it
        MakeAcceptButton(1),
        // after the button, registers are queried again
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),

        MakeQueryResponseProgress(cmd, ProgressCode::ERREngagingIdler),
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),

        // let it finish normally
        MakeQueryResponseProgress(cmd, ProgressCode::UnloadingToFinda),
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::RetractingFromFinda),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::DisengagingIdler),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseFinished(cmd),
    });
    MMU2::mmu2.unload();
    mockLog.DeduplicateLog();
    mockLog.DeduplicateExpected();
    INFO(mockLog.InfoText());
    CHECK(mockLog.MatchesExpected());
}

TEST_CASE("Marlin::MMU2::MMU2 cut", "[Marlin][MMU2]") {
    InitEnvironment(MMU2::FilamentState::NOT_PRESENT);
    SimulateCommStart(MMU2::mmu2);

    static constexpr char cmd[] = "K0";
    IOSimStart({
        MakeInitialQuery(1),

        MakeCommandAccepted(cmd, "FullScreenMsgCut"),
        MakeQueryResponseProgress(cmd, ProgressCode::SelectingFilamentSlot),
        MakeRegistersCommand(0, 0, 5, 5, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::FeedingToFinda),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::RetractingFromFinda),
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::PreparingBlade),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::PushingFilament),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::DisengagingIdler),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::PerformingCut),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::Homing),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::ReturningSelector),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseFinished(cmd),
    });
    MMU2::mmu2.cut_filament(0, true);
    mockLog.DeduplicateExpected();
    mockLog.DeduplicateLog();
    CHECK(mockLog.MatchesExpected());
}

TEST_CASE("Marlin::MMU2::MMU2 eject", "[Marlin][MMU2]") {
    InitEnvironment(MMU2::FilamentState::NOT_PRESENT);
    SimulateCommStart(MMU2::mmu2);

    static constexpr char cmd[] = "E0";
    IOSimStart({
        MakeInitialQuery(1),

        MakeCommandAccepted(cmd, "FullScreenMsgEject"),
        MakeQueryResponseProgress(cmd, ProgressCode::ParkingSelector),
        MakeRegistersCommand(0, 0, 5, 5, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::EngagingIdler),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::EjectingFilament),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::ERRDisengagingIdler),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),

        MakeQueryResponseErrorNoIncMMUFails(cmd, ErrorCode::FILAMENT_EJECTED),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),

        // press middle button - filament eject has been completed
        MakeQueryResponseErrorButton(cmd, ErrorCode::FILAMENT_EJECTED, 1),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeAcceptButton(1),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),

        MakeQueryResponseFinished(cmd),
    });
    MMU2::mmu2.eject_filament(0, true);
    mockLog.DeduplicateExpected();
    mockLog.DeduplicateLog();
    CHECK(mockLog.MatchesExpected());
}

TEST_CASE("Marlin::MMU2::MMU2 toolchange", "[Marlin][MMU2]") {
    InitEnvironment(MMU2::FilamentState::NOT_PRESENT);
    SimulateCommStart(MMU2::mmu2);

    // @@TODO need to setup the "extruder" index in mmu2 - that's also a bug ;)
    SetMarlinIsPrinting(true);

    static constexpr char cmd[] = "T0";
    IOSimStart({
        MakeInitialQuery(1),

        MakeCommandAccepted(cmd),
        MakeQueryResponseProgress(cmd, ProgressCode::EngagingIdler),
        MakeRegistersCommand(0, 0, 5, 5, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::FeedingToFinda),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::FeedingToBondtech),
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::FeedingToFSensor, []() { MMU2::fs = MMU2::FilamentState::AT_FSENSOR; }),
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::FeedingToNozzle),
        MakeRegistersCommand(1, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::DisengagingIdler),
        MakeRegistersCommand(1, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),

        // several finished records
        MakeQueryResponseToolChangeFinished(cmd),
        MakeRegistersCommand(1, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseFinished(cmd),
        MakeRegistersCommand(1, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseFinished(cmd),
        MakeRegistersCommand(1, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
    });
    MMU2::mmu2.tool_change(0);
    mockLog.DeduplicateExpected();
    mockLog.DeduplicateLog();
    CHECK(mockLog.MatchesExpected()); // @@TODO tryunload sequence is missing
}

TEST_CASE("Marlin::MMU2::MMU2 unload with preheat", "[Marlin][MMU2]") {
    InitEnvironment(MMU2::FilamentState::AT_FSENSOR);
    SimulateCommStart(MMU2::mmu2);

    SetHotendTargetTemp(215);
    mockLog.expected.push_back("SetHotendTargetTemp(215)");
    static constexpr char cmd[] = "U0";
    IOSimStart({
        MakeInitialQuery(MMU2::heartBeatPeriod + 1),

        MakeLogEntry("BeginReport(U, 1)"),

        // heat up POC
        MakeLogEntry("ReportProgressHook(U, 37)"),
        MakeQueryResponseProgressM("X0", ProgressCode::OK, { "SetHotendCurrentTemp(50)" }, []() { SetHotendCurrentTemp(50); }),
        MakeRegistersQuery(1, 5, 5, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeLogEntry("ReportProgressHook(U, 37)"),
        MakeQueryResponseProgressM("X0", ProgressCode::OK, { "SetHotendCurrentTemp(100)" }, []() { SetHotendCurrentTemp(100); }),
        MakeRegistersQuery(1, 5, 5, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeLogEntry("ReportProgressHook(U, 37)"),
        MakeQueryResponseProgressM("X0", ProgressCode::OK, { "SetHotendCurrentTemp(150)" }, []() { SetHotendCurrentTemp(150); }),
        MakeRegistersQuery(1, 5, 5, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeLogEntry("ReportProgressHook(U, 37)"),
        MakeQueryResponseProgressM("X0", ProgressCode::OK, { "SetHotendCurrentTemp(200)" }, []() { SetHotendCurrentTemp(200); }),
        MakeRegistersQuery(1, 5, 5, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeLogEntry("ReportProgressHook(U, 37)"),
        MakeQueryResponseProgressM("X0", ProgressCode::OK, { "SetHotendCurrentTemp(215)" }, []() { SetHotendCurrentTemp(215); }),
        MakeRegistersQuery(1, 5, 5, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeLogEntry("ReportProgressHook(U, 37)"),

        MakeCommandWithoutBeginReport(cmd),
        MakeLogEntry("MakeSound"),

        //        MakeRegistersCommand(1, 1, 5, 5, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::EngagingIdler),
        MakeRegistersCommand(1, 1, 5, 5, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::UnloadingToFinda),
        MakeRegistersCommand(1, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::UnloadingToFinda, []() { MMU2::fs = MMU2::FilamentState::NOT_PRESENT; }),
        MakeRegistersCommand(0, 1, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::RetractingFromFinda),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseProgress(cmd, ProgressCode::DisengagingIdler),
        MakeRegistersCommand(0, 0, 0, 0, 0, 0, MMU2::heartBeatPeriod + 1),
        MakeQueryResponseFinished(cmd),
    });
    MMU2::mmu2.unload();
    // ReportErrorHook would have been called multiple times if the code didn't have a protection against it
    // -> remove duplicit consecutive records in the expected sequence ;)
    mockLog.DeduplicateExpected();
    mockLog.DeduplicateLog();
    CHECK(mockLog.MatchesExpected());
}

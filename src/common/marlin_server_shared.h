#pragma once

#include "cmsis_os.h" // for osThreadId
#include "utils/utility_extensions.hpp"
#include <limits>

namespace marlin_server {

// usr8 in variant8_t message contains id (bit0..6) and event flag (bit7)
inline constexpr uint8_t MARLIN_USR8_MSK_ID = 0x7f; // usr8 - event id mask

inline constexpr uint32_t TIME_TO_END_INVALID = std::numeric_limits<uint32_t>::max();
inline constexpr time_t TIMESTAMP_INVALID = std::numeric_limits<time_t>::max();

inline constexpr uint8_t CURRENT_TOOL = std::numeric_limits<uint8_t>::max();

enum class State {
    Idle,
    WaitGui,
    PrintPreviewInit, ///< Print is being initialized
    PrintPreviewImage, ///< Showing print preview and waiting for user to click print
    PrintPreviewConfirmed, ///< Print is confirmed to be printed (either user clicked print, or WUI/Connect started print without confirmation on printer)
    PrintPreviewQuestions, ///< Some problems with print detected, ask user to skip/fix them
    PrintPreviewToolsMapping, ///< Waiting for user to do the tool mapping/spool join
    PrintInit,
    SerialPrintInit,
    Printing,
    Pausing_Begin,
    Pausing_Failed_Code,
    Pausing_WaitIdle,
    Pausing_ParkHead,
    Paused,
    Resuming_Begin,
    Resuming_Reheating,
    Resuming_UnparkHead_XY,
    Resuming_UnparkHead_ZE,
    Aborting_Begin,
    Aborting_WaitIdle,
    Aborting_ParkHead,
    Aborting_Preview, ///< Print preview can do filament load/unload and needs to be aborted properly
    Aborted,
    Finishing_WaitIdle,
    Finishing_ParkHead,
    Finished,
    Exit, // sets idle, notifies clients to close fsm
    CrashRecovery_Begin,
    CrashRecovery_Retracting,
    CrashRecovery_Lifting,
    CrashRecovery_ToolchangePowerPanic, // Prepare for toolchange after power panic
    CrashRecovery_XY_Measure,
    CrashRecovery_Tool_Pickup,
    CrashRecovery_XY_HOME,
    CrashRecovery_HOMEFAIL, // Shows retry button after homing fails
    CrashRecovery_Axis_NOK,
    CrashRecovery_Repeated_Crash,
    PowerPanic_acFault,
    PowerPanic_Resume,
    PowerPanic_AwaitingResume,
};

/// Marlin client -> server messages
enum class Msg : char {
    EventMask = 'A',
    _RESERVED_0 = 'B',
    Stop = 'C',
    _RESERVED_3 = 'D',
    Start = 'E',
    Gcode = 'F',
    InjectGcode = 'G',
    SetVariable = 'H',
    _RESERVED_1 = 'I',
    Babystep = 'J',
    ConfigSave = 'K',
    ConfigLoad = 'L',
    ConfigReset = 'M',
    _RESERVED_2 = 'N',
    _RESERVED_4 = 'O',
    TestStart = 'P',
    TestAbort = 'Q',
    PrintStart = 'R',
    PrintAbort = 'S',
    PrintPause = 'T',
    PrintResume = 'U',
    PrintExit = 'V',
    Park = 'W',
    KnobMove = 'X',
    KnobClick = 'Y',
    FSM = 'Z',
    Move = 'a',
    PrintReady = 'b',
    GuiCantPrint = 'c',
    CancelObjectID = 'd',
    UncancelObjectID = 'e',
    CancelCurrentObject = 'f',
};

inline bool is_abort_state(State st) {
    return ftrstd::to_underlying(st) >= ftrstd::to_underlying(State::Aborting_Begin) && ftrstd::to_underlying(st) <= ftrstd::to_underlying(State::Aborted);
}

// converts message's ID to string
// string must have 3 bytes at least
void marlin_msg_to_str(const Msg id, char *str);

extern osThreadId server_task; // task of marlin server

} // namespace marlin_server

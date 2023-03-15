#pragma once

#include "cmsis_os.h" // for osThreadId
#include <limits>

// usr8 in variant8_t message contains id (bit0..6) and event flag (bit7)
inline constexpr uint8_t MARLIN_USR8_MSK_ID = 0x7f; // usr8 - event id mask

inline constexpr uint32_t TIME_TO_END_INVALID = std::numeric_limits<uint32_t>::max();

inline constexpr uint8_t MARLIN_SERVER_CURRENT_TOOL = std::numeric_limits<uint8_t>::max();

typedef enum {
    mpsIdle = 0,
    mpsWaitGui,
    mpsPrintPreviewInit,
    mpsPrintPreviewImage,
    mpsPrintPreviewQuestions,
    mpsPrintInit,
    mpsPrinting,
    mpsPausing_Begin,
    mpsPausing_Failed_Code,
    mpsPausing_WaitIdle,
    mpsPausing_ParkHead,
    mpsPaused,
    mpsResuming_Begin,
    mpsResuming_Reheating,
    mpsResuming_UnparkHead_XY,
    mpsResuming_UnparkHead_ZE,
    mpsAborting_Begin,
    mpsAborting_WaitIdle,
    mpsAborting_ParkHead,
    mpsAborted,
    mpsFinishing_WaitIdle,
    mpsFinishing_ParkHead,
    mpsFinished,
    mpsExit, // sets idle, notifies clients to close fsm
    mpsCrashRecovery_Begin,
    mpsCrashRecovery_Retracting,
    mpsCrashRecovery_Lifting,
    mpsCrashRecovery_XY_Measure,
    mpsCrashRecovery_Tool_Pickup,
    mpsCrashRecovery_XY_HOME,
    mpsCrashRecovery_Axis_NOK,
    mpsCrashRecovery_Repeated_Crash,
    mpsPowerPanic_acFault,
    mpsPowerPanic_Resume,
    mpsPowerPanic_AwaitingResume,
} marlin_print_state_t;

/// Marlin client -> server messages
typedef enum {
    MARLIN_MSG_EVENT_MASK = 'A',
    _RESERVED_0 = 'B',
    MARLIN_MSG_STOP = 'C',
    _RESERVED_3 = 'D',
    MARLIN_MSG_START = 'E',
    MARLIN_MSG_GCODE = 'F',
    MARLIN_MSG_INJECT_GCODE = 'G',
    MARLIN_MSG_SET_VARIABLE = 'H',
    _RESERVED_1 = 'I',
    MARLIN_MSG_BABYSTEP = 'J',
    MARLIN_MSG_CONFIG_SAVE = 'K',
    MARLIN_MSG_CONFIG_LOAD = 'L',
    MARLIN_MSG_CONFIG_RESET = 'M',
    _RESERVED_2 = 'N',
    _RESERVED_4 = 'O',
    MARLIN_MSG_TEST_START = 'P',
    MARLIN_MSG_TEST_ABORT = 'Q',
    MARLIN_MSG_PRINT_START = 'R',
    MARLIN_MSG_PRINT_ABORT = 'S',
    MARLIN_MSG_PRINT_PAUSE = 'T',
    MARLIN_MSG_PRINT_RESUME = 'U',
    MARLIN_MSG_PRINT_EXIT = 'V',
    MARLIN_MSG_PARK = 'W',
    MARLIN_MSG_KNOB_MOVE = 'X',
    MARLIN_MSG_KNOB_CLICK = 'Y',
    MARLIN_MSG_FSM = 'Z',
    MARLIN_MSG_MOVE = 'a',
    MARLIN_MSG_GUI_PRINT_READY = 'b',
    MARLIN_MSG_GUI_CANT_PRINT = 'c',
} marlin_msg_t;

inline int is_abort_state(marlin_print_state_t st) {
    return ((int)st) >= ((int)mpsAborting_Begin) && ((int)st) <= ((int)mpsAborted);
}

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

// converts message's ID to string
// string must have 3 bytes at least
extern void marlin_msg_to_str(const marlin_msg_t id, char *str);

extern osThreadId marlin_server_task; // task of marlin server

#ifdef __cplusplus
}
#endif //__cplusplus

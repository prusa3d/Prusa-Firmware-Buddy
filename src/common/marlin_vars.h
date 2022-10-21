// marlin_vars.h
#pragma once

#include "variant8.h"
#include "../../src/gui/file_list_defs.h"

// Marlin variables
typedef enum {
    MARLIN_VAR_MOTION,              // R:  uint8, method stepper.axis_is_moving
    MARLIN_VAR_GQUEUE,              // R:  uint8, method queue.length
    MARLIN_VAR_PQUEUE,              // R:  uint8, variables planner.block_buffer_head/tail
    MARLIN_VAR_IPOS_X,              // RW: int32, variable stepper.count_position
    MARLIN_VAR_IPOS_Y,              // RW: ==||==
    MARLIN_VAR_IPOS_Z,              // RW: ==||==
    MARLIN_VAR_IPOS_E,              // RW: ==||==
    MARLIN_VAR_POS_X,               // RW: float, planner.getAxisPosition_mm(), setAxisPosition_mm()
    MARLIN_VAR_POS_Y,               // RW: ==||==
    MARLIN_VAR_POS_Z,               // RW: ==||==
    MARLIN_VAR_POS_E,               // RW: ==||==
    MARLIN_VAR_TEMP_NOZ,            // R:  float, thermalManager.temp_hotend[0].current
    MARLIN_VAR_TEMP_BED,            // R:  float, thermalManager.temp_bed.current
    MARLIN_VAR_TTEM_NOZ,            // RW: float, thermalManager.temp_hotend[0].target, thermalManager.setTargetHotend()
    MARLIN_VAR_TTEM_BED,            // RW: float, thermalManager.temp_bed.target, thermalManager.setTargetBed()
    MARLIN_VAR_Z_OFFSET,            // R:  float, zprobe_zoffset
    MARLIN_VAR_FANSPEED,            // RW: uint8, thermalManager.fan_speed[0], thermalManager.set_fan_speed()
    MARLIN_VAR_PRNSPEED,            // RW: uint16, feedrate_percentage
    MARLIN_VAR_FLOWFACT,            // RW: uint16, planner.flow_percentage
    MARLIN_VAR_WAITHEAT,            // RW: bool, Marlin, wait_for_heatup
    MARLIN_VAR_WAITUSER,            // RW: bool, Marlin, wait_for_user
    MARLIN_VAR_SD_PDONE,            // R:  uint8, card.percentDone()
    MARLIN_VAR_DURATION,            // R:  uint32, print_job_timer.duration()
    MARLIN_VAR_MEDIAINS,            // R:  bool, media_is_inserted()
    MARLIN_VAR_PRNSTATE,            // R:  marlin_print_state_t, marlin_server.print_state
    MARLIN_VAR_FILENAME,            // R:  char*,
    MARLIN_VAR_FILEPATH,            // R:  char*,
    MARLIN_VAR_DTEM_NOZ,            // R:  float, nozzle temperature to display
    MARLIN_VAR_TIMTOEND,            // R:  uint32, oProgressData.oTime2End.mGetValue() or -1 if not valid
    MARLIN_VAR_PRINT_FAN_RPM,       // R:  uint16, fanCtlPrint.getActualRPM()
    MARLIN_VAR_HEATBREAK_FAN_RPM,   // R:  uint16, fanCtlHeatBreak.getActualRPM()
    MARLIN_VAR_FAN_CHECK_ENABLED,   // RW: bool, fan_check
    MARLIN_VAR_ENDSTOPS,            // R: endstops state
    MARLIN_VAR_FS_AUTOLOAD_ENABLED, // RW: bool, fs_autoload
    MARLIN_VAR_JOB_ID,              // RW: uint16_t job id incremented at every print start(for connect)
    MARLIN_VAR_CURR_POS_X,          // R: float current_position
    MARLIN_VAR_CURR_POS_Y,          // R: ==||==
    MARLIN_VAR_CURR_POS_Z,          // R: ==||==
    MARLIN_VAR_CURR_POS_E,          // R: ==||==
    MARLIN_VAR_TRAVEL_ACCEL,        // R: float travel_acceleration
    MARLIN_VAR_MAX = MARLIN_VAR_TRAVEL_ACCEL
} marlin_var_id_t;

// variable masks
#define MARLIN_VAR_MSK(v_id)                                     (((uint64_t)1) << (uint8_t)(v_id))
#define MARLIN_VAR_MSK2(id1, id2)                                (MARLIN_VAR_MSK(id1) | MARLIN_VAR_MSK(id2))
#define MARLIN_VAR_MSK3(id1, id2, id3)                           (MARLIN_VAR_MSK2(id1, id2) | MARLIN_VAR_MSK(id3))
#define MARLIN_VAR_MSK4(id1, id2, id3, id4)                      (MARLIN_VAR_MSK2(id1, id2) | MARLIN_VAR_MSK2(id3, id4))
#define MARLIN_VAR_MSK5(id1, id2, id3, id4, id5)                 (MARLIN_VAR_MSK4(id1, id2, id3, id4) | MARLIN_VAR_MSK(id5))
#define MARLIN_VAR_MSK6(id1, id2, id3, id4, id5, id6)            (MARLIN_VAR_MSK3(id1, id2, id3) | MARLIN_VAR_MSK3(id4, id5, id6))
#define MARLIN_VAR_MSK7(id1, id2, id3, id4, id5, id6, id7)       (MARLIN_VAR_MSK4(id1, id2, id3, id4) | MARLIN_VAR_MSK3(id5, id6, id7))
#define MARLIN_VAR_MSK12(id1, id2, id3, id4, id5, id6, ...)      (MARLIN_VAR_MSK6(id1, id2, id3, id4, id5, id6) | MARLIN_VAR_MSK6(__VA_ARGS__))
#define MARLIN_VAR_MSK13(id1, id2, id3, id4, id5, id6, id7, ...) (MARLIN_VAR_MSK7(id1, id2, id3, id4, id5, id6, id7) | MARLIN_VAR_MSK6(__VA_ARGS__))

//maximum number of masks is 64
//maximum mask index is 63
#if (MARLIN_VAR_MAX == 63)
    //in case MARLIN_VAR_MAX == 63 MARLIN_VAR_MSK((MARLIN_VAR_MAX + 1) would fail
    #define MARLIN_VAR_MSK_ALL ((uint64_t)(-1))
#else
    #define MARLIN_VAR_MSK_ALL (MARLIN_VAR_MSK((MARLIN_VAR_MAX + 1)) - (uint64_t)(1)) /// cannot be enum so leave DEFINE (or static const uint64_t)
#endif

#define MARLIN_VAR_MSK_IPOS_XYZE (MARLIN_VAR_MSK4(MARLIN_VAR_IPOS_X, MARLIN_VAR_IPOS_Y, MARLIN_VAR_IPOS_Z, MARLIN_VAR_IPOS_E))

#define MARLIN_VAR_MSK_CURR_POS_XYZE (MARLIN_VAR_MSK4(MARLIN_VAR_CURR_POS_X, MARLIN_VAR_CURR_POS_Y, MARLIN_VAR_CURR_POS_Z, MARLIN_VAR_CURR_POS_E))

static const uint64_t MARLIN_VAR_MSK_POS_XYZE = MARLIN_VAR_MSK4(MARLIN_VAR_POS_X, MARLIN_VAR_POS_Y, MARLIN_VAR_POS_Z, MARLIN_VAR_POS_E);

#define MARLIN_VAR_MSK_TEMP_CURR (MARLIN_VAR_MSK2(MARLIN_VAR_TEMP_NOZ, MARLIN_VAR_TEMP_BED))

static const uint64_t MARLIN_VAR_MSK_TEMP_TARG = MARLIN_VAR_MSK2(MARLIN_VAR_TTEM_NOZ, MARLIN_VAR_TTEM_BED);

static const uint64_t MARLIN_VAR_MSK_TEMP_ALL
    = MARLIN_VAR_MSK7(MARLIN_VAR_TEMP_NOZ, MARLIN_VAR_TEMP_BED, MARLIN_VAR_TTEM_NOZ, MARLIN_VAR_TTEM_BED, MARLIN_VAR_DTEM_NOZ, MARLIN_VAR_PRINT_FAN_RPM, MARLIN_VAR_HEATBREAK_FAN_RPM);

// variables defined in this mask are automaticaly updated every 100ms in _server_update_vars
static const uint64_t MARLIN_VAR_MSK_DEF = MARLIN_VAR_MSK_ALL & ~MARLIN_VAR_MSK(MARLIN_VAR_PQUEUE) & ~MARLIN_VAR_MSK_IPOS_XYZE & ~MARLIN_VAR_MSK(MARLIN_VAR_WAITHEAT) & ~MARLIN_VAR_MSK(MARLIN_VAR_WAITUSER) & ~MARLIN_VAR_MSK(MARLIN_VAR_FILEPATH);

static const uint64_t MARLIN_VAR_MSK_WUI = MARLIN_VAR_MSK_TEMP_ALL | MARLIN_VAR_MSK13(MARLIN_VAR_POS_X, MARLIN_VAR_POS_Y, MARLIN_VAR_POS_Z, MARLIN_VAR_POS_E, MARLIN_VAR_PRNSPEED, MARLIN_VAR_FLOWFACT, MARLIN_VAR_DURATION, MARLIN_VAR_SD_PDONE, MARLIN_VAR_FILENAME, MARLIN_VAR_FILEPATH, MARLIN_VAR_ENDSTOPS, MARLIN_VAR_JOB_ID, MARLIN_VAR_MEDIAINS);

// usr8 in variant8_t message contains id (bit0..6) and variable/event flag (bit7)
static const uint8_t MARLIN_USR8_VAR_FLG = 0x80; // usr8 - variable flag (bit7 set)
static const uint8_t MARLIN_USR8_MSK_ID = 0x7f;  // usr8 - event/variable id mask

static const uint8_t MARLINE_VAR_NAME_MAX = 16; //var_name max length

enum {
    MARLIN_VAR_INDEX_X = 0,
    MARLIN_VAR_INDEX_Y = 1,
    MARLIN_VAR_INDEX_Z = 2,
    MARLIN_VAR_INDEX_E = 3,
};

static const uint8_t MARLIN_VAR_MOTION_MSK_X = 1 << MARLIN_VAR_INDEX_X;
static const uint8_t MARLIN_VAR_MOTION_MSK_Y = 1 << MARLIN_VAR_INDEX_Y;
static const uint8_t MARLIN_VAR_MOTION_MSK_Z = 1 << MARLIN_VAR_INDEX_Z;
static const uint8_t MARLIN_VAR_MOTION_MSK_E = 1 << MARLIN_VAR_INDEX_E;

static const uint32_t TIME_TO_END_INVALID = (uint32_t)-1;

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
    MARLIN_MSG_CHANGE_MASK = 'B',
    MARLIN_MSG_STOP = 'C',
    MARLIN_MSG_EXCLUSIVE = 'D',
    MARLIN_MSG_START = 'E',
    MARLIN_MSG_GCODE = 'F',
    MARLIN_MSG_INJECT_GCODE = 'G',
    MARLIN_MSG_SET_VARIABLE = 'H',
    MARLIN_MSG_UPDATE_VARIABLE = 'I',
    MARLIN_MSG_BABYSTEP = 'J',
    MARLIN_MSG_CONFIG_SAVE = 'K',
    MARLIN_MSG_CONFIG_LOAD = 'L',
    MARLIN_MSG_CONFIG_RESET = 'M',
    MARLIN_MSG_UPDATE = 'N',
    MARLIN_MSG_QUICK_STOP = 'O',
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

// variables structure - used in server and client
// deliberately ordered from longest data types to shortest to avoid alignment issues
typedef struct _marlin_vars_t {
    // 4B base types
    float pos[4];                     // position XYZE [mm]
    int32_t ipos[4];                  // integer position XYZE [steps]
    float curr_pos[4];                // current position XYZE according to G-code [mm]
    float temp_nozzle;                // nozzle temperature [C]
    float temp_bed;                   // bed temperature [C]
    float target_nozzle;              // nozzle target temperature [C]
    float target_bed;                 // bed target temperature [C]
    float z_offset;                   // probe z-offset [mm]
    float display_nozzle;             // nozzle temperature to display [C]
    float travel_acceleration;        // travel acceleration from planner
    uint32_t print_duration;          // print_job_timer.duration() [ms]
    uint32_t time_to_end;             // oProgressData.oTime2End.mGetValue() [s]
    char *media_LFN;                  // Long-File-Name of the currently selected file - a pointer to a global static buffer
    char *media_SFN_path;             // Short-File-Name path to currently selected file - a pointer to a global static buffer
    marlin_print_state_t print_state; // marlin_server.print_state
    uint32_t endstops;                // Binary mask of all endstops

    // 2B base types
    uint16_t print_speed;       // printing speed factor [%]
    uint16_t flow_factor;       // flow factor [%]
    uint16_t print_fan_rpm;     // fanCtlPrint.getActualRPM() [1/min]
    uint16_t heatbreak_fan_rpm; // fanCtlHeatBreak.getActualRPM() [1/min]
    uint16_t job_id;            // print job id incremented at every print start(for connect)

    // 1B base types
    uint8_t motion;              // motion (bit0-X, bit1-Y, bit2-Z, bit3-E)
    uint8_t gqueue;              // number of commands in gcode queue
    uint8_t pqueue;              // number of commands in planner queue
    uint8_t print_fan_speed;     // print fan speed [0..255]
    uint8_t wait_heat;           // wait_for_heatup
    uint8_t wait_user;           // wait_for_user
    uint8_t sd_percent_done;     // card.percentDone() [%]
    uint8_t media_inserted;      // media_is_inserted()
    uint8_t fan_check_enabled;   // fan_check [on/off]
    uint8_t fs_autoload_enabled; // fs_autoload [on/off]
} marlin_vars_t;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

inline int is_abort_state(marlin_print_state_t st) {
    return ((int)st) >= ((int)mpsAborting_Begin) && ((int)st) <= ((int)mpsAborted);
}

// returns variable name
extern const char *marlin_vars_get_name(marlin_var_id_t var_id);

// returns variable id by name or -1 if not match
extern marlin_var_id_t marlin_vars_get_id_by_name(const char *var_name);

// get variable value as variant8 directly from vars structure
// \returns empty variant if the variable is not readable
extern variant8_t marlin_vars_get_var(marlin_vars_t *vars, marlin_var_id_t var_id);

// set variable value as variant8 directly in vars structure
extern void marlin_vars_set_var(marlin_vars_t *vars, marlin_var_id_t var_id, variant8_t var);

// format variable to string
extern int marlin_vars_value_to_str(marlin_vars_t *vars, marlin_var_id_t var_id, char *str, unsigned int size);

// parse variable from string, returns sscanf result (1 = ok)
extern int marlin_vars_str_to_value(marlin_vars_t *vars, marlin_var_id_t var_id, const char *str);

// converts message's ID to string
// string must have 3 bytes at least
extern void marlin_msg_to_str(const marlin_msg_t id, char *str);

#ifdef __cplusplus
}
#endif //__cplusplus

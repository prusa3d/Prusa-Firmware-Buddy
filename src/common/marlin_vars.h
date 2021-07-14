// marlin_vars.h
#pragma once

#include "variant8.h"

// Marlin variables
enum {
    MARLIN_VAR_MOTION = 0x00,              // R:  uint8, method stepper.axis_is_moving
    MARLIN_VAR_GQUEUE = 0x01,              // R:  uint8, method queue.length
    MARLIN_VAR_PQUEUE = 0x02,              // R:  uint8, variables planner.block_buffer_head/tail
    MARLIN_VAR_IPOS_X = 0x03,              // RW: int32, variable stepper.count_position
    MARLIN_VAR_IPOS_Y = 0x04,              // RW: ==||==
    MARLIN_VAR_IPOS_Z = 0x05,              // RW: ==||==
    MARLIN_VAR_IPOS_E = 0x06,              // RW: ==||==
    MARLIN_VAR_POS_X = 0x07,               // RW: float, planner.getAxisPosition_mm(), setAxisPosition_mm()
    MARLIN_VAR_POS_Y = 0x08,               // RW: ==||==
    MARLIN_VAR_POS_Z = 0x09,               // RW: ==||==
    MARLIN_VAR_POS_E = 0x0a,               // RW: ==||==
    MARLIN_VAR_TEMP_NOZ = 0x0b,            // R:  float, thermalManager.temp_hotend[0].current
    MARLIN_VAR_TEMP_BED = 0x0c,            // R:  float, thermalManager.temp_bed.current
    MARLIN_VAR_TTEM_NOZ = 0x0d,            // RW: float, thermalManager.temp_hotend[0].target, thermalManager.setTargetHotend()
    MARLIN_VAR_TTEM_BED = 0x0e,            // RW: float, thermalManager.temp_bed.target, thermalManager.setTargetBed()
    MARLIN_VAR_Z_OFFSET = 0x0f,            // R:  float, zprobe_zoffset
    MARLIN_VAR_FANSPEED = 0x10,            // RW: uint8, thermalManager.fan_speed[0], thermalManager.set_fan_speed()
    MARLIN_VAR_PRNSPEED = 0x11,            // RW: uint16, feedrate_percentage
    MARLIN_VAR_FLOWFACT = 0x12,            // RW: uint16, planner.flow_percentage
    MARLIN_VAR_WAITHEAT = 0x13,            // RW: uint8, Marlin, wait_for_heatup
    MARLIN_VAR_WAITUSER = 0x14,            // RW: uint8, Marlin, wait_for_user
    MARLIN_VAR_SD_PRINT = 0x15,            // R:  uint8, card.flag.sdprinting
    MARLIN_VAR_SD_PDONE = 0x16,            // R:  uint8, card.percentDone()
    MARLIN_VAR_DURATION = 0x17,            // R:  uint32, print_job_timer.duration()
    MARLIN_VAR_MEDIAINS = 0x18,            // R:  uint8, media_is_inserted()
    MARLIN_VAR_PRNSTATE = 0x19,            // R:  marlin_print_state_t, marlin_server.print_state
    MARLIN_VAR_FILENAME = 0x1a,            // R:  char*,
    MARLIN_VAR_FILEPATH = 0x1b,            // R:  char*,
    MARLIN_VAR_DTEM_NOZ = 0x1c,            // R:  float, nozzle temperature to display
    MARLIN_VAR_TIMTOEND = 0x1d,            // R:  uint32, oProgressData.oTime2End.mGetValue() or -1 if not valid
    MARLIN_VAR_FAN0_RPM = 0x1e,            // R:  uint16, fanctl0.getActualRPM()
    MARLIN_VAR_FAN1_RPM = 0x1f,            // R:  uint16, fanctl1.getActualRPM()
    MARLIN_VAR_FAN_CHECK_ENABLED = 0x20,   //RW: uintt8, fan_check
    MARLIN_VAR_FS_AUTOLOAD_ENABLED = 0x21, //RW: uint8_t fs_autoload
    MARLIN_VAR_MAX = MARLIN_VAR_FS_AUTOLOAD_ENABLED
};

// variable masks
#define MARLIN_VAR_MSK(v_id) (((uint64_t)1) << (uint8_t)(v_id))

//maximum number of masks is 64
//maximum mask index is 63
#if (MARLIN_VAR_MAX == 63)
    //in case MARLIN_VAR_MAX == 63 MARLIN_VAR_MSK((MARLIN_VAR_MAX + 1) would fail
    #define MARLIN_VAR_MSK_ALL ((uint64_t)(-1))
#else
    #define MARLIN_VAR_MSK_ALL (MARLIN_VAR_MSK((MARLIN_VAR_MAX + 1)) - (uint64_t)(1)) /// cannot be enum so leave DEFINE (or static const uint64_t)
#endif

#define MARLIN_VAR_MSK_IPOS_XYZE (MARLIN_VAR_MSK(MARLIN_VAR_IPOS_X) | MARLIN_VAR_MSK(MARLIN_VAR_IPOS_Y) | MARLIN_VAR_MSK(MARLIN_VAR_IPOS_Z) | MARLIN_VAR_MSK(MARLIN_VAR_IPOS_E))

static const uint64_t MARLIN_VAR_MSK_POS_XYZE = MARLIN_VAR_MSK(MARLIN_VAR_POS_X) | MARLIN_VAR_MSK(MARLIN_VAR_POS_Y) | MARLIN_VAR_MSK(MARLIN_VAR_POS_Z) | MARLIN_VAR_MSK(MARLIN_VAR_POS_E);

#define MARLIN_VAR_MSK_TEMP_CURR (MARLIN_VAR_MSK(MARLIN_VAR_TEMP_NOZ) | MARLIN_VAR_MSK(MARLIN_VAR_TEMP_BED))

static const uint64_t MARLIN_VAR_MSK_TEMP_TARG = MARLIN_VAR_MSK(MARLIN_VAR_TTEM_NOZ) | MARLIN_VAR_MSK(MARLIN_VAR_TTEM_BED);

static const uint64_t MARLIN_VAR_MSK_TEMP_ALL
    = MARLIN_VAR_MSK(MARLIN_VAR_TEMP_NOZ) | MARLIN_VAR_MSK(MARLIN_VAR_TEMP_BED) | MARLIN_VAR_MSK(MARLIN_VAR_TTEM_NOZ) | MARLIN_VAR_MSK(MARLIN_VAR_TTEM_BED) | MARLIN_VAR_MSK(MARLIN_VAR_DTEM_NOZ) | MARLIN_VAR_MSK(MARLIN_VAR_FAN0_RPM) | MARLIN_VAR_MSK(MARLIN_VAR_FAN0_RPM);

// variables defined in this mask are automaticaly updated every 100ms in _server_update_vars
static const uint64_t MARLIN_VAR_MSK_DEF = MARLIN_VAR_MSK_ALL & ~MARLIN_VAR_MSK(MARLIN_VAR_PQUEUE) & ~MARLIN_VAR_MSK_IPOS_XYZE & ~MARLIN_VAR_MSK(MARLIN_VAR_WAITHEAT) & ~MARLIN_VAR_MSK(MARLIN_VAR_WAITUSER) & ~MARLIN_VAR_MSK(MARLIN_VAR_FILEPATH);

static const uint64_t MARLIN_VAR_MSK_WUI
    = MARLIN_VAR_MSK_TEMP_CURR | MARLIN_VAR_MSK(MARLIN_VAR_POS_Z) | MARLIN_VAR_MSK(MARLIN_VAR_PRNSPEED) | MARLIN_VAR_MSK(MARLIN_VAR_FLOWFACT) | MARLIN_VAR_MSK(MARLIN_VAR_DURATION) | MARLIN_VAR_MSK(MARLIN_VAR_SD_PDONE) | MARLIN_VAR_MSK(MARLIN_VAR_SD_PRINT) | MARLIN_VAR_MSK(MARLIN_VAR_FILENAME);

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

enum {
    FILE_NAME_MAX_LEN = 96 + 1 + 5 + 1,
    FILE_PATH_MAX_LEN = 96 + 1 + 5 + 1,
};
static const uint32_t TIME_TO_END_INVALID = (uint32_t)-1;

typedef enum {
    mpsIdle = 0,
    mpsPrinting,
    mpsPausing_Begin,
    mpsPausing_WaitIdle,
    mpsPausing_ParkHead,
    mpsPaused,
    mpsResuming_Begin,
    mpsResuming_Reheating,
    mpsResuming_UnparkHead,
    mpsAborting_Begin,
    mpsAborting_WaitIdle,
    mpsAborting_ParkHead,
    mpsAborted,
    mpsFinishing_WaitIdle,
    mpsFinishing_ParkHead,
    mpsFinished,
} marlin_print_state_t;

// variables structure - used in server and client
// deliberately ordered from longest data types to shortest to avoid alignment issues
typedef struct _marlin_vars_t {
    // 4B base types
    float pos[4];                     // position XYZE [mm]
    int32_t ipos[4];                  // integer position XYZE [steps]
    float temp_nozzle;                // nozzle temperature [C]
    float temp_bed;                   // bed temperature [C]
    float target_nozzle;              // nozzle target temperature [C]
    float target_bed;                 // bed target temperature [C]
    float z_offset;                   // probe z-offset [mm]
    float display_nozzle;             // nozzle temperature to display [C]
    uint32_t print_duration;          // print_job_timer.duration() [ms]
    uint32_t time_to_end;             // oProgressData.oTime2End.mGetValue() [s]
    char *media_LFN;                  // Long-File-Name of the currently selected file - a pointer to a global static buffer
    char *media_SFN_path;             // Short-File-Name path to currently selected file - a pointer to a global static buffer
    marlin_print_state_t print_state; // marlin_server.print_state

    // 2B base types
    uint16_t print_speed; // printing speed factor [%]
    uint16_t flow_factor; // flow factor [%]
    uint16_t fan0_rpm;    // fanctl0.getActualRPM() [1/min]
    uint16_t fan1_rpm;    // fanctl1.getActualRPM() [1/min]

    // 1B base types
    uint8_t motion;              // motion (bit0-X, bit1-Y, bit2-Z, bit3-E)
    uint8_t gqueue;              // number of commands in gcode queue
    uint8_t pqueue;              // number of commands in planner queue
    uint8_t fan_speed;           // print fan0 speed [0..255]
    uint8_t wait_heat;           // wait_for_heatup
    uint8_t wait_user;           // wait_for_user
    uint8_t sd_printing;         // card.flag.sdprinting
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
extern const char *marlin_vars_get_name(uint8_t var_id);

// returns variable id by name or -1 if not match
extern int marlin_vars_get_id_by_name(const char *var_name);

// get variable value as variant8 directly from vars structure
extern variant8_t marlin_vars_get_var(marlin_vars_t *vars, uint8_t var_id);

// set variable value as variant8 directly in vars structure
extern void marlin_vars_set_var(marlin_vars_t *vars, uint8_t var_id, variant8_t var);

// format variable to string
extern int marlin_vars_value_to_str(marlin_vars_t *vars, uint8_t var_id, char *str, unsigned int size);

// parse variable from string, returns sscanf result (1 = ok)
extern int marlin_vars_str_to_value(marlin_vars_t *vars, uint8_t var_id, const char *str);

#ifdef __cplusplus
}
#endif //__cplusplus

// marlin_vars.h
#ifndef _MARLIN_VARS_H
#define _MARLIN_VARS_H

#include "variant8.h"

// Marlin variables
#define MARLIN_VAR_MOTION   0x00 // R:  uint8, method stepper.axis_is_moving
#define MARLIN_VAR_GQUEUE   0x01 // R:  uint8, method queue.length
#define MARLIN_VAR_PQUEUE   0x02 // R:  uint8, variables planner.block_buffer_head/tail
#define MARLIN_VAR_IPOS_X   0x03 // RW: int32, variable stepper.count_position
#define MARLIN_VAR_IPOS_Y   0x04 // RW: ==||==
#define MARLIN_VAR_IPOS_Z   0x05 // RW: ==||==
#define MARLIN_VAR_IPOS_E   0x06 // RW: ==||==
#define MARLIN_VAR_POS_X    0x07 // RW: float, planner.getAxisPosition_mm(), setAxisPosition_mm()
#define MARLIN_VAR_POS_Y    0x08 // RW: ==||==
#define MARLIN_VAR_POS_Z    0x09 // RW: ==||==
#define MARLIN_VAR_POS_E    0x0a // RW: ==||==
#define MARLIN_VAR_TEMP_NOZ 0x0b // R:  float, thermalManager.temp_hotend[0].current
#define MARLIN_VAR_TEMP_BED 0x0c // R:  float, thermalManager.temp_bed.current
#define MARLIN_VAR_TTEM_NOZ 0x0d // RW: float, thermalManager.temp_hotend[0].target, thermalManager.setTargetHotend()
#define MARLIN_VAR_TTEM_BED 0x0e // RW: float, thermalManager.temp_bed.target, thermalManager.setTargetBed()
#define MARLIN_VAR_Z_OFFSET 0x0f // R:  float, zprobe_zoffset
#define MARLIN_VAR_FANSPEED 0x10 // RW: uint8, thermalManager.fan_speed[0], thermalManager.set_fan_speed()
#define MARLIN_VAR_PRNSPEED 0x11 // RW: uint16, feedrate_percentage
#define MARLIN_VAR_FLOWFACT 0x12 // RW: uint16, planner.flow_percentage
#define MARLIN_VAR_WAITHEAT 0x13 // RW: uint8, Marlin, wait_for_heatup
#define MARLIN_VAR_WAITUSER 0x14 // RW: uint8, Marlin, wait_for_user
#define MARLIN_VAR_SD_PRINT 0x15 // R:  uint8, card.flag.sdprinting
#define MARLIN_VAR_SD_PDONE 0x16 // R:  uint8, card.percentDone()
#define MARLIN_VAR_DURATION 0x17 // R:  uint32, print_job_timer.duration()
#define MARLIN_VAR_MEDIAINS 0x18 // R:  uint8, media_is_inserted()
#define MARLIN_VAR_PRNSTATE 0x19 // R:  marlin_print_state_t, marlin_server.print_state
#define MARLIN_VAR_FILENAME 0x1a // R:  char*,
#define MARLIN_VAR_FILEPATH 0x1b // R:  char*,
#define MARLIN_VAR_DTEM_NOZ 0x1c // R:  float, nozzle temperature to display
#define MARLIN_VAR_TIMTOEND 0x1d // R:  uint32, oProgressData.oTime2End.mGetValue() or -1 if not valid
#define MARLIN_VAR_MAX      MARLIN_VAR_TIMTOEND

// variable masks
#define MARLIN_VAR_MSK(v_id) ((uint64_t)1 << (v_id))

#define MARLIN_VAR_MSK_IPOS_XYZE ( \
    MARLIN_VAR_MSK(MARLIN_VAR_IPOS_X) | MARLIN_VAR_MSK(MARLIN_VAR_IPOS_Y) | MARLIN_VAR_MSK(MARLIN_VAR_IPOS_Z) | MARLIN_VAR_MSK(MARLIN_VAR_IPOS_E))

#define MARLIN_VAR_MSK_POS_XYZE ( \
    MARLIN_VAR_MSK(MARLIN_VAR_POS_X) | MARLIN_VAR_MSK(MARLIN_VAR_POS_Y) | MARLIN_VAR_MSK(MARLIN_VAR_POS_Z) | MARLIN_VAR_MSK(MARLIN_VAR_POS_E))

#define MARLIN_VAR_MSK_TEMP_CURR ( \
    MARLIN_VAR_MSK(MARLIN_VAR_TEMP_NOZ) | MARLIN_VAR_MSK(MARLIN_VAR_TEMP_BED))

#define MARLIN_VAR_MSK_TEMP_TARG ( \
    MARLIN_VAR_MSK(MARLIN_VAR_TTEM_NOZ) | MARLIN_VAR_MSK(MARLIN_VAR_TTEM_BED))

#define MARLIN_VAR_MSK_TEMP_ALL ( \
    MARLIN_VAR_MSK(MARLIN_VAR_TEMP_NOZ) | MARLIN_VAR_MSK(MARLIN_VAR_TEMP_BED) | MARLIN_VAR_MSK(MARLIN_VAR_TTEM_NOZ) | MARLIN_VAR_MSK(MARLIN_VAR_TTEM_BED) | MARLIN_VAR_MSK(MARLIN_VAR_DTEM_NOZ))

#define MARLIN_VAR_MSK_DEF ( \
    MARLIN_VAR_MSK(MARLIN_VAR_MOTION) | MARLIN_VAR_MSK(MARLIN_VAR_GQUEUE) | MARLIN_VAR_MSK_POS_XYZE | MARLIN_VAR_MSK_TEMP_ALL | MARLIN_VAR_MSK(MARLIN_VAR_Z_OFFSET) | MARLIN_VAR_MSK(MARLIN_VAR_SD_PRINT) | MARLIN_VAR_MSK(MARLIN_VAR_SD_PDONE) | MARLIN_VAR_MSK(MARLIN_VAR_DURATION) | MARLIN_VAR_MSK(MARLIN_VAR_MEDIAINS) | MARLIN_VAR_MSK(MARLIN_VAR_PRNSTATE) | MARLIN_VAR_MSK(MARLIN_VAR_FILENAME) | MARLIN_VAR_MSK(MARLIN_VAR_TIMTOEND))

#define MARLIN_VAR_MSK_WUI ( \
    MARLIN_VAR_MSK_TEMP_CURR | MARLIN_VAR_MSK(MARLIN_VAR_POS_Z) | MARLIN_VAR_MSK(MARLIN_VAR_PRNSPEED) | MARLIN_VAR_MSK(MARLIN_VAR_FLOWFACT) | MARLIN_VAR_MSK(MARLIN_VAR_DURATION) | MARLIN_VAR_MSK(MARLIN_VAR_SD_PDONE) | MARLIN_VAR_MSK(MARLIN_VAR_SD_PRINT) | MARLIN_VAR_MSK(MARLIN_VAR_FILENAME))

#define MARLIN_VAR_MSK_ALL ( \
    MARLIN_VAR_MSK(MARLIN_VAR_MOTION) | MARLIN_VAR_MSK(MARLIN_VAR_GQUEUE) | MARLIN_VAR_MSK(MARLIN_VAR_PQUEUE) | MARLIN_VAR_MSK_IPOS_XYZE | MARLIN_VAR_MSK_POS_XYZE | MARLIN_VAR_MSK_TEMP_ALL | MARLIN_VAR_MSK(MARLIN_VAR_Z_OFFSET) | MARLIN_VAR_MSK(MARLIN_VAR_FANSPEED) | MARLIN_VAR_MSK(MARLIN_VAR_PRNSPEED) | MARLIN_VAR_MSK(MARLIN_VAR_FLOWFACT) | MARLIN_VAR_MSK(MARLIN_VAR_WAITHEAT) | MARLIN_VAR_MSK(MARLIN_VAR_WAITUSER) | MARLIN_VAR_MSK(MARLIN_VAR_SD_PRINT) | MARLIN_VAR_MSK(MARLIN_VAR_SD_PDONE) | MARLIN_VAR_MSK(MARLIN_VAR_DURATION) | MARLIN_VAR_MSK(MARLIN_VAR_PRNSTATE) | MARLIN_VAR_MSK(MARLIN_VAR_FILENAME) | MARLIN_VAR_MSK(MARLIN_VAR_FILEPATH) | MARLIN_VAR_MSK(MARLIN_VAR_TIMTOEND))

// usr8 in variant8_t message contains id (bit0..6) and variable/event flag (bit7)
#define MARLIN_USR8_VAR_FLG 0x80 // usr8 - variable flag (bit7 set)
#define MARLIN_USR8_MSK_ID  0x7f // usr8 - event/variable id mask

#define MARLINE_VAR_NAME_MAX 16 //var_name max length

#define MARLIN_VAR_INDEX_X      0
#define MARLIN_VAR_INDEX_Y      1
#define MARLIN_VAR_INDEX_Z      2
#define MARLIN_VAR_INDEX_E      3
#define MARLIN_VAR_MOTION_MSK_X (1 << MARLIN_VAR_INDEX_X)
#define MARLIN_VAR_MOTION_MSK_Y (1 << MARLIN_VAR_INDEX_Y)
#define MARLIN_VAR_MOTION_MSK_Z (1 << MARLIN_VAR_INDEX_Z)
#define MARLIN_VAR_MOTION_MSK_E (1 << MARLIN_VAR_INDEX_E)

#define FILE_NAME_MAX_LEN (96 + 1 + 5 + 1)
#define FILE_PATH_MAX_LEN (96 + 1 + 5 + 1)

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

#pragma pack(push)
#pragma pack(1)

// variables structure - used in server and client
typedef struct _marlin_vars_t {
    uint8_t motion;                   // motion (bit0-X, bit1-Y, bit2-Z, bit3-E)
    uint8_t gqueue;                   // number of commands in gcode queue
    uint8_t pqueue;                   // number of commands in planner queue
    int32_t ipos[4];                  // integer position XYZE [steps]
    float pos[4];                     // position XYZE [mm]
    float temp_nozzle;                // nozzle temperature [C]
    float temp_bed;                   // bed temperature [C]
    float target_nozzle;              // nozzle target temperature [C]
    float target_bed;                 // bed target temperature [C]
    float z_offset;                   // probe z-offset [mm]
    uint8_t fan_speed;                // print fan0 speed [0..255]
    uint16_t print_speed;             // printing speed factor [%]
    uint16_t flow_factor;             // flow factor [%]
    uint8_t wait_heat;                // wait_for_heatup
    uint8_t wait_user;                // wait_for_user
    uint8_t sd_printing;              // card.flag.sdprinting
    uint8_t sd_percent_done;          // card.percentDone() [%]
    uint32_t print_duration;          // print_job_timer.duration() [ms]
    uint8_t media_inserted;           // media_is_inserted()
    marlin_print_state_t print_state; // marlin_server.print_state
    char *media_file_name;            //
    char *media_file_path;            //
    float display_nozzle;             // nozzle temperature to display [C]
    uint32_t time_to_end;             // oProgressData.oTime2End.mGetValue() [ms]
} marlin_vars_t;

#pragma pack(pop)

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

#endif //_MARLIN_VARS_H

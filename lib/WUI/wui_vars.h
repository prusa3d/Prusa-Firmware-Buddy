#ifndef WUI_VARS_H
#define WUI_VARS_H

#include "marlin_vars.h"

#define X_AXIS_POS 0
#define Y_AXIS_POS 1
#define Z_AXIS_POS 2

typedef struct {
    char gcode_name[FILE_NAME_MAX_LEN];
    float pos[4];
    float temp_nozzle;
    float temp_bed;
    float target_nozzle;
    float target_bed;
    uint32_t print_dur;
    uint32_t time_to_end;
    uint16_t print_speed;
    uint16_t flow_factor;
    uint8_t fan_speed;
    uint8_t sd_precent_done;
    uint8_t sd_printing;
    marlin_print_state_t print_state;
} wui_vars_t;

extern wui_vars_t wui_vars;

#endif //WUI_VARS_H

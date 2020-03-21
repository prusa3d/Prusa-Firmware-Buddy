#ifndef WUI_VARS_H
#define WUI_VARS_H

#include "marlin_vars.h"

#define X_AXIS_POS 0
#define Y_AXIS_POS 1
#define Z_AXIS_POS 2
typedef struct {
    char gcode_name[GCODE_NAME_MAX_LEN + 1];
    float pos[4];
    float temp_nozzle;
    float temp_bed;
    uint32_t print_dur;
    uint16_t print_speed;
    uint16_t flow_factor;
    uint8_t sd_precent_done;
    uint8_t sd_printing;
} web_vars_t;
extern web_vars_t web_vars;

#endif //WUI_VARS_H

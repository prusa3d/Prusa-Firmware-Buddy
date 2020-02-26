#ifndef WUI_VARS_H
#define WUI_VARS_H

#include "marlin_vars.h"

#define X_AXIS_POS 0
#define Y_AXIS_POS 1
#define Z_AXIS_POS 2
typedef struct {
    float pos[4];
    float temp_nozzle;
    float temp_bed;
    uint16_t print_speed;
    uint16_t flow_factor;
} web_vars_t;
extern web_vars_t web_vars;

#endif //WUI_VARS_H

/*
 * status_footer.h
 *
 *  Created on: 22. 7. 2019
 *      Author: mcbig
 */

#pragma once

#include "gui.h"

#pragma pack(push, 1)
//#pragma pack(1) makes enums 8 bit
// which is an ugly and unreadable hack (probably a side effect)
typedef enum heat_state_e {
    HEATING,
    COOLING,
    PREHEAT,
    STABLE,
} heat_state_t;

#pragma pack(pop)

typedef struct
{
    float nozzle;                /// current temperature of nozzle
    float nozzle_target;         /// target temperature of nozzle (not shown)
    float nozzle_target_display; /// target temperature of nozzle shown on display
    float heatbed;               /// current temperature of bed
    float heatbed_target;        /// target temperature of bed
    int32_t z_pos;               /// z position, 000.00 fixed point
    uint32_t last_timer_repaint_values;
    uint32_t last_timer_repaint_colors;
    uint32_t last_timer_repaint_z_pos;

    window_icon_t wi_nozzle;
    window_icon_t wi_heatbed;
    window_icon_t wi_prnspeed;
    window_icon_t wi_z_axis;
    window_icon_t wi_filament;

    window_text_t wt_nozzle;
    window_text_t wt_heatbed;
    window_text_t wt_prnspeed;
    window_text_t wt_z_axis;
    window_text_t wt_filament;

    uint16_t print_speed; /// print speed in percents
    heat_state_t nozzle_state;
    heat_state_t heatbed_state;
    bool show_second_color;

} status_footer_t;

#define REPAINT_Z_POS_PERIOD 256  /// time span between z position repaint [miliseconds]
#define REPAINT_VALUE_PERIOD 1024 /// time span between value repaint [miliseconds]
#define BLINK_PERIOD         512  /// time span between color changes [miliseconds]

#define COOL_NOZZLE 50 /// highest temperature of nozzle to be considered as cool
#define COOL_BED    45 /// highest temperature of bed to be considered as cool

#define DEFAULT_COLOR COLOR_WHITE
#define STABLE_COLOR  COLOR_WHITE
#define HEATING_COLOR COLOR_ORANGE
#define COOLING_COLOR COLOR_BLUE
#define PREHEAT_COLOR COLOR_GREEN

void status_footer_init(status_footer_t *footer, int16_t parent);
int status_footer_event(status_footer_t *footer, window_t *window, uint8_t event, const void *param);

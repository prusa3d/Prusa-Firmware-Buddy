/*
 * status_footer.h
 *
 *  Created on: 22. 7. 2019
 *      Author: mcbig
 */

#ifndef STATUS_FOOTER_H_
#define STATUS_FOOTER_H_

#include "gui.h"

#pragma pack(push)
#pragma pack(1)

typedef enum heat_state_e {
    HEATING,
    COOLING,
    PREHEAT,
    STABLE,
} heat_state_t;

typedef struct
{
    float nozzle;        /// temperature of nozzle shown on display
    float nozzle_target; /// target temperature of nozzle shown on display
    float heatbed;       /// temperature of bed shown on display

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

    char text_nozzle[10]; // "215/215°C"
    char text_heatbed[10];
    char text_prnspeed[5]; // "999%"
    char text_z_axis[7];   // "999.95"

#ifdef LCD_HEATBREAK_TO_FILAMENT
    char text_heatbreak[5]; // "99°C"
#endif

    uint32_t last_timer_repaint_values;
    uint32_t last_timer_repaint_colors;

    heat_state_t nozzle_state;
    heat_state_t heatbed_state;
    bool show_second_color;

} status_footer_t;

#pragma pack(pop)

#define BUTTON_STATUS_NOZZLE   0xf0
#define BUTTON_STATUS_HEATBED  0xf1
#define BUTTON_STATUS_PRNSPEED 0xf2
#define BUTTON_STATUS_Z_AXIS   0xf3
#define BUTTON_STATUS_FILAMENT 0xf4

#define REPAINT_VALUE_PERIOD 1000 /// time span between value repaint [miliseconds]
#define BLINK_PERIOD         500  /// time span between color changes [miliseconds]

#define COOL_NOZZLE 50 /// highest temperature of nozzle to be considered as cool
#define COOL_BED    45 /// highest temperature of be to be considered as cool

#define DEFAULT_COLOR COLOR_WHITE
#define STABLE_COLOR  COLOR_WHITE
#define HEATING_COLOR COLOR_ORANGE
#define COOLING_COLOR COLOR_BLUE
#define PREHEAT_COLOR COLOR_GREEN

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

void status_footer_init(status_footer_t *footer, int16_t parent);
int status_footer_event(status_footer_t *footer, window_t *window, uint8_t event, const void *param);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* STATUS_FOOTER_H_ */

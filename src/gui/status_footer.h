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

typedef struct
{
    float nozzle;
    float heatbed;

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

    uint32_t last_timer_repaint_temperatures, last_timer_repaint_z;

} status_footer_t;

#pragma pack(pop)

#define BUTTON_STATUS_NOZZLE   0xf0
#define BUTTON_STATUS_HEATBED  0xf1
#define BUTTON_STATUS_PRNSPEED 0xf2
#define BUTTON_STATUS_Z_AXIS   0xf3
#define BUTTON_STATUS_FILAMENT 0xf4

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

void status_footer_init(status_footer_t *footer, int16_t parent);
int status_footer_event(status_footer_t *footer, window_t *window, uint8_t event, const void *param);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* STATUS_FOOTER_H_ */

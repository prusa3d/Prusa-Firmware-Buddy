/*
 * status_footer.cpp
 *
 *  Created on: 22. 7. 2019
 *      Author: mcbig
 */

#include "config.h"
#include "status_footer.h"
#include "filament.h"

#include "../Marlin/src/module/temperature.h"
#include "../Marlin/src/module/planner.h"

#define HEATING_DIFFERENCE 2

void status_footer_timer(status_footer_t *footer, uint32_t mseconds);
void status_footer_update_temperatures(status_footer_t *footer,
    bool &heating_nozzle, bool &heating_heatbed, bool &cooling_nozzle,
    bool &cooling_heatbed);
void status_footer_update_feedrate(status_footer_t *footer);
void status_footer_update_z_axis(status_footer_t *footer);
void status_footer_update_filament(status_footer_t *footer);
void status_footer_indicate_nozzle(window_text_t *nozzle, bool heating, bool cooling);
void status_footer_indicate_heatbed(window_text_t *heatbed, bool heating, bool cooling);

void status_footer_init(status_footer_t *footer, int16_t parent) {
    int16_t id;

    strcpy(footer->text_nozzle, "0/0\177C");
    strcpy(footer->text_heatbed, "0/0\177C");
    strcpy(footer->text_prnspeed, "0%");
    strcpy(footer->text_z_axis, "999.95");

    id = window_create_ptr( // nozzle
        WINDOW_CLS_ICON, parent,
        rect_ui16(8, 270, 16, 16), // 2px padding from right
        &(footer->wi_nozzle));
    window_set_icon_id(id, IDR_PNG_status_icon_nozzle);
    window_set_tag(id, BUTTON_STATUS_NOZZLE);
    // window_enable(id);

    id = window_create_ptr(
        WINDOW_CLS_TEXT, parent,
        rect_ui16(24, 269, 85, 20),
        &(footer->wt_nozzle));
    footer->wt_nozzle.font = resource_font(IDR_FNT_SPECIAL);
    window_set_alignment(id, ALIGN_CENTER);
    window_set_text(id, footer->text_nozzle);

    id = window_create_ptr( // heatbed
        WINDOW_CLS_ICON, parent,
        rect_ui16(128, 270, 20, 16),
        &(footer->wi_heatbed));
    window_set_icon_id(id, IDR_PNG_status_icon_heatbed);
    window_set_tag(id, BUTTON_STATUS_HEATBED);
    //window_enable(id);

    id = window_create_ptr(
        WINDOW_CLS_TEXT, parent,
        rect_ui16(150, 269, 85, 22),
        &(footer->wt_heatbed));
    footer->wt_heatbed.font = resource_font(IDR_FNT_SPECIAL);
    window_set_alignment(id, ALIGN_CENTER);
    window_set_text(id, footer->text_heatbed);

    id = window_create_ptr( // prnspeed
        WINDOW_CLS_ICON, parent,
        rect_ui16(10, 297, 16, 12),
        &(footer->wi_prnspeed));
    window_set_icon_id(id, IDR_PNG_status_icon_prnspeed);
    window_set_tag(id, BUTTON_STATUS_PRNSPEED);
    //window_enable(id);

    id = window_create_ptr(
        WINDOW_CLS_TEXT, parent,
        rect_ui16(28, 296, 40, 22),
        &(footer->wt_prnspeed));
    footer->wt_prnspeed.font = resource_font(IDR_FNT_SPECIAL);
    window_set_alignment(id, ALIGN_CENTER);
    window_set_text(id, footer->text_prnspeed);

    id = window_create_ptr( // z-axis
        WINDOW_CLS_ICON, parent,
        rect_ui16(80, 297, 16, 16),
        &(footer->wi_z_axis));
    window_set_icon_id(id, IDR_PNG_status_icon_z_axis);
    window_set_tag(id, BUTTON_STATUS_PRNSPEED);
    //window_enable(id);

    id = window_create_ptr(
        WINDOW_CLS_TEXT, parent,
        rect_ui16(102, 296, 58, 22),
        &(footer->wt_z_axis));
    footer->wt_z_axis.font = resource_font(IDR_FNT_SPECIAL);
    window_set_alignment(id, ALIGN_CENTER);
    window_set_text(id, footer->text_z_axis);

    id = window_create_ptr( // filament
        WINDOW_CLS_ICON, parent,
        rect_ui16(163, 297, 16, 16),
        &(footer->wi_filament));
    window_set_icon_id(id, IDR_PNG_status_icon_filament);
    window_set_tag(id, BUTTON_STATUS_PRNSPEED);
    //window_enable(id);

    id = window_create_ptr(
        WINDOW_CLS_TEXT, parent,
        rect_ui16(181, 296, 49, 22),
        &(footer->wt_filament));
    footer->wt_filament.font = resource_font(IDR_FNT_SPECIAL);
    window_set_alignment(id, ALIGN_CENTER);
    window_set_text(id, filaments[get_filament()].name);

    status_footer_timer(footer, 0); // do update
}

int status_footer_event(status_footer_t *footer, window_t *window,
    uint8_t event, const void *param) {
    status_footer_timer(footer, (HAL_GetTick() / 50) * 50);

    if (event != WINDOW_EVENT_CLICK) {
        return 0;
    }

    switch ((int)param) {
    case BUTTON_STATUS_NOZZLE:
    case BUTTON_STATUS_HEATBED:
    case BUTTON_STATUS_PRNSPEED:
    case BUTTON_STATUS_Z_AXIS:
    case BUTTON_STATUS_FILAMENT:
        return 0;
    }
    return 0;
}

void status_footer_timer(status_footer_t *footer, uint32_t mseconds) {
    bool heating_nozzle = false;
    bool cooling_nozzle = false;
    bool heating_heatbed = false;
    bool cooling_heatbed = false;

    //if (mseconds % 1000 == 0){ // this is not OK, it assumes, that the timer ticks are coming in regularly
    // which is not the case here
    if ((mseconds - footer->last_timer_repaint_temperatures) >= 1000) {
        status_footer_update_temperatures(footer, heating_nozzle, heating_heatbed,
            cooling_nozzle, cooling_heatbed);
        status_footer_update_feedrate(footer);
        status_footer_update_filament(footer);
        footer->last_timer_repaint_temperatures = mseconds;
    }

    if ((mseconds - footer->last_timer_repaint_z) >= 500) {
        status_footer_update_z_axis(footer);
        // WTF - heating_nozzle is false and occasionally true - is that what we want?
        status_footer_indicate_nozzle(&(footer->wt_nozzle), heating_nozzle, cooling_nozzle);
        status_footer_indicate_heatbed(&(footer->wt_heatbed), heating_heatbed, cooling_heatbed);
        footer->last_timer_repaint_z = mseconds;
    }
}

void status_footer_update_temperatures(status_footer_t *footer,
    bool &heating_nozzle, bool &heating_heatbed, bool &cooling_nozzle,
    bool &cooling_heatbed) {
    /*
	float actual_nozzle = ExtUI::getActualTemp_celsius(ExtUI::extruder_t::E0);
	float target_nozzle = ExtUI::getTargetTemp_celsius(ExtUI::extruder_t::E0);
	float actual_heatbed = ExtUI::getActualTemp_celsius(ExtUI::heater_t::BED);
	float target_heatbed = ExtUI::getTargetTemp_celsius(ExtUI::heater_t::BED);
	*/

    float actual_nozzle = thermalManager.degHotend(0);
    float target_nozzle = thermalManager.degTargetHotend(0);
    float actual_heatbed = thermalManager.degBed();
    float target_heatbed = thermalManager.degTargetBed();

    heating_nozzle = (target_nozzle > (actual_nozzle + HEATING_DIFFERENCE));
    cooling_nozzle = (target_nozzle < (actual_nozzle - HEATING_DIFFERENCE) && actual_nozzle > 50);
    heating_heatbed = (target_heatbed > (actual_heatbed + HEATING_DIFFERENCE));
    cooling_heatbed = (target_heatbed < (actual_heatbed - HEATING_DIFFERENCE) && actual_heatbed > 45);

    if (heating_nozzle && target_nozzle != footer->nozzle) {
        footer->nozzle = target_nozzle;
    } else if (!heating_nozzle && actual_nozzle != footer->nozzle) {
        footer->nozzle = actual_nozzle;
    }
    sprintf(footer->text_nozzle, "%.0f/%.0f\177C", (double)actual_nozzle, (double)target_nozzle);
    window_set_text(footer->wt_nozzle.win.id, footer->text_nozzle);

    if (heating_heatbed && target_heatbed != footer->heatbed) {
        footer->heatbed = target_heatbed;
    } else if (!heating_heatbed && actual_heatbed != footer->heatbed) {
        footer->heatbed = actual_heatbed;
    }
    sprintf(footer->text_heatbed, "%.0f/%.0f\177C", (double)actual_heatbed, (double)target_heatbed);
    window_set_text(footer->wt_heatbed.win.id, footer->text_heatbed);

#ifdef LCD_HEATBREAK_TO_FILAMENT
    float actual_heatbreak = thermalManager.degHeatbreak();
    //float actual_heatbreak = analogRead(6);
    sprintf(footer->text_heatbreak, "%.0f\177C", (double)actual_heatbreak);
    window_set_text(footer->wt_filament.win.id, footer->text_heatbreak);
#endif //LCD_HEATBREAK_TO_FILAMENT
}

void status_footer_update_feedrate(status_footer_t *footer) {
    if ((uint16_t)feedrate_percentage <= 999)
        snprintf(footer->text_prnspeed, sizeof(footer->text_prnspeed) / sizeof(footer->text_prnspeed[0]), "%d%%", feedrate_percentage);
    else
        snprintf(footer->text_prnspeed, sizeof(footer->text_prnspeed) / sizeof(footer->text_prnspeed[0]), "ERR");
    window_set_text(footer->wt_prnspeed.win.id, footer->text_prnspeed);
}

void status_footer_update_z_axis(status_footer_t *footer) {
    sprintf(footer->text_z_axis, "%.2f", (double)current_position[2]);
    window_set_text(footer->wt_z_axis.win.id, footer->text_z_axis);
}

void status_footer_update_filament(status_footer_t *footer) {
    window_set_text(footer->wt_filament.win.id, filaments[get_filament()].name);
#ifndef LCD_HEATBREAK_TO_FILAMENT
    window_set_text(footer->wt_filament.win.id, filaments[get_filament()].name);
#endif
}

void status_footer_indicate_nozzle(window_text_t *nozzle, bool heating, bool cooling) {
    if (window_get_color_text(nozzle->win.id) == COLOR_ORANGE
        || window_get_color_text(nozzle->win.id) == COLOR_BLUE) {
        window_set_color_text(nozzle->win.id, COLOR_WHITE);
    } else if (heating) {
        window_set_color_text(nozzle->win.id, COLOR_ORANGE);
    } else if (cooling) {
        window_set_color_text(nozzle->win.id, COLOR_BLUE);
    }
}

void status_footer_indicate_heatbed(window_text_t *heatbed, bool heating, bool cooling) {
    if (window_get_color_text(heatbed->win.id) == COLOR_ORANGE
        || window_get_color_text(heatbed->win.id) == COLOR_BLUE) {
        window_set_color_text(heatbed->win.id, COLOR_WHITE);
    } else if (heating) {
        window_set_color_text(heatbed->win.id, COLOR_ORANGE);
    } else if (cooling) {
        window_set_color_text(heatbed->win.id, COLOR_BLUE);
    }
}

/*
 * status_footer.cpp
 *
 *  Created on: 22. 7. 2019
 *      Author: mcbig
 */

#include "config.h"
#include "status_footer.h"
#include "filament.h"
#include "marlin_vars.h"
#include "marlin_client.h"

#include "stm32f4xx_hal.h"

// #include "../Marlin/src/module/temperature.h"
// #include "../Marlin/src/module/planner.h"

#define HEATING_DIFFERENCE 2

void status_footer_timer(status_footer_t *footer, uint32_t mseconds);
void status_footer_update_temperatures(status_footer_t *footer);
void status_footer_update_feedrate(status_footer_t *footer);
void status_footer_update_z_axis(status_footer_t *footer);
void status_footer_update_filament(status_footer_t *footer);
void status_footer_repaint_nozzle(const status_footer_t *footer);
void status_footer_repaint_heatbed(const status_footer_t *footer);

void status_footer_init(status_footer_t *footer, int16_t parent) {
    footer->show_second_color = false;
    int16_t id;

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
    window_set_text(id, "0/0\177C");

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
    window_set_text(id, "0/0\177C");

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
    window_set_text(id, "0%");

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
    window_set_text(id, "999.95");

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

    footer->last_timer_repaint_values = 0;
    footer->last_timer_repaint_z_pos = 0;
    footer->last_timer_repaint_colors = 0;

    //read and draw real values
    status_footer_update_temperatures(footer);
    status_footer_update_feedrate(footer);
    status_footer_update_filament(footer);
    status_footer_update_z_axis(footer);
    status_footer_repaint_nozzle(footer);
    status_footer_repaint_heatbed(footer);
}

int status_footer_event(status_footer_t *footer, window_t *window,
    uint8_t event, const void *param) {
    status_footer_timer(footer, HAL_GetTick());

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

/// Callback function which triggers update and repaint of values
void status_footer_timer(status_footer_t *footer, uint32_t mseconds) {
    if (mseconds - footer->last_timer_repaint_values >= REPAINT_VALUE_PERIOD) {
        status_footer_update_temperatures(footer);
        status_footer_update_feedrate(footer);
        status_footer_update_filament(footer);
        footer->last_timer_repaint_values = mseconds;
    }

    if (mseconds - footer->last_timer_repaint_z_pos >= REPAINT_Z_POS_PERIOD) {
        status_footer_update_z_axis(footer);
        footer->last_timer_repaint_z_pos = mseconds;
    }

    if ((mseconds - footer->last_timer_repaint_colors) >= BLINK_PERIOD) {
        footer->show_second_color = !footer->show_second_color;
        status_footer_repaint_nozzle(footer);
        status_footer_repaint_heatbed(footer);
        footer->last_timer_repaint_colors = mseconds;
    }
}

void status_footer_update_nozzle(status_footer_t *footer, const marlin_vars_t *vars) {
    if (footer->nozzle == vars->temp_nozzle
        && footer->nozzle_target == vars->target_nozzle
        && footer->nozzle_target_display == vars->display_nozzle)
        return;

    /// nozzle state
    if (vars->target_nozzle != vars->display_nozzle) { /// preheat mode
        footer->nozzle_state = PREHEAT;
        if (vars->target_nozzle > vars->temp_nozzle + HEATING_DIFFERENCE) {
            footer->nozzle_state = HEATING;
        } else if (vars->display_nozzle < vars->temp_nozzle - HEATING_DIFFERENCE) {
            // vars->display_nozzle (not target_nozzle) is OK, because it's weird to show 200/215 and cooling color
            footer->nozzle_state = COOLING;
        }
    } else {
        footer->nozzle_state = STABLE;
        if (vars->target_nozzle > vars->temp_nozzle + HEATING_DIFFERENCE) {
            footer->nozzle_state = HEATING;
        } else if (vars->target_nozzle < vars->temp_nozzle - HEATING_DIFFERENCE && vars->temp_nozzle > COOL_NOZZLE) {
            footer->nozzle_state = COOLING;
        }
    }

    /// update values
    footer->nozzle = vars->temp_nozzle;
    footer->nozzle_target = vars->target_nozzle;
    footer->nozzle_target_display = vars->display_nozzle;

    if (0 < snprintf(footer->text_nozzle, TEXT_LENGTH_NOZZLE, "%d/%d\177C", (int)vars->temp_nozzle, (int)vars->display_nozzle))
        window_set_text(footer->wt_nozzle.win.id, footer->text_nozzle);
}

void status_footer_update_heatbed(status_footer_t *footer, const marlin_vars_t *vars) {
    if (footer->heatbed == vars->temp_bed && footer->heatbed_target == vars->target_bed)
        return;

    /// heatbed state
    footer->heatbed_state = STABLE;
    if (vars->target_bed > vars->temp_bed + HEATING_DIFFERENCE) {
        footer->heatbed_state = HEATING;
    } else if (vars->target_bed < vars->temp_bed - HEATING_DIFFERENCE && vars->temp_bed > COOL_BED) {
        footer->heatbed_state = COOLING;
    }

    /// update values
    footer->heatbed = vars->temp_bed;
    footer->heatbed_target = vars->target_bed;

    if (0 < snprintf(footer->text_heatbed, TEXT_LENGTH_HEATBED, "%d/%d\177C", (int)vars->temp_bed, (int)vars->target_bed))
        window_set_text(footer->wt_heatbed.win.id, footer->text_heatbed);
}

/// Updates values in footer state from real values and repaint
void status_footer_update_temperatures(status_footer_t *footer) {

    /// force update of temperatures
    uint64_t mask = MARLIN_VAR_MSK(MARLIN_VAR_TEMP_NOZ)
        | MARLIN_VAR_MSK(MARLIN_VAR_TEMP_BED)
        | MARLIN_VAR_MSK(MARLIN_VAR_DTEM_NOZ)
        | MARLIN_VAR_MSK(MARLIN_VAR_TTEM_BED);

    const marlin_vars_t *vars = marlin_update_vars(mask);
    if (!vars)
        return;

    status_footer_update_nozzle(footer, vars);
    status_footer_update_heatbed(footer, vars);

#ifdef LCD_HEATBREAK_TO_FILAMENT
    const float actual_heatbreak = thermalManager.degHeatbreak();
    //float actual_heatbreak = analogRead(6);
    char text[10];
    sprintf(text, "%.0f\177C", (double)actual_heatbreak);
    window_set_text(footer->wt_filament.win.id, footer->text);
#endif //LCD_HEATBREAK_TO_FILAMENT
}

void status_footer_update_feedrate(status_footer_t *footer) {
    const marlin_vars_t *vars = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_PRNSPEED));
    if (!vars)
        return;

    const uint16_t speed = vars->print_speed;
    if (speed == footer->print_speed)
        return;

    footer->print_speed = speed;
    if (0 < speed && speed <= 999)
        snprintf(footer->text_prnspeed, sizeof(footer->text_prnspeed) / sizeof(footer->text_prnspeed[0]), "%3d%%", speed);
    else
        snprintf(footer->text_prnspeed, sizeof(footer->text_prnspeed) / sizeof(footer->text_prnspeed[0]), "ERR");
    window_set_text(footer->wt_prnspeed.win.id, footer->text_prnspeed);
}

void status_footer_update_z_axis(status_footer_t *footer) {
    const marlin_vars_t *vars = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_POS_Z));
    if (!vars)
        return;

    const float pos = vars->pos[2];
    if (pos == footer->z_pos)
        return;

    footer->z_pos = pos;
    snprintf(footer->text_z_axis, sizeof(footer->text_z_axis) / sizeof(footer->text_z_axis[0]), "%.2f", (double)pos);
    window_set_text(footer->wt_z_axis.win.id, footer->text_z_axis);
}

void status_footer_update_filament(status_footer_t *footer) {
    if (0 == strcmp(footer->filament, filaments[get_filament()].name))
        return;

    strncpy(footer->filament, filaments[get_filament()].name, TEXT_LENGTH_FILAMENT);
    window_set_text(footer->wt_filament.win.id, footer->filament);
    // #ifndef LCD_HEATBREAK_TO_FILAMENT
    //     window_set_text(footer->wt_filament.win.id, filaments[get_filament()].name);
    // #endif
}

/// Repaints nozzle temperature in proper color
void status_footer_repaint_nozzle(const status_footer_t *footer) {
    color_t clr = DEFAULT_COLOR;

    switch (footer->nozzle_state) {
    case HEATING:
        clr = footer->show_second_color ? HEATING_COLOR : DEFAULT_COLOR;
        break;
    case COOLING:
        clr = footer->show_second_color ? COOLING_COLOR : DEFAULT_COLOR;
        break;
    case PREHEAT:
        clr = footer->show_second_color ? PREHEAT_COLOR : DEFAULT_COLOR;
        break;
    case STABLE:
        clr = STABLE_COLOR;
        break;
    default:
        clr = DEFAULT_COLOR;
    }

    window_set_color_text(footer->wt_nozzle.win.id, clr);
}

/// Repaints heatbed temperature in proper color
void status_footer_repaint_heatbed(const status_footer_t *footer) {
    color_t clr = DEFAULT_COLOR;

    switch (footer->heatbed_state) {
    case HEATING:
        clr = footer->show_second_color ? HEATING_COLOR : DEFAULT_COLOR;
        break;
    case COOLING:
        clr = footer->show_second_color ? COOLING_COLOR : DEFAULT_COLOR;
        break;
    case STABLE:
        clr = STABLE_COLOR;
        break;
    default:
        clr = DEFAULT_COLOR;
    }

    window_set_color_text(footer->wt_heatbed.win.id, clr);
}

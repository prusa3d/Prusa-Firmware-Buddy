
#include <cmath>
#include <algorithm>
#include "math.h"
#include "limits.h"

/// don't draw above line specified in gui.c
/// FIXME footer should receive window to know where to draw

#include "config.h"
#include "status_footer.h"
#include "filament.hpp"
#include "marlin_client.h"
#include "stm32f4xx_hal.h"
#include "cmath_ext.h"
#include "eeprom.h"
#include "screen_home.hpp"
static const float heating_difference = 2.5F;

/*enum class ButtonStatus {
    Nozzle = 0xf0,
    Heatbed,
    PrnSpeed,
    Z_axis,
    Filament
};*/

/// these texts have to be stored here
/// because window_text_t->SetText does not copy the text
/// and windows have delayed redrawing
static char text_nozzle[10];                       // "215/215°C"
static char text_heatbed[10];                      // "110/110°C"
static char text_prnspeed[5];                      // "999%"
static char text_z_profile[MAX_SHEET_NAME_LENGTH]; // "999.95", more space than needed to avoid warning (sprintf)
static const char emptystr[1] = "";
static const char *filament; // "PETG"
static char const *err = "ERR";

/// Callback function which triggers update and repaint of values
void status_footer_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    uint32_t mseconds = HAL_GetTick();

    if (mseconds - last_timer_repaint_values >= REPAINT_VALUE_PERIOD) {
        update_temperatures();
        update_feedrate();
        update_filament();
        last_timer_repaint_values = mseconds;
    }

    if (dynamic_cast<screen_home_data_t *>(GetParent()) != nullptr //is home_screen
        && sheet_number_of_calibrated() > 1) {                     // calibrated more profiles than 1
        update_sheet_profile();
    } else if (mseconds - last_timer_repaint_z_pos >= REPAINT_Z_POS_PERIOD) {
        update_z_axis();
        last_timer_repaint_z_pos = mseconds;
    }

    if ((mseconds - last_timer_repaint_colors) >= BLINK_PERIOD) {
        show_second_color = !show_second_color;
        repaint_nozzle();
        repaint_heatbed();
        last_timer_repaint_colors = mseconds;
    }
}

void status_footer_t::update_nozzle(const marlin_vars_t *vars) {
    if (nozzle == vars->temp_nozzle
        && nozzle_target == vars->target_nozzle
        && nozzle_target_display == vars->display_nozzle)
        return;

    /// nozzle state
    if (nearlyEqual(vars->target_nozzle, PREHEAT_TEMP, 0.4999f) && vars->display_nozzle > vars->target_nozzle) { /// preheat mode
        nozzle_state = HeatState::PREHEAT;
        if (vars->target_nozzle > vars->temp_nozzle + heating_difference) {
            nozzle_state = HeatState::HEATING;
        } else if (vars->display_nozzle < vars->temp_nozzle - heating_difference) {
            // vars->display_nozzle (not target_nozzle) is OK, because it's weird to show 200/215 and cooling color
            nozzle_state = HeatState::COOLING;
        }
        nozzle_target_display = vars->display_nozzle;
    } else {
        nozzle_state = HeatState::STABLE;
        if (vars->target_nozzle > vars->temp_nozzle + heating_difference) {
            nozzle_state = HeatState::HEATING;
        } else if (vars->target_nozzle < vars->temp_nozzle - heating_difference && vars->temp_nozzle > COOL_NOZZLE) {
            nozzle_state = HeatState::COOLING;
        }
        nozzle_target_display = vars->target_nozzle;
    }

    /// update values
    nozzle = vars->temp_nozzle;
    nozzle_target = vars->target_nozzle;

    if (0 < snprintf(text_nozzle, sizeof(text_nozzle), "%d/%d\177C", (int)roundf(vars->temp_nozzle), (int)roundf(nozzle_target_display))) {
        // this MakeRAM is safe - text_nozzle is statically allocated
        wt_nozzle.SetText(string_view_utf8::MakeRAM((const uint8_t *)text_nozzle));
    }
}

void status_footer_t::update_heatbed(const marlin_vars_t *vars) {
    if (heatbed == vars->temp_bed && heatbed_target == vars->target_bed)
        return;

    /// heatbed state
    heatbed_state = HeatState::STABLE;
    if (vars->target_bed > vars->temp_bed + heating_difference) {
        heatbed_state = HeatState::HEATING;
    } else if (vars->target_bed < vars->temp_bed - heating_difference && vars->temp_bed > COOL_BED) {
        heatbed_state = HeatState::COOLING;
    }

    /// update values
    heatbed = vars->temp_bed;
    heatbed_target = vars->target_bed;

    if (0 < snprintf(text_heatbed, sizeof(text_heatbed), "%d/%d\177C", (int)roundf(vars->temp_bed), (int)roundf(vars->target_bed))) {
        // this MakeRAM is safe - text_heatbed is statically allocated
        wt_heatbed.SetText(string_view_utf8::MakeRAM((const uint8_t *)text_heatbed));
    }
}

/// Updates values in footer state from real values and repaint
void status_footer_t::update_temperatures() {
    const marlin_vars_t *vars = marlin_vars();
    if (!vars)
        return;

    update_nozzle(vars);
    update_heatbed(vars);
}

void status_footer_t::update_feedrate() {
    const marlin_vars_t *vars = marlin_vars();
    if (!vars) {
        snprintf(text_prnspeed, sizeof(text_prnspeed), err);
        return;
    }

    const uint16_t speed = vars->print_speed;
    if (speed == print_speed)
        return;

    print_speed = speed;
    if (0 < speed && speed <= 999)
        snprintf(text_prnspeed, sizeof(text_prnspeed), "%3d%%", speed);
    else
        snprintf(text_prnspeed, sizeof(text_prnspeed), err);
    // this MakeRAM is safe - text_prnspeed is statically allocated
    wt_prnspeed.SetText(string_view_utf8::MakeRAM((const uint8_t *)text_prnspeed));
}

void status_footer_t::update_z_axis() {
    const marlin_vars_t *vars = marlin_vars();
    if (!vars) {
        wt_z_profile.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)err));
        return;
    }

    const int32_t pos = (int32_t)round(vars->pos[2] * 100); // convert to 000.00 fix point number;
    if (pos == z_pos)
        return;

    z_pos = pos;
    if (0 > snprintf(text_z_profile, sizeof(text_z_profile), "%d.%02d", (int)(pos / 100), (int)std::abs(pos % 100))) {
        wt_z_profile.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)err));
        return;
    }
    // this MakeRAM is safe, text_z_axis is preallocated in RAM
    wt_z_profile.SetText(string_view_utf8::MakeRAM((const uint8_t *)text_z_profile));
    wi_z_profile.SetIdRes(IDR_PNG_z_axis_16px);
}

void status_footer_t::update_filament() {
    if (0 == strcmp(filament, filaments[size_t(get_filament())].name))
        return;

    filament = filaments[size_t(get_filament())].name;
    wt_filament.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)filament));
}

/// Repaints nozzle temperature in proper color
void status_footer_t::repaint_nozzle() {
    color_t clr = DEFAULT_COLOR;

    switch (nozzle_state) {
    case HeatState::HEATING:
        clr = show_second_color ? HEATING_COLOR : DEFAULT_COLOR;
        break;
    case HeatState::COOLING:
        clr = show_second_color ? COOLING_COLOR : DEFAULT_COLOR;
        break;
    case HeatState::PREHEAT:
        clr = show_second_color ? PREHEAT_COLOR : DEFAULT_COLOR;
        break;
    case HeatState::STABLE:
        clr = STABLE_COLOR;
        break;
    default:
        clr = DEFAULT_COLOR;
    }

    wt_nozzle.SetTextColor(clr);
}

/// Repaints heatbed temperature in proper color
void status_footer_t::repaint_heatbed() {
    color_t clr = DEFAULT_COLOR;

    switch (heatbed_state) {
    case HeatState::HEATING:
        clr = show_second_color ? HEATING_COLOR : DEFAULT_COLOR;
        break;
    case HeatState::COOLING:
        clr = show_second_color ? COOLING_COLOR : DEFAULT_COLOR;
        break;
    case HeatState::STABLE:
        clr = STABLE_COLOR;
        break;
    default:
        clr = DEFAULT_COLOR;
    }

    wt_heatbed.SetTextColor(clr);
}

void status_footer_t::update_sheet_profile() {
    sheet_active_name(text_z_profile, MAX_SHEET_NAME_LENGTH);
    wt_z_profile.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)text_z_profile));
    wi_z_profile.SetIdRes(IDR_PNG_sheet_profile);
}

status_footer_t::status_footer_t(window_t *parent)
    : AddSuperWindow<window_frame_t>(parent, GuiDefaults::RectFooter)
    , wi_nozzle(this, Rect16(8, 270, 16, 16), IDR_PNG_nozzle_16px)
    , wi_heatbed(this, Rect16(128, 270, 20, 16), IDR_PNG_heatbed_16px)
    , wi_prnspeed(this, Rect16(10, 297, 16, 12), IDR_PNG_speed_16px)
    , wi_z_profile(this, Rect16(74, 297, 16, 16), IDR_NULL)
    , wi_filament(this, Rect16(163, 297, 16, 16), IDR_PNG_spool_16px)
    , wt_nozzle(this, Rect16(24, 269, 85, 20), is_multiline::no)
    , wt_heatbed(this, Rect16(150, 269, 85, 22), is_multiline::no)
    , wt_prnspeed(this, Rect16(28, 296, 40, 22), is_multiline::no)
    , wt_z_profile(this, Rect16(92, 296, 68, 22), is_multiline::no)
    , wt_filament(this, Rect16(181, 296, 49, 22), is_multiline::no)
    , nozzle(-273)
    , nozzle_target(-273)
    , nozzle_target_display(-273)
    , heatbed(-273)
    , heatbed_target(-273)
    , z_pos(INT_MIN)
    , last_timer_repaint_values(0)
    , last_timer_repaint_colors(0)
    , last_timer_repaint_z_pos(0)
    , print_speed(0) /// print speed in percents
    //, nozzle_state;
    //, heatbed_state;
    , show_second_color(false) {

    wt_nozzle.font = resource_font(IDR_FNT_SPECIAL);
    wt_nozzle.SetAlignment(ALIGN_CENTER);
    wt_nozzle.SetText(string_view_utf8::MakeNULLSTR());

    wt_heatbed.font = resource_font(IDR_FNT_SPECIAL);
    wt_heatbed.SetAlignment(ALIGN_CENTER);
    wt_heatbed.SetText(string_view_utf8::MakeNULLSTR());

    wt_prnspeed.font = resource_font(IDR_FNT_SPECIAL);
    wt_prnspeed.SetAlignment(ALIGN_CENTER);
    wt_prnspeed.SetText(string_view_utf8::MakeNULLSTR());

    wt_z_profile.font = resource_font(IDR_FNT_SPECIAL);
    wt_z_profile.SetAlignment(ALIGN_CENTER);
    wt_z_profile.SetText(string_view_utf8::MakeNULLSTR());

    wt_filament.font = resource_font(IDR_FNT_SPECIAL);
    wt_filament.SetAlignment(ALIGN_CENTER);
    wt_filament.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)filaments[size_t(get_filament())].name));

    filament = emptystr;

    //read and draw real values
    update_temperatures();
    update_feedrate();
    update_filament();
    if (sheet_number_of_calibrated() > 1)
        update_sheet_profile();
    else
        update_z_axis();
    repaint_nozzle();
    repaint_heatbed();

    Disable();
}

// status_footer.h
#pragma once

#include "gui.hpp"
#include "marlin_vars.h"

enum class HeatState : uint8_t {
    HEATING,
    COOLING,
    PREHEAT,
    STABLE,
};

class status_footer_t : public AddSuperWindow<window_frame_t> {
    window_icon_t wi_nozzle;
    window_icon_t wi_heatbed;
    window_icon_t wi_prnspeed;
    window_icon_t wi_z_profile;
    window_icon_t wi_filament;

    window_text_t wt_nozzle;
    window_text_t wt_heatbed;
    window_text_t wt_prnspeed;
    window_text_t wt_z_profile;
    window_text_t wt_filament;

    float nozzle;                /// current temperature of nozzle
    float nozzle_target;         /// target temperature of nozzle (not shown)
    float nozzle_target_display; /// target temperature of nozzle shown on display
    float heatbed;               /// current temperature of bed
    float heatbed_target;        /// target temperature of bed

    int32_t z_pos; /// z position, 000.00 fixed point
    uint32_t last_timer_repaint_values;
    uint32_t last_timer_repaint_colors;
    uint32_t last_timer_repaint_z_pos;
    uint32_t last_timer_repaint_profile;
    uint16_t print_speed; /// print speed in percents
    HeatState nozzle_state;
    HeatState heatbed_state;
    bool show_second_color;
    uint32_t cached_no_of_calibrated_sheets;

    void timer(uint32_t mseconds);
    void update_nozzle(const marlin_vars_t *vars);
    void update_heatbed(const marlin_vars_t *vars);
    void update_temperatures();
    void update_feedrate();
    void update_z_axis();
    void update_filament();
    void repaint_nozzle();
    void repaint_heatbed();
    void update_sheet_profile();
    uint32_t no_of_calibrated_sheets();

public:
    status_footer_t(window_t *parent);

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

static const uint16_t REPAINT_Z_POS_PERIOD = 256;    /// time span between z position repaint [miliseconds]
static const uint16_t REPAINT_VALUE_PERIOD = 1024;   /// time span between value repaint [miliseconds]
static const uint16_t BLINK_PERIOD = 512;            /// time span between color changes [miliseconds]
static const uint16_t REPAINT_PROFILE_PERIOD = 1024; /// time span between profile changes [miliseconds]

static const uint8_t COOL_NOZZLE = 50; /// highest temperature of nozzle to be considered as cool
static const uint8_t COOL_BED = 45;    /// highest temperature of bed to be considered as cool

static const color_t DEFAULT_COLOR = color_t::White;
static const color_t STABLE_COLOR = color_t::White;
static const color_t HEATING_COLOR = color_t::Orange;
static const color_t COOLING_COLOR = color_t::Blue;
static const color_t PREHEAT_COLOR = color_t::Green;

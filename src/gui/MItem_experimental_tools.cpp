/**
 * @file MItem_experimental_tools.cpp
 * @author Radek Vana
 * @date 2021-08-03
 */
#include "MItem_experimental_tools.hpp"
#include "WindowMenuSpin.hpp"
#include "ScreenHandler.hpp"
#include "img_resources.hpp"
#include <gui/menu_vars.h>

#if PRINTER_IS_PRUSA_MK3_5()
/*****************************************************************************/
// MI_ALT_FAN
bool MI_ALT_FAN::init_index() {
    return config_store().has_alt_fans.get();
}

void MI_ALT_FAN::OnChange([[maybe_unused]] size_t old_index) {
    config_store().has_alt_fans.set(!config_store().has_alt_fans.get());
}
#endif

/*****************************************************************************/
// MI_Z_AXIS_LEN
static constexpr NumericInputConfig z_axis_len_spin_config {
    .min_value = Z_MIN_LEN_LIMIT,
    .max_value = Z_MAX_LEN_LIMIT,
    .unit = Unit::millimeter,
};

MI_Z_AXIS_LEN::MI_Z_AXIS_LEN()
    : WiSpin(get_z_max_pos_mm_rounded(), z_axis_len_spin_config, _("Z-axis length")) {}

void MI_Z_AXIS_LEN::Store() {
    set_z_max_pos_mm(GetVal());
}

/*****************************************************************************/
// MI_RESET_Z_AXIS_LEN
MI_RESET_Z_AXIS_LEN::MI_RESET_Z_AXIS_LEN()
    : IWindowMenuItem(_("Reset Z-length")) {}

void MI_RESET_Z_AXIS_LEN::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Reset_Z);
}

namespace {

constexpr NumericInputConfig steps_per_unit_xy_spin_config {
    .min_value = 1,
    .max_value = 1000,
    .special_value = config_store_ns::steps_per_unit_unset,
    .special_value_str = N_("Default"),
    .max_decimal_places = 2,
};

constexpr NumericInputConfig steps_per_unit_spin_config {
    .min_value = 1,
    .max_value = 1000,
    .max_decimal_places = 2,
};

float steps_per_mm_to_val(auto &store_item) {
    // std::abs would not work if the unset val is anything else
    static_assert(config_store_ns::steps_per_unit_unset == 0);
    return std::abs(store_item.get());
}

[[maybe_unused]] float value_to_store(float val, const WiSwitchDirection &wrong_direction_item, float default_val) {
    const bool wrong_direction = wrong_direction_item.current_item() == 1;

    if (val == config_store_ns::steps_per_unit_unset && wrong_direction) {
        // Enforce value if wrong_dir is set, otherwise -0 == 0 and wrong_dir would not get stored
        val = default_val;
    } else {
        val = std::copysignf(val, default_val);
    }

    return val * (wrong_direction ? -1 : 1);
}

} // namespace

#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
/*****************************************************************************/
// MI_STEPS_PER_UNIT_X
MI_STEPS_PER_UNIT_X::MI_STEPS_PER_UNIT_X()
    : WiSpin(steps_per_mm_to_val(config_store().axis_steps_per_unit_x), steps_per_unit_xy_spin_config, _("X-axis steps per unit")) {
}

float MI_STEPS_PER_UNIT_X::value_to_store(const MI_DIRECTION_X &wrong_direction) const {
    return ::value_to_store(value(), wrong_direction, get_default_steps_per_unit_x_signed());
}

void MI_STEPS_PER_UNIT_X::Store(const MI_DIRECTION_X &wrong_direction) {
    config_store().axis_steps_per_unit_x.set(value_to_store(wrong_direction));
}

/*****************************************************************************/
// MI_STEPS_PER_UNIT_Y
MI_STEPS_PER_UNIT_Y::MI_STEPS_PER_UNIT_Y()
    : WiSpin(steps_per_mm_to_val(config_store().axis_steps_per_unit_y), steps_per_unit_xy_spin_config, _("Y-axis steps per unit")) {}

float MI_STEPS_PER_UNIT_Y::value_to_store(const MI_DIRECTION_Y &wrong_direction) const {
    return ::value_to_store(value(), wrong_direction, get_default_steps_per_unit_y_signed());
}

void MI_STEPS_PER_UNIT_Y::Store(const MI_DIRECTION_Y &wrong_direction) {
    config_store().axis_steps_per_unit_y.set(value_to_store(wrong_direction));
}

/*****************************************************************************/
// MI_STEPS_PER_UNIT_Z
MI_STEPS_PER_UNIT_Z::MI_STEPS_PER_UNIT_Z()
    : WiSpin(get_steps_per_unit_z(), steps_per_unit_spin_config, _("Z-axis steps per unit")) {}

void MI_STEPS_PER_UNIT_Z::Store() {
    set_steps_per_unit_z(GetVal());
}
#endif

/*****************************************************************************/
// MI_STEPS_PER_UNIT_E
MI_STEPS_PER_UNIT_E::MI_STEPS_PER_UNIT_E()
    : WiSpin(get_steps_per_unit_e(), steps_per_unit_spin_config, _("Extruder steps per unit")) {}

void MI_STEPS_PER_UNIT_E::Store() {
    set_steps_per_unit_e(GetVal());
}

/*****************************************************************************/
// MI_RESET_STEPS_PER_UNIT
MI_RESET_STEPS_PER_UNIT::MI_RESET_STEPS_PER_UNIT()
    : IWindowMenuItem(_("Reset steps per unit")) {}

void MI_RESET_STEPS_PER_UNIT::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Reset_steps);
}

/*****************************************************************************/
// WiSwitchDirection
static constexpr const char *switch_direction_items[] = {
    N_("Prusa"),
    N_("Wrong"),
};

WiSwitchDirection::WiSwitchDirection(bool current_direction_wrong, const string_view_utf8 &label_view)
    : MenuItemSwitch(label_view, switch_direction_items, current_direction_wrong) {}

#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
/*****************************************************************************/
// MI_DIRECTION_X
MI_DIRECTION_X::MI_DIRECTION_X()
    : WiSwitchDirection(has_wrong_x(), _("X-axis direction")) {}

/*****************************************************************************/
// MI_DIRECTION_Y
MI_DIRECTION_Y::MI_DIRECTION_Y()
    : WiSwitchDirection(has_wrong_y(), _("Y-axis direction")) {}

/*****************************************************************************/
// MI_DIRECTION_Z
MI_DIRECTION_Z::MI_DIRECTION_Z()
    : WiSwitchDirection(has_wrong_z(), _("Z-axis direction")) {}

void MI_DIRECTION_Z::Store() {
    get_index() == 1 ? set_wrong_direction_z() : set_PRUSA_direction_z();
}
#endif

/*****************************************************************************/
// MI_DIRECTION_E
MI_DIRECTION_E::MI_DIRECTION_E()
    : WiSwitchDirection(has_wrong_e(), _("Extruder direction")) {}

void MI_DIRECTION_E::Store() {
    get_index() == 1 ? set_wrong_direction_e() : set_PRUSA_direction_e();
}

/*****************************************************************************/
// MI_RESET_DIRECTION
MI_RESET_DIRECTION::MI_RESET_DIRECTION()
    : IWindowMenuItem(_("Reset directions")) {}

void MI_RESET_DIRECTION::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Reset_directions);
}

static constexpr NumericInputConfig rms_current_spin_config = {
    .min_value = 0,
    .max_value = 800,
    .special_value = 0,
    .special_value_str = N_("Default"),
    .unit = Unit::milliamper,
};

#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
/*****************************************************************************/
// MI_CURRENT_X
MI_CURRENT_X::MI_CURRENT_X()
    : WiSpin(config_store().axis_rms_current_ma_X_.get(), rms_current_spin_config, _("X current")) {}

void MI_CURRENT_X::Store() {
    set_rms_current_ma_x(static_cast<uint16_t>(GetVal()));
}

/*****************************************************************************/
// MI_CURRENT_Y
MI_CURRENT_Y::MI_CURRENT_Y()
    : WiSpin(config_store().axis_rms_current_ma_Y_.get(), rms_current_spin_config, _("Y current")) {}

void MI_CURRENT_Y::Store() {
    set_rms_current_ma_y(static_cast<uint16_t>(GetVal()));
}

/*****************************************************************************/
// MI_CURRENT_Z
MI_CURRENT_Z::MI_CURRENT_Z()
    : WiSpin(config_store().axis_rms_current_ma_Z_.get(), rms_current_spin_config, _("Z current")) {}

void MI_CURRENT_Z::Store() {
    set_rms_current_ma_z(static_cast<uint16_t>(GetVal()));
}

/*****************************************************************************/
// MI_CURRENT_E
MI_CURRENT_E::MI_CURRENT_E()
    : WiSpin(config_store().axis_rms_current_ma_E0_.get(), rms_current_spin_config, _("Extruder current")) {}

void MI_CURRENT_E::Store() {
    set_rms_current_ma_e(static_cast<uint16_t>(GetVal()));
}
#endif

#if HAS_EXTRA_EXPERIMENTAL_SETTINGS()
/*****************************************************************************/
// MI_RESET_CURRENTS
MI_RESET_CURRENTS::MI_RESET_CURRENTS()
    : IWindowMenuItem(_("Reset currents")) {}

void MI_RESET_CURRENTS::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Reset_currents);
}
#endif

/*****************************************************************************/
// MI_SAVE_AND_RETURN
MI_SAVE_AND_RETURN::MI_SAVE_AND_RETURN()
    : IWindowMenuItem(_("Save and return"), &img::folder_up_16x16, is_enabled_t::yes, is_hidden_t::no) {
    has_return_behavior_ = true;
}

void MI_SAVE_AND_RETURN::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Return);
}

#if HAS_ILI9488_DISPLAY()
/*****************************************************************************/
// MI_FAST_DRAW_ENABLE
// If this is put outside of ScreenMenuExperimental (that resets the printer
// after exiting), the config_store().fast_draw_enabled usage in ili9488
// must be reworked to not store the result in a static variable.
MI_FAST_DRAW_ENABLE::MI_FAST_DRAW_ENABLE()
    : WI_ICON_SWITCH_OFF_ON_t {
        config_store().fast_draw_enabled.get(),
        // translation: experimental menu item enabling faster display routines
        _("Fast Draw"),
    } {
}
void MI_FAST_DRAW_ENABLE::Store() {
    config_store().fast_draw_enabled.set(value());
}
#endif

/**
 * @file MItem_experimental_tools.cpp
 * @author Radek Vana
 * @date 2021-08-03
 */
#include "MItem_experimental_tools.hpp"
#include "WindowMenuSpin.hpp"
#include "ScreenHandler.hpp"
#include "string.h" // memcmp
#include "img_resources.hpp"
#include <gui/menu_vars.h>

#define NOTRAN(x) string_view_utf8::MakeCPUFLASH((const uint8_t *)x)

#if PRINTER_IS_PRUSA_MK3_5()
/*****************************************************************************/
// MI_ALT_FAN_CORRECTION
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
    : WiSpin(get_z_max_pos_mm_rounded(), z_axis_len_spin_config, NOTRAN(label)) {}

void MI_Z_AXIS_LEN::Store() {
    set_z_max_pos_mm(GetVal());
}

/*****************************************************************************/
// MI_RESET_Z_AXIS_LEN
MI_RESET_Z_AXIS_LEN::MI_RESET_Z_AXIS_LEN()
    : IWindowMenuItem(NOTRAN(label)) {}

void MI_RESET_Z_AXIS_LEN::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Reset_Z);
}

static constexpr NumericInputConfig steps_per_unit_spin_config = {
    .min_value = 1,
    .max_value = 1000,
};

/*****************************************************************************/
// MI_STEPS_PER_UNIT_X
MI_STEPS_PER_UNIT_X::MI_STEPS_PER_UNIT_X()
    : WiSpin(get_steps_per_unit_x_rounded(), steps_per_unit_spin_config, NOTRAN(label)) {}

void MI_STEPS_PER_UNIT_X::Store() {
    set_steps_per_unit_x(GetVal());
}

/*****************************************************************************/
// MI_STEPS_PER_UNIT_Y
MI_STEPS_PER_UNIT_Y::MI_STEPS_PER_UNIT_Y()
    : WiSpin(get_steps_per_unit_y_rounded(), steps_per_unit_spin_config, NOTRAN(label)) {}

void MI_STEPS_PER_UNIT_Y::Store() {
    set_steps_per_unit_y(GetVal());
}

/*****************************************************************************/
// MI_STEPS_PER_UNIT_Z
MI_STEPS_PER_UNIT_Z::MI_STEPS_PER_UNIT_Z()
    : WiSpin(get_steps_per_unit_z_rounded(), steps_per_unit_spin_config, NOTRAN(label)) {}

void MI_STEPS_PER_UNIT_Z::Store() {
    set_steps_per_unit_z(GetVal());
}

/*****************************************************************************/
// MI_STEPS_PER_UNIT_E
MI_STEPS_PER_UNIT_E::MI_STEPS_PER_UNIT_E()
    : WiSpin(get_steps_per_unit_e_rounded(), steps_per_unit_spin_config, NOTRAN(label)) {}

void MI_STEPS_PER_UNIT_E::Store() {
    set_steps_per_unit_e(GetVal());
}

/*****************************************************************************/
// MI_RESET_STEPS_PER_UNIT
MI_RESET_STEPS_PER_UNIT::MI_RESET_STEPS_PER_UNIT()
    : IWindowMenuItem(NOTRAN(label)) {}

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

/*****************************************************************************/
// MI_DIRECTION_X
MI_DIRECTION_X::MI_DIRECTION_X()
    : WiSwitchDirection(has_wrong_x(), NOTRAN(label)) {}

void MI_DIRECTION_X::Store() {
    GetIndex() == 1 ? set_wrong_direction_x() : set_PRUSA_direction_x();
}

/*****************************************************************************/
// MI_DIRECTION_Y
MI_DIRECTION_Y::MI_DIRECTION_Y()
    : WiSwitchDirection(has_wrong_y(), NOTRAN(label)) {}

void MI_DIRECTION_Y::Store() {
    GetIndex() == 1 ? set_wrong_direction_y() : set_PRUSA_direction_y();
}

/*****************************************************************************/
// MI_DIRECTION_Z
MI_DIRECTION_Z::MI_DIRECTION_Z()
    : WiSwitchDirection(has_wrong_z(), NOTRAN(label)) {}

void MI_DIRECTION_Z::Store() {
    GetIndex() == 1 ? set_wrong_direction_z() : set_PRUSA_direction_z();
}

/*****************************************************************************/
// MI_DIRECTION_E
MI_DIRECTION_E::MI_DIRECTION_E()
    : WiSwitchDirection(has_wrong_e(), NOTRAN(label)) {}

void MI_DIRECTION_E::Store() {
    GetIndex() == 1 ? set_wrong_direction_e() : set_PRUSA_direction_e();
}

/*****************************************************************************/
// MI_RESET_DIRECTION
MI_RESET_DIRECTION::MI_RESET_DIRECTION()
    : IWindowMenuItem(NOTRAN(label)) {}

void MI_RESET_DIRECTION::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Reset_directions);
}

static constexpr NumericInputConfig rms_current_spin_config = {
    .max_value = 800,
    .unit = Unit::milliamper,
};

/*****************************************************************************/
// MI_CURRENT_X
MI_CURRENT_X::MI_CURRENT_X()
    : WiSpin(config_store().axis_rms_current_ma_X_.get(), rms_current_spin_config, NOTRAN(label)) {}

void MI_CURRENT_X::Store() {
    set_rms_current_ma_x(GetVal());
}

/*****************************************************************************/
// MI_CURRENT_Y
MI_CURRENT_Y::MI_CURRENT_Y()
    : WiSpin(config_store().axis_rms_current_ma_Y_.get(), rms_current_spin_config, NOTRAN(label)) {}

void MI_CURRENT_Y::Store() {
    set_rms_current_ma_y(GetVal());
}

/*****************************************************************************/
// MI_CURRENT_Z
MI_CURRENT_Z::MI_CURRENT_Z()
    : WiSpin(get_rms_current_ma_z(), rms_current_spin_config, NOTRAN(label)) {}

void MI_CURRENT_Z::Store() {
    set_rms_current_ma_z(GetVal());
}

/*****************************************************************************/
// MI_CURRENT_E
MI_CURRENT_E::MI_CURRENT_E()
    : WiSpin(get_rms_current_ma_e(), rms_current_spin_config, NOTRAN(label)) {}

void MI_CURRENT_E::Store() {
    set_rms_current_ma_e(GetVal());
}

/*****************************************************************************/
// MI_RESET_CURRENTS
MI_RESET_CURRENTS::MI_RESET_CURRENTS()
    : IWindowMenuItem(NOTRAN(label)) {}

void MI_RESET_CURRENTS::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Reset_currents);
}

/*****************************************************************************/
// MI_SAVE_AND_RETURN
MI_SAVE_AND_RETURN::MI_SAVE_AND_RETURN()
    : IWindowMenuItem(NOTRAN(label), &img::folder_up_16x16, is_enabled_t::yes, is_hidden_t::no) {
    has_return_behavior_ = true;
}

void MI_SAVE_AND_RETURN::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Return);
}

/**
 * @file MItem_experimental_tools.cpp
 * @author Radek Vana
 * @date 2021-08-03
 */
#include "MItem_experimental_tools.hpp"
#include "menu_spin_config.hpp"
#include "ScreenHandler.hpp"
#include "string.h" // memcmp
#include "img_resources.hpp"

#define NOTRAN(x) string_view_utf8::MakeCPUFLASH((const uint8_t *)x)

#if PRINTER_IS_PRUSA_MK3_5
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
MI_Z_AXIS_LEN::MI_Z_AXIS_LEN()
    : WiSpinInt(get_z_max_pos_mm_rounded(), SpinCnf::axis_z_max_range, NOTRAN(label)) {}

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

/*****************************************************************************/
// MI_STEPS_PER_UNIT_X
MI_STEPS_PER_UNIT_X::MI_STEPS_PER_UNIT_X()
    : WiSpinInt(get_steps_per_unit_x_rounded(), SpinCnf::steps_per_unit, NOTRAN(label)) {}

void MI_STEPS_PER_UNIT_X::Store() {
    set_steps_per_unit_x(GetVal());
}

/*****************************************************************************/
// MI_STEPS_PER_UNIT_Y
MI_STEPS_PER_UNIT_Y::MI_STEPS_PER_UNIT_Y()
    : WiSpinInt(get_steps_per_unit_y_rounded(), SpinCnf::steps_per_unit, NOTRAN(label)) {}

void MI_STEPS_PER_UNIT_Y::Store() {
    set_steps_per_unit_y(GetVal());
}

/*****************************************************************************/
// MI_STEPS_PER_UNIT_Z
MI_STEPS_PER_UNIT_Z::MI_STEPS_PER_UNIT_Z()
    : WiSpinInt(get_steps_per_unit_z_rounded(), SpinCnf::steps_per_unit, NOTRAN(label)) {}

void MI_STEPS_PER_UNIT_Z::Store() {
    set_steps_per_unit_z(GetVal());
}

/*****************************************************************************/
// MI_STEPS_PER_UNIT_E
MI_STEPS_PER_UNIT_E::MI_STEPS_PER_UNIT_E()
    : WiSpinInt(get_steps_per_unit_e_rounded(), SpinCnf::steps_per_unit, NOTRAN(label)) {}

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
WiSwitchDirection::WiSwitchDirection(bool current_direction_wrong, string_view_utf8 label_view)
    : WI_SWITCH_t<2>(current_direction_wrong, label_view, nullptr, is_enabled_t::yes, is_hidden_t::no,
        NOTRAN(str_prusa), NOTRAN(str_wrong)) {
}

/*****************************************************************************/
// MI_DIRECTION_X
MI_DIRECTION_X::MI_DIRECTION_X()
    : WiSwitchDirection(has_wrong_x(), NOTRAN(label)) {}

void MI_DIRECTION_X::Store() {
    index == 1 ? set_wrong_direction_x() : set_PRUSA_direction_x();
}

/*****************************************************************************/
// MI_DIRECTION_Y
MI_DIRECTION_Y::MI_DIRECTION_Y()
    : WiSwitchDirection(has_wrong_y(), NOTRAN(label)) {}

void MI_DIRECTION_Y::Store() {
    index == 1 ? set_wrong_direction_y() : set_PRUSA_direction_y();
}

/*****************************************************************************/
// MI_DIRECTION_Z
MI_DIRECTION_Z::MI_DIRECTION_Z()
    : WiSwitchDirection(has_wrong_z(), NOTRAN(label)) {}

void MI_DIRECTION_Z::Store() {
    index == 1 ? set_wrong_direction_z() : set_PRUSA_direction_z();
}

/*****************************************************************************/
// MI_DIRECTION_E
MI_DIRECTION_E::MI_DIRECTION_E()
    : WiSwitchDirection(has_wrong_e(), NOTRAN(label)) {}

void MI_DIRECTION_E::Store() {
    index == 1 ? set_wrong_direction_e() : set_PRUSA_direction_e();
}

/*****************************************************************************/
// MI_RESET_DIRECTION
MI_RESET_DIRECTION::MI_RESET_DIRECTION()
    : IWindowMenuItem(NOTRAN(label)) {}

void MI_RESET_DIRECTION::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Reset_directions);
}

/*****************************************************************************/
// MI_MICROSTEPS_X
MI_MICROSTEPS_X::MI_MICROSTEPS_X()
    : WiSpinExpWith0(config_store().axis_microsteps_X_.get(), SpinCnf::microstep_exponential_with_0, NOTRAN(label)) {}

void MI_MICROSTEPS_X::Store() {
    set_microsteps_x(GetVal());
}

/*****************************************************************************/
// MI_MICROSTEPS_Y
MI_MICROSTEPS_Y::MI_MICROSTEPS_Y()
    : WiSpinExpWith0(config_store().axis_microsteps_Y_.get(), SpinCnf::microstep_exponential_with_0, NOTRAN(label)) {}

void MI_MICROSTEPS_Y::Store() {
    set_microsteps_y(GetVal());
}

/*****************************************************************************/
// MI_MICROSTEPS_Z
MI_MICROSTEPS_Z::MI_MICROSTEPS_Z()
    : WiSpinExp(get_microsteps_z(), SpinCnf::microstep_exponential, NOTRAN(label)) {}

void MI_MICROSTEPS_Z::Store() {
    set_microsteps_z(GetVal());
}

/*****************************************************************************/
// MI_MICROSTEPS_E
MI_MICROSTEPS_E::MI_MICROSTEPS_E()
    : WiSpinExp(get_microsteps_e(), SpinCnf::microstep_exponential, NOTRAN(label)) {}

void MI_MICROSTEPS_E::Store() {
    set_microsteps_e(GetVal());
}

/*****************************************************************************/
// MI_RESET_MICROSTEPS
MI_RESET_MICROSTEPS::MI_RESET_MICROSTEPS()
    : IWindowMenuItem(NOTRAN(label)) {}

void MI_RESET_MICROSTEPS::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Reset_microsteps);
}

/*****************************************************************************/
// MI_CURRENT_X
MI_CURRENT_X::MI_CURRENT_X()
    : WiSpinInt(config_store().axis_rms_current_ma_X_.get(), SpinCnf::rms_current, NOTRAN(label)) {}

void MI_CURRENT_X::Store() {
    set_rms_current_ma_x(GetVal());
}

/*****************************************************************************/
// MI_CURRENT_Y
MI_CURRENT_Y::MI_CURRENT_Y()
    : WiSpinInt(config_store().axis_rms_current_ma_Y_.get(), SpinCnf::rms_current, NOTRAN(label)) {}

void MI_CURRENT_Y::Store() {
    set_rms_current_ma_y(GetVal());
}

/*****************************************************************************/
// MI_CURRENT_Z
MI_CURRENT_Z::MI_CURRENT_Z()
    : WiSpinInt(get_rms_current_ma_z(), SpinCnf::rms_current, NOTRAN(label)) {}

void MI_CURRENT_Z::Store() {
    set_rms_current_ma_z(GetVal());
}

/*****************************************************************************/
// MI_CURRENT_E
MI_CURRENT_E::MI_CURRENT_E()
    : WiSpinInt(get_rms_current_ma_e(), SpinCnf::rms_current, NOTRAN(label)) {}

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

/**
 * @file MItem_experimental_tools.cpp
 * @author Radek Vana
 * @date 2021-08-03
 */
#include "MItem_experimental_tools.hpp"
#include "eeprom.h"
#include "menu_spin_config.hpp"
#include "ScreenHandler.hpp"
#include "string.h" // memcmp

/*****************************************************************************/
//MI_Z_AXIS_LEN
MI_Z_AXIS_LEN::MI_Z_AXIS_LEN()
    : WiSpinInt(get_z_max_pos_mm_rounded(), SpinCnf::axis_z_max_range, _(label)) {}

/*****************************************************************************/
//MI_RESET_Z_AXIS_LEN
MI_RESET_Z_AXIS_LEN::MI_RESET_Z_AXIS_LEN()
    : WI_LABEL_t(_(label)) {}

void MI_RESET_Z_AXIS_LEN::click(IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Reset_Z);
}

/*****************************************************************************/
//MI_STEPS_PER_UNIT_X
MI_STEPS_PER_UNIT_X::MI_STEPS_PER_UNIT_X()
    : WiSpinInt(get_steps_per_unit_x_rounded(), SpinCnf::steps_per_unit, _(label)) {}

/*****************************************************************************/
//MI_STEPS_PER_UNIT_Y
MI_STEPS_PER_UNIT_Y::MI_STEPS_PER_UNIT_Y()
    : WiSpinInt(get_steps_per_unit_y_rounded(), SpinCnf::steps_per_unit, _(label)) {}

/*****************************************************************************/
//MI_STEPS_PER_UNIT_Z
MI_STEPS_PER_UNIT_Z::MI_STEPS_PER_UNIT_Z()
    : WiSpinInt(get_steps_per_unit_z_rounded(), SpinCnf::steps_per_unit, _(label)) {}

/*****************************************************************************/
//MI_STEPS_PER_UNIT_E
MI_STEPS_PER_UNIT_E::MI_STEPS_PER_UNIT_E()
    : WiSpinInt(get_steps_per_unit_e_rounded(), SpinCnf::steps_per_unit, _(label)) {}

/*****************************************************************************/
//MI_RESET_STEPS_PER_UNIT
MI_RESET_STEPS_PER_UNIT::MI_RESET_STEPS_PER_UNIT()
    : WI_LABEL_t(_(label)) {}

void MI_RESET_STEPS_PER_UNIT::click(IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Reset_steps);
}

/*****************************************************************************/
//MI_MICROSTEPS_X
MI_MICROSTEPS_X::MI_MICROSTEPS_X()
    : WiSpinExp(get_microsteps_x(), SpinCnf::microstep_exponential, _(label)) {}

/*****************************************************************************/
//MI_MICROSTEPS_Y
MI_MICROSTEPS_Y::MI_MICROSTEPS_Y()
    : WiSpinExp(get_microsteps_y(), SpinCnf::microstep_exponential, _(label)) {}

/*****************************************************************************/
//MI_MICROSTEPS_Z
MI_MICROSTEPS_Z::MI_MICROSTEPS_Z()
    : WiSpinExp(get_microsteps_z(), SpinCnf::microstep_exponential, _(label)) {}

/*****************************************************************************/
//MI_MICROSTEPS_E
MI_MICROSTEPS_E::MI_MICROSTEPS_E()
    : WiSpinExp(get_microsteps_e(), SpinCnf::microstep_exponential, _(label)) {}

/*****************************************************************************/
//MI_RESET_MICROSTEPS
MI_RESET_MICROSTEPS::MI_RESET_MICROSTEPS()
    : WI_LABEL_t(_(label)) {}

void MI_RESET_MICROSTEPS::click(IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Reset_microsteps);
}

/*****************************************************************************/
//MI_CURRENT_X
MI_CURRENT_X::MI_CURRENT_X()
    : WiSpinInt(get_rms_current_ma_x(), SpinCnf::rms_current, _(label)) {}

/*****************************************************************************/
//MI_CURRENT_Y
MI_CURRENT_Y::MI_CURRENT_Y()
    : WiSpinInt(get_rms_current_ma_y(), SpinCnf::rms_current, _(label)) {}

/*****************************************************************************/
//MI_CURRENT_Z
MI_CURRENT_Z::MI_CURRENT_Z()
    : WiSpinInt(get_rms_current_ma_z(), SpinCnf::rms_current, _(label)) {}

/*****************************************************************************/
//MI_CURRENT_E
MI_CURRENT_E::MI_CURRENT_E()
    : WiSpinInt(get_rms_current_ma_e(), SpinCnf::rms_current, _(label)) {}

/*****************************************************************************/
//MI_RESET_CURRENTS
MI_RESET_CURRENTS::MI_RESET_CURRENTS()
    : WI_LABEL_t(_(label)) {}

void MI_RESET_CURRENTS::click(IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Reset_currents);
}

/*****************************************************************************/
//MI_SAVE_AND_RETURN
MI_SAVE_AND_RETURN::MI_SAVE_AND_RETURN()
    : WI_LABEL_t(_(label), IDR_PNG_folder_up_16px, is_enabled_t::yes, is_hidden_t::no) {}

void MI_SAVE_AND_RETURN::click(IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Return);
}

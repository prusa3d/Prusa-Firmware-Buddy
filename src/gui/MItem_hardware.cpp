#include "MItem_hardware.hpp"
#include "ScreenHandler.hpp"
#include "menu_spin_config.hpp"
#include <option/has_toolchanger.h>
#include <option/has_side_fsensor.h>
#include "DialogHandler.hpp"

#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
    #if HAS_SIDE_FSENSOR()
        #include <filament_sensors_handler_XL_remap.hpp>
        #include <gui_fsensor_api.hpp>
    #endif /*HAS_SIDE_FSENSOR()*/
#endif /*HAS_TOOLCHANGER()*/

#if ENABLED(PRUSA_TOOLCHANGER)
MI_NOZZLE_DIAMETER::MI_NOZZLE_DIAMETER(int tool_idx, is_hidden_t with_toolchanger)
    : WiSpinFlt(get_eeprom(tool_idx), SpinCnf::nozzle_diameter, _(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_toolchanger_enabled() ? with_toolchanger : is_hidden_t::no) //< Hide if toolchanger is enabled
    , tool_idx(tool_idx) {
}
#else /*ENABLED(PRUSA_TOOLCHANGER)*/
MI_NOZZLE_DIAMETER::MI_NOZZLE_DIAMETER(int tool_idx, [[maybe_unused]] is_hidden_t with_toolchanger)
    : WiSpinFlt(get_eeprom(tool_idx), SpinCnf::nozzle_diameter, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no)
    , tool_idx(tool_idx) {
}
#endif /*ENABLED(PRUSA_TOOLCHANGER)*/

float MI_NOZZLE_DIAMETER::get_eeprom(int tool_idx) const {
    return config_store().get_nozzle_diameter(tool_idx);
}

void MI_NOZZLE_DIAMETER::OnClick() {
    config_store().set_nozzle_diameter(tool_idx, GetVal());
}

MI_HARDWARE_G_CODE_CHECKS::MI_HARDWARE_G_CODE_CHECKS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_HARDWARE_G_CODE_CHECKS::click(IWindowMenu &) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuHardwareChecks>);
}

// MI_NOZZLE_TYPE
MI_NOZZLE_TYPE::MI_NOZZLE_TYPE()
    : WI_SWITCH_t<2>(config_store().nozzle_type.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev, _(str_normal), _(str_high_flow)) {};

void MI_NOZZLE_TYPE::OnChange([[maybe_unused]] size_t old_index) {
    config_store().nozzle_type.set(index);
}

// MI_NOZZLE_SOCK
MI_NOZZLE_SOCK::MI_NOZZLE_SOCK()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().nozzle_sock.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {};

void MI_NOZZLE_SOCK::OnChange([[maybe_unused]] size_t old_index) {
    config_store().nozzle_sock.set(!old_index);
}

#if HAS_TOOLCHANGER() && HAS_SIDE_FSENSOR()
// MI_SIDE_FSENSOR_REMAP
MI_SIDE_FSENSOR_REMAP::MI_SIDE_FSENSOR_REMAP()
    : WI_ICON_SWITCH_OFF_ON_t(side_fsensor_remap::is_remapped(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {};

void MI_SIDE_FSENSOR_REMAP::OnChange([[maybe_unused]] size_t old_index) {
    if (uint8_t mask = side_fsensor_remap::ask_to_remap(); mask != 0) { // Ask user to remap
        Screens::Access()->Get()->Validate(); // Do not redraw this menu yet
        index = side_fsensor_remap::is_remapped(); // Change index by what user selected
        Validate(); // Do not redraw this switch yet
        marlin_client::test_start_for_tools(stmFSensor, mask); // Start filament sensor calibration for moved tools
    } else {
        index = side_fsensor_remap::is_remapped(); // Change index by what user selected
    }
}
#endif /*HAS_TOOLCHANGER() && HAS_SIDE_FSENSOR()*/

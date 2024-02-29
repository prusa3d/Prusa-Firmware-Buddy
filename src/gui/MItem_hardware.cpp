#include "MItem_hardware.hpp"
#include "ScreenHandler.hpp"
#include "menu_spin_config.hpp"
#include <option/has_toolchanger.h>
#include <option/has_side_fsensor.h>

#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
    #if HAS_SIDE_FSENSOR()
        #include <filament_sensors_handler_XL_remap.hpp>
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
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_HARDWARE_G_CODE_CHECKS::click(IWindowMenu &) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuHardwareChecks>);
}

// MI_NOZZLE_TYPE
MI_NOZZLE_TYPE::MI_NOZZLE_TYPE()
    : IWiSwitch(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {

    SetIndex(ftrstd::to_underlying(config_store().nozzle_type.get()));

    // Items are initialized now, update extension width
    changeExtentionWidth();
};

void MI_NOZZLE_TYPE::OnChange([[maybe_unused]] size_t old_index) {
    config_store().nozzle_type.set(static_cast<NozzleType>(index));
}

// MI_HOTEND_TYPE
MI_HOTEND_TYPE::MI_HOTEND_TYPE()
    : IWiSwitch(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) //
{
    // Determine current index
    if (auto he = std::find(supported_hotend_types.begin(), supported_hotend_types.end(), config_store().hotend_type.get()); he != supported_hotend_types.end()) {
        SetIndex(he - supported_hotend_types.begin());
    }

    // Items are initialized now, update extension width
    changeExtentionWidth();
}

void MI_HOTEND_TYPE::OnChange([[maybe_unused]] size_t old_index) {
    config_store().hotend_type.set(supported_hotend_types[index]);
}

// MI_NOZZLE_SOCK
MI_NOZZLE_SOCK::MI_NOZZLE_SOCK()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().hotend_type.get() == HotendType::stock_with_sock, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {};

void MI_NOZZLE_SOCK::OnChange([[maybe_unused]] size_t old_index) {
    config_store().hotend_type.set(index ? HotendType::stock_with_sock : HotendType::stock);
}

#if HAS_TOOLCHANGER() && HAS_SIDE_FSENSOR()
// MI_SIDE_FSENSOR_REMAP
MI_SIDE_FSENSOR_REMAP::MI_SIDE_FSENSOR_REMAP()
    : WI_ICON_SWITCH_OFF_ON_t(side_fsensor_remap::is_remapped(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {};

void MI_SIDE_FSENSOR_REMAP::OnChange([[maybe_unused]] size_t old_index) {
    if (uint8_t mask = side_fsensor_remap::ask_to_remap(); mask != 0) { // Ask user to remap
        Screens::Access()->Get()->Validate(); // Do not redraw this menu yet

        // Change index by what user selected)
        set_value(side_fsensor_remap::is_remapped(), false);

        Validate(); // Do not redraw this switch yet
        marlin_client::test_start_with_data(stmFSensor, static_cast<ToolMask>(mask)); // Start filament sensor calibration for moved tools

    } else {
        // Change index by what user selected)
        set_value(side_fsensor_remap::is_remapped(), false);
    }
}
#endif /*HAS_TOOLCHANGER() && HAS_SIDE_FSENSOR()*/

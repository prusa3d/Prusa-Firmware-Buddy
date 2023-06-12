#include "MItem_hardware.hpp"
#include "ScreenHandler.hpp"
#include <option/has_toolchanger.h>

#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif /*HAS_TOOLCHANGER()*/

#if ENABLED(PRUSA_TOOLCHANGER)
MI_NOZZLE_DIAMETER::MI_NOZZLE_DIAMETER(int tool_idx, is_hidden_t with_toolchanger)
    : MI_SWITCH_NOZZLE_DIAMETER_t(get_eeprom(tool_idx), _(label), nullptr, is_enabled_t::yes,
        prusa_toolchanger.is_toolchanger_enabled() ? with_toolchanger : is_hidden_t::no, diameters) //< Hide if toolchanger is enabled
    , tool_idx(tool_idx) {
}
#else  /*ENABLED(PRUSA_TOOLCHANGER)*/
MI_NOZZLE_DIAMETER::MI_NOZZLE_DIAMETER(int tool_idx, [[maybe_unused]] is_hidden_t with_toolchanger)
    : MI_SWITCH_NOZZLE_DIAMETER_t(get_eeprom(tool_idx), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, diameters)
    , tool_idx(tool_idx) {
}
#endif /*ENABLED(PRUSA_TOOLCHANGER)*/

size_t MI_NOZZLE_DIAMETER::get_eeprom(int tool_idx) const {
    auto value = eeprom_get_nozzle_dia(tool_idx);
    auto it = std::find(begin(diameters), end(diameters), value);
    return it == end(diameters) ? DEFAULT_DIAMETER_INDEX : it - begin(diameters);
}

void MI_NOZZLE_DIAMETER::OnChange([[maybe_unused]] size_t old_index) {
    eeprom_set_nozzle_dia(tool_idx, diameters[index]);
}

MI_HARDWARE_G_CODE_CHECKS::MI_HARDWARE_G_CODE_CHECKS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_HARDWARE_G_CODE_CHECKS::click(IWindowMenu &) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuHardwareChecks>);
}

// MI_NOZZLE_TYPE
MI_NOZZLE_TYPE::MI_NOZZLE_TYPE()
    : WI_SWITCH_t<2>(eeprom_get_ui8(EEVAR_NOZZLE_TYPE), _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev, _(str_normal), _(str_high_flow)) {};

void MI_NOZZLE_TYPE::OnChange([[maybe_unused]] size_t old_index) {
    eeprom_set_ui8(EEVAR_NOZZLE_TYPE, index);
}

// MI_NOZZLE_SOCK
MI_NOZZLE_SOCK::MI_NOZZLE_SOCK()
    : WI_ICON_SWITCH_OFF_ON_t(eeprom_get_bool(EEVAR_NOZZLE_SOCK), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {};

void MI_NOZZLE_SOCK::OnChange([[maybe_unused]] size_t old_index) {
    eeprom_set_bool(EEVAR_NOZZLE_SOCK, !old_index);
}

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
MI_NOZZLE_DIAMETER::MI_NOZZLE_DIAMETER(int tool_idx, is_hidden_t with_toolchanger)
    : MI_SWITCH_NOZZLE_DIAMETER_t(get_eeprom(tool_idx), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, diameters)
    , tool_idx(tool_idx) {
}
#endif /*ENABLED(PRUSA_TOOLCHANGER)*/

size_t MI_NOZZLE_DIAMETER::get_eeprom(int tool_idx) const {
    auto value = eeprom_get_nozzle_dia(tool_idx);
    auto it = std::find(begin(diameters), end(diameters), value);
    return it == end(diameters) ? DEFAULT_DIAMETER_INDEX : it - begin(diameters);
}

void MI_NOZZLE_DIAMETER::OnChange(size_t old_index) {
    eeprom_set_nozzle_dia(tool_idx, diameters[index]);
}

MI_HARDWARE_CHECKS::MI_HARDWARE_CHECKS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_HARDWARE_CHECKS::click(IWindowMenu &) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuHardwareChecks>);
}

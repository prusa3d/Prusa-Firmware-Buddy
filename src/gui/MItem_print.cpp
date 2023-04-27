#include "MItem_print.hpp"
#include "marlin_client.hpp"
#include "menu_vars.h"
#include "eeprom.h"
#include "menu_spin_config.hpp"
#include "png_resources.hpp"
#if ENABLED(PRUSA_TOOLCHANGER)
    #include "module/prusa/toolchanger.h"
#endif

/*****************************************************************************/
//MI_NOZZLE_ABSTRACT
is_hidden_t MI_NOZZLE_ABSTRACT::is_hidden([[maybe_unused]] uint8_t tool_nr) {
#if ENABLED(PRUSA_TOOLCHANGER)
    return prusa_toolchanger.is_tool_enabled(tool_nr) ? is_hidden_t::no : is_hidden_t::yes;
#else
    return is_hidden_t::no;
#endif
}

MI_NOZZLE_ABSTRACT::MI_NOZZLE_ABSTRACT(uint8_t tool_nr, [[maybe_unused]] const char *label)
    : WiSpinInt(uint16_t(marlin_vars()->hotend(tool_nr).target_nozzle), SpinCnf::nozzle,
#if ENABLED(PRUSA_TOOLCHANGER)
        prusa_toolchanger.is_toolchanger_enabled() ? _(label) : _(generic_label),
#else
        _(generic_label),
#endif
        &png::nozzle_16x16, is_enabled_t::yes, is_hidden(tool_nr))
    , tool_nr(tool_nr) {
}

void MI_NOZZLE_ABSTRACT::OnClick() {
    marlin_set_target_nozzle(GetVal(), tool_nr);
    marlin_set_display_nozzle(GetVal(), tool_nr);
}

/*****************************************************************************/
//MI_HEATBED
MI_HEATBED::MI_HEATBED()
    : WiSpinInt(uint8_t(marlin_vars()->target_bed),
        SpinCnf::bed, _(label), &png::heatbed_16x16, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_HEATBED::OnClick() {
    marlin_set_target_bed(GetVal());
}

/*****************************************************************************/
//MI_PRINTFAN
MI_PRINTFAN::MI_PRINTFAN()
    : WiSpinInt(val_mapping(false, marlin_vars()->print_fan_speed, 255, 100),
        SpinCnf::printfan, _(label), &png::fan_16x16, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_PRINTFAN::OnClick() {
    marlin_set_fan_speed(val_mapping(true, GetVal(), 100, 255));
}

uint8_t MI_PRINTFAN::val_mapping(const bool rounding_floor, const uint8_t val, const uint8_t max_val, const uint8_t new_max_val) {
    if (rounding_floor) {
        return uint8_t(static_cast<uint32_t>(val) * new_max_val / max_val);
    } else {
        return uint8_t((static_cast<uint32_t>(val) * new_max_val + max_val - 1) / max_val);
    }
}

/*****************************************************************************/
//MI_SPEED
MI_SPEED::MI_SPEED()
    : WiSpinInt(uint16_t(marlin_vars()->print_speed),
        SpinCnf::feedrate, _(label), &png::speed_16x16, is_enabled_t::yes, is_hidden_t::no) {}
void MI_SPEED::OnClick() {
    marlin_set_print_speed(GetVal());
}

/*****************************************************************************/
//MI_FLOWFACT
MI_FLOWFACT::MI_FLOWFACT()
    : WiSpinInt(uint16_t(marlin_vars()->active_hotend().flow_factor),
        SpinCnf::flowfact, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
void MI_FLOWFACT::OnClick() {
    marlin_set_flow_factor(GetVal());
}

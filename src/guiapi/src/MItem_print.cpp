#include "MItem_print.hpp"
#include "marlin_client.h"
#include "menu_vars.h"
#include "eeprom.h"
#include "menu_spin_config.hpp"
/*****************************************************************************/
//MI_NOZZLE
MI_NOZZLE::MI_NOZZLE()
    : WiSpinInt(uint16_t(marlin_vars()->target_nozzle),
        SpinCnf::nozzle, _(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
void MI_NOZZLE::OnClick() {
    marlin_set_target_nozzle(GetVal());
    marlin_set_display_nozzle(GetVal());
}

/*****************************************************************************/
//MI_HEATBED
MI_HEATBED::MI_HEATBED()
    : WiSpinInt(uint8_t(marlin_vars()->target_bed),
        SpinCnf::bed, _(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
void MI_HEATBED::OnClick() {
    marlin_set_target_bed(GetVal());
}

/*****************************************************************************/
//MI_PRINTFAN
MI_PRINTFAN::MI_PRINTFAN()
    : WiSpinInt(uint8_t(marlin_vars()->print_fan_speed),
        SpinCnf::printfan, _(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
void MI_PRINTFAN::OnClick() {
    marlin_set_fan_speed(GetVal());
}

/*****************************************************************************/
//MI_SPEED
MI_SPEED::MI_SPEED()
    : WiSpinInt(uint16_t(marlin_vars()->print_speed),
        SpinCnf::feedrate, _(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
void MI_SPEED::OnClick() {
    marlin_set_print_speed(GetVal());
}

/*****************************************************************************/
//MI_FLOWFACT
MI_FLOWFACT::MI_FLOWFACT()
    : WiSpinInt(uint16_t(marlin_vars()->flow_factor),
        SpinCnf::flowfact, _(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
void MI_FLOWFACT::OnClick() {
    marlin_set_flow_factor(GetVal());
}

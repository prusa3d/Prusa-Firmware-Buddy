#include "MItem_print.hpp"
#include "marlin_client.hpp"
#include "menu_vars.h"
#include "eeprom.h"
#include "menu_spin_config.hpp"
/*****************************************************************************/
//MI_NOZZLE
MI_NOZZLE::MI_NOZZLE()
    : WiSpinInt(uint16_t(print_client::vars()->target_nozzle),
        SpinCnf::nozzle, _(label), IDR_NULL, is_enabled_t::yes, is_hidden_t::no) {}
void MI_NOZZLE::OnClick() {
    print_client::set_target_nozzle(GetVal());
    print_client::set_display_nozzle(GetVal());
}

/*****************************************************************************/
//MI_HEATBED
MI_HEATBED::MI_HEATBED()
    : WiSpinInt(uint8_t(print_client::vars()->target_bed),
        SpinCnf::bed, _(label), IDR_NULL, is_enabled_t::yes, is_hidden_t::no) {}
void MI_HEATBED::OnClick() {
    print_client::set_target_bed(GetVal());
}

/*****************************************************************************/
//MI_PRINTFAN
MI_PRINTFAN::MI_PRINTFAN()
    : WiSpinInt(uint8_t(print_client::vars()->print_fan_speed),
        SpinCnf::printfan, _(label), IDR_NULL, is_enabled_t::yes, is_hidden_t::no) {}
void MI_PRINTFAN::OnClick() {
    print_client::set_fan_speed(GetVal());
}

/*****************************************************************************/
//MI_SPEED
MI_SPEED::MI_SPEED()
    : WiSpinInt(uint16_t(print_client::vars()->print_speed),
        SpinCnf::feedrate, _(label), IDR_NULL, is_enabled_t::yes, is_hidden_t::no) {}
void MI_SPEED::OnClick() {
    print_client::set_print_speed(GetVal());
}

/*****************************************************************************/
//MI_FLOWFACT
MI_FLOWFACT::MI_FLOWFACT()
    : WiSpinInt(uint16_t(print_client::vars()->flow_factor),
        SpinCnf::flowfact, _(label), IDR_NULL, is_enabled_t::yes, is_hidden_t::no) {}
void MI_FLOWFACT::OnClick() {
    print_client::set_flow_factor(GetVal());
}

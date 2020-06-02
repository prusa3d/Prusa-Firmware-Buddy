#include "MItem_print.hpp"
#include "marlin_client.h"
#include "menu_vars.h"
/*****************************************************************************/
//MI_NOZZLE
MI_NOZZLE::MI_NOZZLE()
    : WI_SPIN_U16_t(uint16_t(marlin_vars()->target_nozzle),
        MenuVars::nozzle_range.data(), label, 0, true, false) {}
void MI_NOZZLE::OnClick() {
    marlin_set_target_nozzle(value);
}

/*****************************************************************************/
//MI_HEATBED
MI_HEATBED::MI_HEATBED()
    : WI_SPIN_U08_t(uint8_t(marlin_vars()->target_bed),
        MenuVars::bed_range.data(), label, 0, true, false) {}
void MI_HEATBED::OnClick() {
    marlin_set_target_bed(value);
}

/*****************************************************************************/
//MI_PRINTFAN
MI_PRINTFAN::MI_PRINTFAN()
    : WI_SPIN_U08_t(uint8_t(marlin_vars()->fan_speed),
        MenuVars::printfan_range.data(), label, 0, true, false) {}
void MI_PRINTFAN::OnClick() {
    marlin_set_fan_speed(value);
}

/**
 * @file footer_items_nozzle_bed.cpp
 * @author Radek Vana
 * @date 2021-04-14
 */

#include "footer_items_nozzle_bed.hpp"
#include "marlin_client.h"
#include "resource.h"
#include "filament.hpp"

FooterItemNozzle::FooterItemNozzle(window_t *parent)
    : AddSuperWindow<FooterItemHeater>(parent, IDR_PNG_nozzle_16px, static_makeView, static_readValue) {
}

FooterItemBed::FooterItemBed(window_t *parent)
    : AddSuperWindow<FooterItemHeater>(parent, IDR_PNG_heatbed_16px, static_makeView, static_readValue) {
}

int FooterItemNozzle::static_readValue() {
    static const uint cold = 50;

    uint current = marlin_vars()->temp_nozzle;
    uint target = marlin_vars()->target_nozzle;
    uint display = marlin_vars()->display_nozzle;

    HeatState state = getState(current, target, display, cold);
    StateAndTemps temps(state, current, display);
    return temps.ToInt();
}

int FooterItemBed::static_readValue() {
    static const uint cold = 40;

    uint current = marlin_vars()->temp_bed;
    uint target = marlin_vars()->target_bed;

    HeatState state = getState(current, target, target, cold); //display == target will disable green blinking preheat
    StateAndTemps temps(state, current, target);
    return temps.ToInt();
}

//This methods cannot be one - need separate buffers
string_view_utf8 FooterItemNozzle::static_makeView(int value) {
    static std::array<char, 10> buff;
    return static_makeViewIntoBuff(value, buff);
}

string_view_utf8 FooterItemBed::static_makeView(int value) {
    static std::array<char, 10> buff;
    return static_makeViewIntoBuff(value, buff);
}

/**
 * @file footer_item_fans.cpp
 * @author Radek Vana
 * @date 2021-12-02
 */
#include "footer_item_fans.hpp"
#include "marlin_client.h"
#include "resource.h"
#include <algorithm>

//static variables
IFooterItemFan::buffer_t FooterItemPrintFan::buffer;
IFooterItemFan::buffer_t FooterItemHeatBreakFan::buffer;

string_view_utf8 IFooterItemFan::static_makeViewIntoBuff(int value, buffer_t &buff) {
    buff.fill('\0');
    uint value_clamped = std::clamp(value, 0, max_rpm);
    snprintf(buff.data(), buff.size(), "%urpm", value_clamped);

    return string_view_utf8::MakeRAM((const uint8_t *)buff.data());
}

IFooterItemFan::IFooterItemFan(window_t *parent, uint16_t icon_id, view_maker_cb view_maker, reader_cb value_reader)
    : AddSuperWindow<FooterIconText_IntVal>(parent, icon_id, view_maker, value_reader) {
}

FooterItemPrintFan::FooterItemPrintFan(window_t *parent)
    : AddSuperWindow<IFooterItemFan>(parent, IDR_PNG_turbine_16x16, static_makeView, static_readValue) {
}
int FooterItemPrintFan::static_readValue() {
    return marlin_vars()->print_fan_rpm;
}

FooterItemHeatBreakFan::FooterItemHeatBreakFan(window_t *parent)
    : AddSuperWindow<IFooterItemFan>(parent, IDR_PNG_fan_16x16, static_makeView, static_readValue) {
}
int FooterItemHeatBreakFan::static_readValue() {
    return marlin_vars()->heatbreak_fan_rpm;
}

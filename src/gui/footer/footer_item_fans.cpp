/**
 * @file footer_item_fans.cpp
 * @author Radek Vana
 * @date 2021-12-02
 */
#include "footer_item_fans.hpp"
#include "marlin_client.h"
#include "resource.h"
#include <algorithm>

static constexpr int max_rpm = 99999;

string_view_utf8 IFooterItemFan::static_makeView(int value) {
    static std::array<char, sizeof("99999rpm")> buff; // the buffer must be static, because string_view does not copy data inside itself
    buff.fill('\0');
    uint value_clamped = std::clamp(value, 0, max_rpm);
    snprintf(buff.data(), buff.size(), "%urpm", value_clamped);

    return string_view_utf8::MakeRAM((const uint8_t *)buff.data());
}

IFooterItemFan::IFooterItemFan(window_t *parent, uint16_t icon_id, reader_cb value_reader)
    : AddSuperWindow<FooterIconText_IntVal>(parent, icon_id, static_makeView, value_reader) {
}

FooterItemPrintFan::FooterItemPrintFan(window_t *parent)
    : AddSuperWindow<IFooterItemFan>(parent, IDR_PNG_turbine_16x16, static_readValue) {
}
int FooterItemPrintFan::static_readValue() {
    return marlin_vars()->print_fan_rpm;
}

FooterItemHeatBreakFan::FooterItemHeatBreakFan(window_t *parent)
    : AddSuperWindow<IFooterItemFan>(parent, IDR_PNG_fan_16x16, static_readValue) {
}
int FooterItemHeatBreakFan::static_readValue() {
    return marlin_vars()->heatbreak_fan_rpm;
}

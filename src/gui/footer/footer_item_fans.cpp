/**
 * @file footer_item_fans.cpp
 */
#include "footer_item_fans.hpp"
#include "marlin_client.hpp"
#include "img_resources.hpp"
#include "i18n.h"
#include <algorithm>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif /*HAS_TOOLCHANGER()*/

// static variables
IFooterItemFan::buffer_t FooterItemPrintFan::buffer;
IFooterItemFan::buffer_t FooterItemHeatBreakFan::buffer;

string_view_utf8 IFooterItemFan::static_makeViewIntoBuff(int value, buffer_t &buff) {
    // Show --- if no tool is picked
    if (value == no_tool_value) {
        return string_view_utf8::MakeCPUFLASH(reinterpret_cast<const uint8_t *>(no_tool_str));
    }

    buff.fill('\0');
    uint value_clamped = std::clamp(value, 0, max_rpm);
    snprintf(buff.data(), buff.size(), "%urpm", value_clamped);

    return string_view_utf8::MakeRAM((const uint8_t *)buff.data());
}

IFooterItemFan::IFooterItemFan(window_t *parent, const img::Resource *icon, view_maker_cb view_maker, reader_cb value_reader)
    : FooterIconText_IntVal(parent, icon, view_maker, value_reader) {
}

FooterItemPrintFan::FooterItemPrintFan(window_t *parent)
    : IFooterItemFan(parent, &img::turbine_16x16, static_makeView, static_readValue) {
}

int FooterItemPrintFan::static_readValue() {
#if HAS_TOOLCHANGER()
    if (marlin_vars().active_extruder == PrusaToolChanger::MARLIN_NO_TOOL_PICKED) {
        return no_tool_value;
    }
#endif /*HAS_TOOLCHANGER()*/

    return marlin_vars().active_hotend().print_fan_rpm;
}

FooterItemHeatBreakFan::FooterItemHeatBreakFan(window_t *parent)
    : IFooterItemFan(parent, &img::fan_16x16, static_makeView, static_readValue) {
}

int FooterItemHeatBreakFan::static_readValue() {
#if HAS_TOOLCHANGER()
    if (marlin_vars().active_extruder == PrusaToolChanger::MARLIN_NO_TOOL_PICKED) {
        return no_tool_value;
    }
#endif /*HAS_TOOLCHANGER()*/

    return marlin_vars().active_hotend().heatbreak_fan_rpm;
}

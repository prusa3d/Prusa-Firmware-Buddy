/**
 * @file footer_item_nozzle_diameter.cpp
 */

#include "footer_item_nozzle_diameter.hpp"
#include "marlin_client.hpp"
#include "display_helper.h" // font_meas_text
#include "img_resources.hpp"
#include <config_store/store_instance.hpp>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif /*HAS_TOOLCHANGER()*/

FooterItemNozzleDiameter::FooterItemNozzleDiameter(window_t *parent)
    : AddSuperWindow<FooterIconText_FloatVal>(parent, &img::nozzle_16x16, static_makeView, static_readValue) {
}

float FooterItemNozzleDiameter::static_readValue() {
#if HAS_TOOLCHANGER()
    if (marlin_vars()->active_extruder == PrusaToolChanger::MARLIN_NO_TOOL_PICKED) {
        return no_tool_value;
    }
#endif /*HAS_TOOLCHANGER()*/

    auto current_nozzle_diameter = config_store().get_nozzle_diameter(marlin_vars()->active_extruder);
    return static_cast<float>(current_nozzle_diameter);
}

string_view_utf8 FooterItemNozzleDiameter::static_makeView(float value) {
    static std::array<char, 5> buff; //"0" - "1", longest "0.99"
    snprintf(buff.data(), buff.size(), "%.2f", (double)value);
    return string_view_utf8::MakeRAM((const uint8_t *)buff.data());
}

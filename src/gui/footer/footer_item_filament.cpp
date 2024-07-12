/**
 * @file footer_item_filament.cpp
 */

#include "footer_item_filament.hpp"
#include "marlin_client.hpp"
#include "img_resources.hpp"
#include <filament.hpp>
#include <encoded_filament.hpp>
#include <config_store/store_instance.hpp>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif /*HAS_TOOLCHANGER()*/

FooterItemFilament::FooterItemFilament(window_t *parent)
    : FooterIconText_IntVal(parent, &img::spool_16x16, static_makeView, static_readValue) {
}

int FooterItemFilament::static_readValue() {
#if HAS_TOOLCHANGER()
    if (marlin_vars().active_extruder == PrusaToolChanger::MARLIN_NO_TOOL_PICKED) {
        return no_tool_value;
    }
#endif /*HAS_TOOLCHANGER()*/

    auto current_filament = config_store().get_filament_type(marlin_vars().active_extruder);
    return EncodedFilamentType(current_filament).data;
}

string_view_utf8 FooterItemFilament::static_makeView(int value) {
    // Show --- if no tool is picked
    if (value == no_tool_value) {
        return string_view_utf8::MakeCPUFLASH(reinterpret_cast<const uint8_t *>(no_tool_str));
    }

    auto filament = EncodedFilamentType::from_data(value).decode();
    return string_view_utf8::MakeCPUFLASH((const uint8_t *)filament::get_name(filament));
}

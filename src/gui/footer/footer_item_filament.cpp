/**
 * @file footer_item_filament.cpp
 */

#include "footer_item_filament.hpp"
#include "marlin_client.hpp"
#include "display_helper.h" // font_meas_text
#include "png_resources.hpp"
#include "filament.hpp"

FooterItemFilament::FooterItemFilament(window_t *parent)
    : AddSuperWindow<FooterIconText_IntVal>(parent, &png::spool_16x16, static_makeView, static_readValue) {
}

int FooterItemFilament::static_readValue() {
    auto current_filament = filament::get_type_in_extruder(marlin_vars()->active_extruder);
    return static_cast<int>(current_filament);
}

string_view_utf8 FooterItemFilament::static_makeView(int value) {
    auto filament = static_cast<filament::Type>(value);
    return string_view_utf8::MakeCPUFLASH((const uint8_t *)filament::get_description(filament).name);
}

string_view_utf8 FooterItemFilament::GetName() { return _("Filament"); }

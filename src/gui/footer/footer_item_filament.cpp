/**
 * @file footer_item_filament.cpp
 */

#include "footer_item_filament.hpp"
#include "display_helper.h" // font_meas_text
#include "png_resources.hpp"
#include "filament.hpp"

FooterItemFilament::FooterItemFilament(window_t *parent)
    : AddSuperWindow<FooterIconText_IntVal>(parent, &png::spool_16x16, static_makeView, static_readValue) {
}

int FooterItemFilament::static_readValue() {
    return int(Filaments::CurrentIndex());
}

string_view_utf8 FooterItemFilament::static_makeView(int value) {
    return string_view_utf8::MakeCPUFLASH((const uint8_t *)Filaments::Current().name);
}

string_view_utf8 FooterItemFilament::GetName() { return _("Filament"); }

/**
 * @file footer_item_filament.cpp
 * @author Radek Vana
 * @date 2021-04-16
 */

#include "footer_item_filament.hpp"
#include "png_resources.hpp"
#include "filament.hpp"

FooterItemFilament::FooterItemFilament(window_t *parent)
    : AddSuperWindow<FooterIconText_IntVal>(parent, png::Get<png::Id::spool_16x16>(), static_makeView, static_readValue) {
}

int FooterItemFilament::static_readValue() {
    return int(Filaments::CurrentIndex());
}

string_view_utf8 FooterItemFilament::static_makeView(int value) {
    return string_view_utf8::MakeCPUFLASH((const uint8_t *)Filaments::Current().name);
}

/**
 * @file footer_item_sheet_profile.cpp
 */

#include "footer_item_sheet_profile.hpp"
#include "png_resources.hpp"
#include "SteelSheets.hpp"
#include "i18n.h"
#include <configuration_store.hpp>

FooterItemSheets::FooterItemSheets(window_t *parent)
    : AddSuperWindow<FooterIconText_IntVal>(parent, &png::sheets_profile_16x14, static_makeView, static_readValue) {
}

int FooterItemSheets::static_readValue() {
    return config_store().active_sheet.get();
}

string_view_utf8 FooterItemSheets::static_makeView([[maybe_unused]] int value) {
    static char buff[8];
    SteelSheets::ActiveSheetName(buff, sizeof(buff));
    return string_view_utf8::MakeRAM((const uint8_t *)buff);
}

/**
 * @file footer_item_sheet_profile.cpp
 */

#include "footer_item_sheet_profile.hpp"
#include "png_resources.hpp"
#include "i18n.h"
#include "display_helper.h" // font_meas_text
#include "resource.h"       // IDR_PNG_sheets_profile_16px
#include "configuration_store.hpp"

FooterItemSheets::FooterItemSheets(window_t *parent)
    : AddSuperWindow<FooterIconText_IntVal>(parent, &png::sheets_profile_16x14, static_makeView, static_readValue) {
}

int FooterItemSheets::static_readValue() {
    return config_store().steel_sheets.get().get_active_sheet_index();
}

string_view_utf8 FooterItemSheets::static_makeView(int value) {
    static decltype(config_store().steel_sheets.get().get_active_sheet_name()) buff;
    buff = config_store().steel_sheets.get().get_active_sheet_name();
    return string_view_utf8::MakeRAM((const uint8_t *)buff.data());
}

string_view_utf8 FooterItemSheets::GetName() { return _("Sheets"); }

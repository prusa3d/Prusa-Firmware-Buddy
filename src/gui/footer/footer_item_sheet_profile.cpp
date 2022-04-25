/**
 * @file footer_item_filament.cpp
 * @author Radek Vana
 * @date 2021-04-17
 */

#include "footer_item_sheet_profile.hpp"
#include "display_helper.h" // font_meas_text
#include "resource.h"       // IDR_PNG_sheets_profile_16px
#include "SteelSheets.hpp"

FooterItemSheets::FooterItemSheets(window_t *parent)
    : AddSuperWindow<FooterIconText_IntVal>(parent, IDR_PNG_sheets_profile_16px, static_makeView, static_readValue) {
}

int FooterItemSheets::static_readValue() {
    return eeprom_get_ui8(EEVAR_ACTIVE_SHEET);
}

string_view_utf8 FooterItemSheets::static_makeView(int value) {
    static char buff[8];
    SteelSheets::ActiveSheetName(buff, sizeof(buff));
    return string_view_utf8::MakeRAM((const uint8_t *)buff);
}

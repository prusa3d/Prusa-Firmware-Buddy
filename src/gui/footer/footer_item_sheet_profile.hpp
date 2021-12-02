/**
 * @file footer_item_sheet_profile.hpp
 * @author Radek Vana
 * @brief footer item displaying selected sheet profile
 * @date 2021-04-17
 */

#pragma once
#include "ifooter_item.hpp"

class FooterItemSheets : public AddSuperWindow<FooterIconText_IntVal> {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    static string_view_utf8 GetName() { return _("Sheets"); }
    FooterItemSheets(window_t *parent);
};

/**
 * @file footer_item_sheet_profile.hpp
 * @brief footer item displaying selected sheet profile
 */

#pragma once
#include "ifooter_item.hpp"

class FooterItemSheets : public AddSuperWindow<FooterIconText_IntVal> {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    FooterItemSheets(window_t *parent);
};

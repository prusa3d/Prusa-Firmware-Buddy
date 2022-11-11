/**
 * @file footer_item_printspeed.hpp
 * @brief footer item displaying filament type
 */

#pragma once
#include "ifooter_item.hpp"
#include "filament.hpp"

class FooterItemSpeed : public AddSuperWindow<FooterIconText_IntVal> {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    static string_view_utf8 GetName();
    FooterItemSpeed(window_t *parent);
};

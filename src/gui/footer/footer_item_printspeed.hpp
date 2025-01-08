/**
 * @file footer_item_printspeed.hpp
 * @brief footer item displaying filament type
 */

#pragma once
#include "ifooter_item.hpp"
#include "filament.hpp"

class FooterItemSpeed final : public FooterIconText_IntVal {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    FooterItemSpeed(window_t *parent);
};

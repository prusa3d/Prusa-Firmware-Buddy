/**
 * @file footer_item_heatbreak.hpp
 * @brief footer item displaying heatbrak temperature
 */

#pragma once
#include "ifooter_item.hpp"
#include "filament.hpp"

class FooterItemHeatBreak final : public FooterIconText_IntVal {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    FooterItemHeatBreak(window_t *parent);
};

/**
 * @file footer_item_live_z.hpp
 * @brief footer item displaying filament type
 */

#pragma once
#include "ifooter_item.hpp"
#include "filament.hpp"

class FooterItemLiveZ : public AddSuperWindow<FooterIconText_IntVal> {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    FooterItemLiveZ(window_t *parent);
};

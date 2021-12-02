/**
 * @file footer_item_live_z.hpp
 * @author Radek Vana
 * @brief footer item displaying filament type
 * @date 2021-04-17
 */

#pragma once
#include "ifooter_item.hpp"
#include "filament.hpp"

class FooterItemLiveZ : public AddSuperWindow<FooterIconText_IntVal> {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    static string_view_utf8 GetName() { return _("Live Z"); }
    FooterItemLiveZ(window_t *parent);
};

/**
 * @file footer_item_fsensor.hpp
 * @brief footer item displaying filament sensor state
 */

#pragma once
#include "ifooter_item.hpp"
#include "filament.hpp"

class FooterItemFSensor : public AddSuperWindow<FooterIconText_IntVal> {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    static string_view_utf8 GetName();
    FooterItemFSensor(window_t *parent);
};

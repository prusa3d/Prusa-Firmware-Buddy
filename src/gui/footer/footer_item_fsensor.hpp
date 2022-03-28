/**
 * @file footer_item_fsensor.hpp
 * @author Radek Vana
 * @brief footer item displaying filament sensor state
 * @date 2021-12-12
 */

#pragma once
#include "ifooter_item.hpp"
#include "filament.hpp"

class FooterItemFSensor : public AddSuperWindow<FooterIconText_IntVal> {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    static string_view_utf8 GetName() { return _("F. Sensor"); }
    FooterItemFSensor(window_t *parent);
};

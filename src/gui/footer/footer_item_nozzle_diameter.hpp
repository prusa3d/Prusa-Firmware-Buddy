/**
 * @file footer_item_nozzle_diameter.hpp
 * @brief footer item displaying current nozzle diameter
 */

#pragma once
#include "ifooter_item.hpp"
#include "i18n.h"

class FooterItemNozzleDiameter : public AddSuperWindow<FooterIconText_FloatVal> {
    static string_view_utf8 static_makeView(float value);
    static float static_readValue();

public:
    FooterItemNozzleDiameter(window_t *parent);
};

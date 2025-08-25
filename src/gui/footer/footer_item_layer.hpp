/**
 * @file footer_item_layer.hpp
 * @brief footer item displaying current layer
 */

#pragma once
#include "ifooter_item.hpp"

class FooterItemCurrentLayer : public FooterIconText_IntVal {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    FooterItemCurrentLayer(window_t *parent);
};

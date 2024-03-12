/**
 * @file footer_item_enclosure.hpp
 * @brief footer item displaying enclosure temperature
 */

#pragma once
#include "ifooter_item.hpp"

class FooterItemEnclosure : public AddSuperWindow<FooterIconText_IntVal> {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    FooterItemEnclosure(window_t *parent);
};

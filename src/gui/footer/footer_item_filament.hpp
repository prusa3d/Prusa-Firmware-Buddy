/**
 * @file footer_item_filament.hpp
 * @brief footer item displaying filament type
 */

#pragma once
#include "ifooter_item.hpp"
#include "i18n.h"

class FooterItemFilament final : public FooterIconText_IntVal {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    FooterItemFilament(window_t *parent);
};

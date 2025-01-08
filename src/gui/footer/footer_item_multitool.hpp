/**
 * @file footer_item_multitool.hpp
 * @brief footer items for tools
 */

#pragma once
#include "ifooter_item.hpp"

class FooterItemFinda final : public FooterIconText_IntVal {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    FooterItemFinda(window_t *parent);
};

class FooterItemCurrentTool final : public FooterIconText_IntVal {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    FooterItemCurrentTool(window_t *parent);
};

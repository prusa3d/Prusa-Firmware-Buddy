#pragma once

#include "ifooter_item.hpp"

class FooterItemSheets : public FooterIconText_IntVal {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    FooterItemSheets(window_t *parent);
};

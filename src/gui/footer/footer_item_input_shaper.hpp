/**
 * @file footer_item_input_shaper.hpp
 * @brief footer item displaying input shaper values of X or Y axis
 */

#pragma once
#include "ifooter_item.hpp"

class FooterItemInputShaperX : public FooterIconText_IntVal {
    using buffer_t = std::array<char, 11>;
    static buffer_t buff;
    static int static_readValue();
    static string_view_utf8 static_makeViewIntoBuff(int value);

public:
    FooterItemInputShaperX(window_t *parent);
};

class FooterItemInputShaperY : public FooterIconText_IntVal {
    using buffer_t = std::array<char, 11>;
    static buffer_t buff;
    static int static_readValue();
    static string_view_utf8 static_makeViewIntoBuff(int value);

public:
    FooterItemInputShaperY(window_t *parent);
};

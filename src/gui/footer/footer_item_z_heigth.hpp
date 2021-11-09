#pragma once
#include "ifooter_item.hpp"

class FooterItemZHeigth : public AddSuperWindow<FooterIconText_FloatVal> {
    static string_view_utf8 static_makeView(float value);
    static float static_readValue();

public:
    FooterItemZHeigth(window_t *parent);
};

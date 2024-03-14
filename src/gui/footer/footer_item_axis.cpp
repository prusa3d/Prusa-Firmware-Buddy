/**
 * @file footer_item_axis.cpp
 */

#include "footer_item_axis.hpp"
#include "img_resources.hpp"

FooterItemAxisX::FooterItemAxisX(window_t *parent)
    : FooterItemAxisPos<0>(parent, &img::x_axis_16x16) {}

FooterItemAxisY::FooterItemAxisY(window_t *parent)
    : FooterItemAxisPos<1>(parent, &img::y_axis_16x16) {}

FooterItemAxisZ::FooterItemAxisZ(window_t *parent)
    : FooterItemAxisPos<2>(parent, &img::z_axis_16x16) {}

FooterItemZHeight::FooterItemZHeight(window_t *parent)
    : FooterItemAxisCurrPos<2>(parent, &img::z_axis_16x16) {}

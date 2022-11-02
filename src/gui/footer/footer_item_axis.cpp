/**
 * @file footer_item_axis.cpp
 */

#include "footer_item_axis.hpp"
#include "png_resources.hpp"

FooterItemAxisX::FooterItemAxisX(window_t *parent)
    : FooterItemAxisPos<0>(parent, &png::x_axis_16x16) {}

FooterItemAxisY::FooterItemAxisY(window_t *parent)
    : FooterItemAxisPos<1>(parent, &png::y_axis_16x16) {}

FooterItemAxisZ::FooterItemAxisZ(window_t *parent)
    : FooterItemAxisPos<2>(parent, &png::z_axis_16x16) {}

FooterItemZHeight::FooterItemZHeight(window_t *parent)
    : FooterItemAxisCurrPos<2>(parent, &png::z_axis_16x16) {}

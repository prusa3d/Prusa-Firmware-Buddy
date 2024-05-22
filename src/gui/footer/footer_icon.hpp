/**
 * @file footer_icon.hpp
 * @author Radek Vana
 * @brief icon used in footer
 * require parrent with relative coords
 * @date 2021-03-31
 */

#pragma once

#include "window_icon.hpp"

class FooterIcon : public AddSuperWindow<window_icon_t> {
public:
    FooterIcon(window_t *parent, const img::Resource *icon);
};

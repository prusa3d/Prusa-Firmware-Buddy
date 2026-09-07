/// @file
#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include <guiconfig/GuiDefaults.hpp>
#include <guitypes.hpp>

template <class... T>
class BasicScreenMenu : public ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, T...> {
    using Base = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, T...>;

public:
    BasicScreenMenu(const string_view_utf8 &header_text, const img::Resource *header_icon = nullptr)
        : Base {
            header_text,
            nullptr,
        } {
        if (header_icon) {
            Base::header.SetIcon(header_icon);
        }
    }
};

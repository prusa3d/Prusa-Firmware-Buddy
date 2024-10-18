/**
 * @file window_menu_adv.hpp
 * @brief advanced window representing menu, containing additional widgets like scrollbar
 */

#pragma once

#include <window_menu_bar.hpp>
#include <window_frame.hpp>

namespace window_menu_frame_ns {
constexpr inline Rect16 calc_menu_rect(Rect16 rc) {
    rc.CutPadding(GuiDefaults::MenuPadding);
    rc -= GuiDefaults::MenuScrollbarWidth;
    return rc;
}

constexpr inline Rect16 calc_scroll_bar_rect(Rect16 rc) {
    Rect16 ret = rc;
    ret += Rect16::Left_t(ret.Width() - GuiDefaults::MenuScrollbarWidth);
    ret = GuiDefaults::MenuScrollbarWidth;
    ret = Rect16::Height_t(rc.Height());
    return ret;
}
} // namespace window_menu_frame_ns

/// Wrapper for window menu with scrollbar and such
template <typename Menu>
class WindowExtendedMenu : public window_frame_t {

public:
    template <typename... Args>
    WindowExtendedMenu(window_t *parent, Rect16 rect, Args &&...args)
        : window_frame_t(parent, rect)
        , menu(this, window_menu_frame_ns::calc_menu_rect(rect), std::forward<Args>(args)...)
        , scroll_bar(this, window_menu_frame_ns::calc_scroll_bar_rect(rect), menu) {
        CaptureNormalWindow(menu);
        menu.SetFocus();
    }

public:
    Menu menu;
    MenuScrollbar scroll_bar;
};

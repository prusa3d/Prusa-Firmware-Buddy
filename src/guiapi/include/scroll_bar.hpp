/**
 * @file scroll_bar.hpp
 * @author Radek Vana
 * @brief Scrollbar window item
 * @date 2022-01-21
 */

#pragma once

#include "window.hpp"
#include "GuiDefaults.hpp"

class ScrollBar : public AddSuperWindow<window_t> {
    Rect16::Height_t offset;
    Rect16::Height_t scroll_height;
    static Rect16 calculateRect(Rect16 parrent_rect, Rect16::Width_t w);

public:
    ScrollBar(window_t *parrent, Rect16::Width_t w = GuiDefaults::MenuScrollbarWidth);
    void SetHeightToScroll(Rect16::Height_t height);
    void SetScrollOffset(Rect16::Height_t offset_);

protected:
    virtual void unconditionalDraw() override;
};

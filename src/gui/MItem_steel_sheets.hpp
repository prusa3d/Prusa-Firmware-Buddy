#pragma once

#include <i_window_menu_item.hpp>

class MI_CURRENT_SHEET_PROFILE : public IWindowMenuItem {
    static constexpr const char *const label = N_("Sheet Profile");

    static constexpr Font font = GuiDefaults::FontMenuItems;
    static constexpr auto extension_width = Rect16::W_t((MAX_SHEET_NAME_LENGTH + 2) * width(font) + GuiDefaults::MenuPaddingItems.left + GuiDefaults::MenuPaddingItems.right);

    std::array<char, MAX_SHEET_NAME_LENGTH + 3> extension_str;

public:
    MI_CURRENT_SHEET_PROFILE();

protected:
    void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const override;
    void click(IWindowMenu &) override;
};

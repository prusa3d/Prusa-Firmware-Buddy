/// @file
#pragma once

#include <utils/color.hpp>
#include <i_window_menu_item.hpp>

/// Menu item for picking a color
class MenuItemColor : public IWindowMenuItem {

public:
    /// Pops a dialog prompting for color selection
    /// @returns selected color or nullopt on cancel
    /// Sorry about the optionalception
    static std::optional<OptionalColor> color_prompt_dialog(const string_view_utf8 &title, OptionalColor default_color);

public:
    MenuItemColor(const string_view_utf8 &label);

    OptionalColor value() const {
        return value_;
    }
    void set_value(OptionalColor new_value);

    bool is_editable() const {
        return is_editable_;
    }
    void set_is_editable(bool set);

protected:
    void printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn raster_op) const override;
    void click(IWindowMenu &) override;

private:
    OptionalColor value_;
    bool is_editable_ = true;
};

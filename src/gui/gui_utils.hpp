/// @file
#pragma once

#include <optional>

#include <fonts.hpp>
#include <font_flags.hpp>
#include <Rect16.h>
#include <string_view_utf8.hpp>
#include <guiconfig/GuiDefaults.hpp>
#include <raster_opfn.hpp>

class IWindowMenuItem;

struct AutoSelectFontArgs {
    string_view_utf8 text;
    Rect16 rect;
    Font largest;
    Font smallest;
    bool multiline;
};

/// @returns the largest font that can fit @p text into @p rect
std::optional<Font> auto_select_font(const AutoSelectFontArgs &args);

/// A standard method of indicating a color
void draw_color_swatch(Rect16 rect, Align_t align, OptionalColor color, Color color_back);

/// Utility class for rendering an extension that contains a color, text (a la WiInfo) and possibly an arrow
struct MenuItemColorSwatchExtension {
    static constexpr uint16_t arrow_section_w = 16;
    static constexpr uint16_t color_swatch_section_w = 32;
    static constexpr Font font = GuiDefaults::FontMenuSpecial;

    string_view_utf8 text = {};

    OptionalColor color = std::nullopt;

    /// Shows the color swatch even if the color is null
    bool show_null_color : 1 = true;

    /// If set, shows the arrow icon on the right
    bool show_arrow : 1 = false;

    /// @returns extension width we should allocate (in pixels)
    uint16_t compute_width() const;

    /// Draws the extension
    void draw(const IWindowMenuItem &item, Rect16 extension_rect, Color color_back, ropfn raster_op) const;
};

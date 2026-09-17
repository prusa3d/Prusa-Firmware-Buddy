/// @file
#include "gui_utils.hpp"

#include <str_utils.hpp>
#include <display.hpp>
#include <guiconfig/GuiDefaults.hpp>
#include <WindowMenuInfo.hpp>
#include <img_resources.hpp>

std::optional<Font> auto_select_font(const AutoSelectFontArgs &args) {
    static constexpr std::array font_list {
#if HAS_LARGE_DISPLAY()
        Font::large,
#endif
            Font::big,
            Font::normal,
            Font::small
    };

    const Font *fnt = font_list.begin();

    // Skip larger fonts than largest
    while (*fnt != args.largest) {
        fnt++;

        if (fnt == font_list.end()) {
            return std::nullopt;
        }
    }

    while (true) {
        StringReaderUtf8 reader(args.text);
        const auto *res = resource_font(*fnt);
        const RectTextLayout layout(reader, args.rect.Width() / res->w, args.rect.Height() / res->h, args.multiline ? is_multiline::yes : is_multiline::no);

        if (!layout.has_text_overflown()) {
            return *fnt;
        }

        if (*fnt == args.smallest) {
            return std::nullopt;
        }

        fnt++;
        if (fnt == font_list.end()) {
            return std::nullopt;
        }
    }
}

void draw_color_swatch(Rect16 rect, Align_t align, OptionalColor color, Color color_back) {
    constexpr auto padding = 1;

    const Color border_color = color_back.to_grayscale() < 128 ? COLOR_WHITE : COLOR_BLACK;

    const auto outer_size = std::min<int16_t>(rect.Height(), rect.Width());
    Rect16 outer_rect = Rect16::fromLTWH(0, 0, outer_size, outer_size);
    outer_rect.Align(rect, align);
    display::draw_rounded_rect(outer_rect, color_back, border_color, GuiDefaults::MenuItemCornerRadius, MIC_ALL_CORNERS);

    debug_assert(outer_size > padding * 2);
    const auto inner_size = outer_size - padding * 2;
    const Rect16 inner_rect = Rect16::fromLTWH(outer_rect.Left() + padding, outer_rect.Top() + padding, inner_size, inner_size);

    // No color -> draw black background for the red cross
    display::draw_rounded_rect(inner_rect, border_color, color.value_or(COLOR_BLACK), GuiDefaults::MenuItemCornerRadius, MIC_ALL_CORNERS);

    if (!color.has_value()) {
        display::draw_line(point_ui16_t::from_point(inner_rect.TopLeft()), point_ui16_t::from_point(inner_rect.BottomRight()), COLOR_RED);
        display::draw_line(point_ui16_t::from_point(inner_rect.TopRight()), point_ui16_t::from_point(inner_rect.BottomLeft()), COLOR_RED);
    }
}

uint16_t MenuItemColorSwatchExtension::compute_width() const {
    uint16_t result = 0;
    if (show_arrow) {
        result += arrow_section_w;
    }
    if (color.has_value() || show_null_color) {
        result += color_swatch_section_w;
    }
    result += resource_font(font)->w * text.computeNumUtf8Chars();

    return std::min(result, IWiInfo::max_extension_width());
}

void MenuItemColorSwatchExtension::draw(const IWindowMenuItem &item, Rect16 extension_rect, Color color_back, ropfn raster_op) const {
    if (show_arrow) {
        render_icon_align(extension_rect, &img::arrow_right_10x16, color_back, Align_t::RightCenter(), raster_op);
        extension_rect -= Rect16::Width_t { arrow_section_w };
    }

    if (color.has_value() || show_null_color) {
        constexpr auto margin = 6;

        draw_color_swatch(
            Rect16::fromLTRB(
                // margin * 2 - add a bit more extra spacing before the text
                extension_rect.Right() - color_swatch_section_w + margin * 2,
                extension_rect.Top() + margin,
                extension_rect.Right(),
                extension_rect.Bottom() - margin),

            Align_t::RightCenter(),
            color,
            color_back);

        extension_rect -= Rect16::Width_t { color_swatch_section_w };
    }

    render_text_align(extension_rect, text, font, color_back, IWiInfo::extension_text_color(item), GuiDefaults::MenuPaddingSpecial, Align_t::RightCenter());
}

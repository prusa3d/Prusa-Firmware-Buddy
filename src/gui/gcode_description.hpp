#pragma once

#include <span>
#include "gui.hpp"
#include "gcode_info.hpp"
#include "guiconfig.h"

#if defined(USE_ST7789)
inline constexpr uint16_t SCREEN_WIDTH = display::GetW();
inline constexpr uint16_t SCREEN_HEIGHT = display::GetH();
inline constexpr uint16_t PADDING = 10;
inline constexpr uint16_t TITLE_HEIGHT = 24;
inline constexpr uint16_t LINE_HEIGHT = 15;
inline constexpr uint16_t LINE_SPACING = 5;
inline constexpr uint16_t THUMBNAIL_HEIGHT = GuiDefaults::PreviewThumbnailRect.Height();
#elif defined(USE_ILI9488)
inline constexpr padding_ui8_t PADDING = { 30, GuiDefaults::HeaderHeight + 6, 35, 13 };
inline constexpr uint16_t TITLE_HEIGHT = 24;
inline constexpr uint16_t LINE_HEIGHT = 17;
inline constexpr uint16_t LINE_SPACING = 5;
#endif // USE_<display>

struct description_line_t {
    window_text_t title;
    window_text_t value;
    char value_buffer[32];
    description_line_t(window_frame_t *frame);
    void update(bool has_preview_thumbnail, size_t row, string_view_utf8 title_str, std::function<void(std::span<char> buffer)> make_value);
    static size_t title_width(string_view_utf8 *title_str);
    static size_t value_width(string_view_utf8 *title_str);

private:
    static constexpr size_t calculate_y(bool has_preview_thumbnail, size_t row) {
        size_t y = 0;
#ifdef USE_ST7789
        y = TITLE_HEIGHT + 2 * PADDING;
        if (has_preview_thumbnail) {
            y += THUMBNAIL_HEIGHT + PADDING;
        }
#endif // USE_ST7789
#ifdef USE_ILI9488
        y = GuiDefaults::PreviewThumbnailRect.Top();
        if (has_preview_thumbnail) {
            y += GuiDefaults::PreviewThumbnailRect.Height() + 15;
        }
#endif // USE_ILI9488
        y += row * (LINE_HEIGHT + LINE_SPACING);
        return y;
    }
};

struct GCodeInfoWithDescription {
    description_line_t description_lines[4];

    /// Constructor, that will create empty description, update has to be called later with gcode data
    GCodeInfoWithDescription(window_frame_t *frame);

    /// Update with gcode data
    void update(GCodeInfo &gcode);
};

/**
 * @file gcode_description.hpp
 * @author Michal Rudolf
 * @brief Structure that holds and shows gcode comment info
 * @date 2021-06-18
 */
#pragma once

#include "gui.hpp"
#include "gcode_info.hpp"

static const constexpr uint16_t SCREEN_WIDTH = display::GetW();
static const constexpr uint16_t SCREEN_HEIGHT = display::GetH();
static const constexpr uint16_t PADDING = 10;
static const constexpr uint16_t TITLE_HEIGHT = 24;
static const constexpr uint16_t LINE_HEIGHT = 15;
static const constexpr uint16_t LINE_SPACING = 5;
static const constexpr uint16_t THUMBNAIL_HEIGHT = GuiDefaults::PreviewThumbnailRect.Height();

struct description_line_t {
    window_text_t title;
    window_text_t value;
    char value_buffer[32];
    description_line_t(window_frame_t *frame, bool has_preview_thumbnail, size_t row,
        string_view_utf8 title, const char *value_fmt, ...);
    static size_t title_width(string_view_utf8 *title_str);
    static size_t value_width(string_view_utf8 *title_str);

private:
    static constexpr size_t calculate_y(bool has_preview_thumbnail, size_t row) {
        size_t y = 0;
        y = TITLE_HEIGHT + 2 * PADDING;
        if (has_preview_thumbnail)
            y += THUMBNAIL_HEIGHT + PADDING;
        y += row * (LINE_HEIGHT + LINE_SPACING);
        return y;
    }
};

struct GCodeInfoWithDescription {
    description_line_t description_lines[4];
    GCodeInfoWithDescription(window_frame_t *frame, GCodeInfo &gcode);
};

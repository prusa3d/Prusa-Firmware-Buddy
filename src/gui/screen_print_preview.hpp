// screen_print_preview.hpp
#pragma once
#include "gui.hpp"
#include "window_header.hpp"
#include "window_roll_text.hpp"
#include "ff.h"
#include "screen.hpp"
#include "display.h"
#include "gcode_info.hpp"

static const constexpr uint16_t PADDING = 10;
static const constexpr uint16_t SCREEN_WIDTH = display::GetW();
static const constexpr uint16_t SCREEN_HEIGHT = display::GetH();
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

//todo implement draw, i am using visible property on some description_lines
struct screen_print_preview_data_t : public AddSuperWindow<screen_t> {

    window_roll_text_t title_text;
    window_icon_button_t print_button;
    window_text_t print_label;
    window_icon_button_t back_button;
    window_text_t back_label;
    WindowPreviewThumbnail thumbnail;
    GCodeInfo &gcode;
    bool ignore_wrong_filament;

    GCodeInfoWithDescription gcode_description; //cannot be first

    bool suppress_draw;

public:
    screen_print_preview_data_t();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
    //virtual void unconditionalDraw() override; //todo move draw from event

private:
    bool gcode_file_exists();
};

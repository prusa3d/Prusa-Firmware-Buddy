// screen_print_preview.hpp
#pragma once
#include "gui.hpp"
#include "window_header.hpp"
#include "window_roll_text.hpp"
#include "ff.h"
#include "screen.hpp"

static const constexpr uint16_t PADDING = 10;
static const constexpr uint16_t SCREEN_WIDTH = 240;  //FIXME should be in display.h
static const constexpr uint16_t SCREEN_HEIGHT = 320; //FIXME should be in display.h
static const constexpr uint16_t THUMBNAIL_HEIGHT = 124;
static const constexpr uint16_t TITLE_HEIGHT = 24;
static const constexpr uint16_t LINE_HEIGHT = 15;
static const constexpr uint16_t LINE_SPACING = 5;

struct description_line_t {
    window_text_t title;
    window_text_t value;
    char value_buffer[32];
    description_line_t(window_frame_t *frame, bool has_thumbnail, size_t row,
        string_view_utf8 title, const char *value_fmt, ...);
    static size_t title_width(string_view_utf8 *title_str);
    static size_t value_width(string_view_utf8 *title_str);

private:
    static constexpr size_t calculate_y(bool has_thumbnail, size_t row) {
        size_t y = TITLE_HEIGHT + 2 * PADDING;
        if (has_thumbnail)
            y += THUMBNAIL_HEIGHT + PADDING;
        y += row * (LINE_HEIGHT + LINE_SPACING);
        return y;
    }
};

struct GCodeInfo {
    FIL file;
    bool file_opened;
    bool has_thumbnail;
    char printing_time[16];
    char filament_type[8];
    unsigned filament_used_g;
    unsigned filament_used_mm;
    GCodeInfo();
};

struct GCodeInfoWithDescription : public GCodeInfo {
    description_line_t description_lines[4];
    GCodeInfoWithDescription(window_frame_t *frame);
};

//todo implement draw, i am using visible property on some description_lines
struct screen_print_preview_data_t : public AddSuperWindow<screen_t> {
    window_roll_text_t title_text;
    window_icon_button_t print_button;
    window_text_t print_label;
    window_icon_button_t back_button;
    window_text_t back_label;

    GCodeInfoWithDescription gcode; //cannot be first

    bool redraw_thumbnail;
    bool suppress_draw;

public:
    screen_print_preview_data_t();
    ~screen_print_preview_data_t();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
    //virtual void unconditionalDraw() override; //todo move draw from event

private:
    bool gcode_file_exists();

public:
    //static methods
    // FIXME: the screen_print_preview currently does not copy fpath and fname
    // therefore, their lifetime must be at least as long as the screen's lifetime
    static void SetGcodeFilepath(const char *fpath);
    static const char *GetGcodeFilepath();
    static void SetGcodeFilename(const char *fname);
};

//screen_printing_serial.hpp
#pragma once
#include "gui.hpp"
#include "window_header.hpp"
#include "ff.h"

#define PADDING          10
#define SCREEN_WIDTH     240 //FIXME should be in display.h
#define SCREEN_HEIGHT    320 //FIXME should be in display.h
#define THUMBNAIL_HEIGHT 124
#define TITLE_HEIGHT     24
#define LINE_HEIGHT      15
#define LINE_SPACING     5

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
struct screen_print_preview_data_t : public window_frame_t {
    window_text_t title_text;
    window_icon_button_t print_button;
    window_text_t print_label;
    window_icon_button_t back_button;
    window_text_t back_label;

    GCodeInfoWithDescription gcode; //cannot be first

    bool redraw_thumbnail;

public:
    screen_print_preview_data_t();
    ~screen_print_preview_data_t();

private:
    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
    //virtual void unconditionalDraw() override; //todo move draw from event
    bool gcode_file_exists();

public:
    //static methods
    // FIXME: the screen_print_preview currently does not copy fpath and fname
    // therefore, their lifetime must be at least as long as the screen's lifetime
    static void SetGcodeFilepath(const char *fpath);
    static const char *GetGcodeFilepath();
    static void SetGcodeFilename(const char *fname);
};

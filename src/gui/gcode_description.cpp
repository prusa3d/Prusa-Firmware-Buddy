//gcode_description.cpp
#include "gcode_description.hpp"
#include <cstdarg>
#include "resource.h"

size_t description_line_t::title_width(string_view_utf8 *title_str) {
    return title_str->computeNumUtf8CharsAndRewind() * resource_font(IDR_FNT_SMALL)->w;
}

size_t description_line_t::value_width(string_view_utf8 *title_str) {
    return SCREEN_WIDTH - PADDING * 2 - title_width(title_str) - 1;
}

description_line_t::description_line_t(window_frame_t *frame, bool has_preview_thumbnail, size_t row, string_view_utf8 title_str, const char *value_fmt, ...)
    : title(frame, Rect16(PADDING, calculate_y(has_preview_thumbnail, row), title_width(&title_str), LINE_HEIGHT), is_multiline::no)
    , value(frame, Rect16(SCREEN_WIDTH - PADDING - value_width(&title_str), calculate_y(has_preview_thumbnail, row), value_width(&title_str), LINE_HEIGHT), is_multiline::no) {
    title.SetText(title_str);
    title.SetAlignment(Align_t::LeftBottom());
    title.SetPadding({ 0, 0, 0, 0 });
    title.font = resource_font(IDR_FNT_SMALL);
    title.SetTextColor(COLOR_GRAY);

    va_list args;
    va_start(args, value_fmt);
    vsnprintf(value_buffer, sizeof(value_buffer), value_fmt, args);
    va_end(args);
    // this MakeRAM is safe - value_buffer is allocated in RAM for the lifetime of line
    value.SetText(string_view_utf8::MakeRAM((const uint8_t *)value_buffer));
    value.SetAlignment(Align_t::RightBottom());
    value.SetPadding({ 0, 0, 0, 0 });
    value.font = resource_font(IDR_FNT_SMALL);
}

GCodeInfoWithDescription::GCodeInfoWithDescription(window_frame_t *frame, GCodeInfo &gcode)
    : description_lines {
        gcode.printing_time[0] ? description_line_t(frame, gcode.has_preview_thumbnail, 0, _("Print Time"), "%s", gcode.printing_time) : description_line_t(frame, gcode.has_preview_thumbnail, 0, _("Print Time"), "unknown"),
        gcode.has_preview_thumbnail ? description_line_t(frame, gcode.has_preview_thumbnail, 1, _("Material"), "%s/%u g/%0.2f m", gcode.filament_type, gcode.filament_used_g, (double)((float)gcode.filament_used_mm / 1000.0F)) : description_line_t(frame, gcode.has_preview_thumbnail, 1, _("Material"), "%s", gcode.filament_type),
        { frame, gcode.has_preview_thumbnail, 2, _("Used Filament"), "%.2f m", (double)((float)gcode.filament_used_mm / 1000.0F) },
        { frame, gcode.has_preview_thumbnail, 3, string_view_utf8::MakeNULLSTR(), "%.0f g", (double)gcode.filament_used_g }
    } {
    if (gcode.has_preview_thumbnail || !gcode.filament_type[0]) {
        description_lines[2].value.Hide();
        description_lines[2].title.Hide();
        description_lines[2].value.Validate(); // == do not redraw
        description_lines[2].title.Validate(); // == do not redraw
    }
    if (gcode.has_preview_thumbnail || !(gcode.filament_used_mm && gcode.filament_used_g)) {
        description_lines[3].value.Hide();
        description_lines[3].title.Hide();
        description_lines[3].value.Validate(); // == do not redraw
        description_lines[3].title.Validate(); // == do not redraw
    }
}

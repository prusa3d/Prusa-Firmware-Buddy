/**
 * @file window_thumbnail.cpp
 */

#include "window_thumbnail.hpp"
#include "gcode_reader_interface.hpp"
#include "display.hpp"
#include <bsod/bsod.h>

//-------------------------- Thumbnail --------------------------------------

WindowThumbnail::WindowThumbnail(window_t *parent, Rect16 rect)
    : window_icon_t(parent, rect, nullptr) {
}

//------------------------- Preview Thumbnail ------------------------------------

WindowPreviewThumbnail::WindowPreviewThumbnail(window_t *parent, Rect16 rect)
    : WindowThumbnail(parent, rect) {
}

void WindowPreviewThumbnail::unconditionalDraw() {
    gcode_reader = AnyGcodeFormatReader { GCodeInfo::getInstance().GetGcodeFilepath() };
    if (!gcode_reader.is_open()) {
        return;
    }
    if (AbstractByteReader *thumbnail_reader = gcode_reader->stream_thumbnail_start(Width(), Height(), IGcodeReader::ImgType::QOI)) {
        display::draw_img(point_ui16(Left(), Top()), *thumbnail_reader);
    }
}

//------------------------- Progress Thumbnail -----------------------------------

WindowProgressThumbnail::WindowProgressThumbnail(window_t *parent, Rect16 rect, size_t allowed_old_thumbnail_width)
    : WindowThumbnail(parent, rect)
    , old_allowed_width(allowed_old_thumbnail_width) {
    gcode_reader = AnyGcodeFormatReader { GCodeInfo::getInstance().GetGcodeFilepath() };
}

Rect16::Left_t WindowProgressThumbnail::get_old_left() {
    debug_assert(old_allowed_width < Width()); // currently unsupported otherwise
    const auto cur_rect { GetRect() };
    // center image by moving left by half of the difference of the widths
    return cur_rect.Left() + (cur_rect.Width() - old_allowed_width) / 2;
}

void WindowProgressThumbnail::unconditionalDraw() {
    if (!gcode_reader.is_open()) {
        return;
    }

    AbstractByteReader *thumbnail_reader = gcode_reader->stream_thumbnail_start(Width(), Height(), IGcodeReader::ImgType::QOI);
    if (thumbnail_reader) {
        display::draw_img(point_ui16(Left(), Top()), *thumbnail_reader);
    } else {
        if (old_allowed_width < Width() && (thumbnail_reader = gcode_reader->stream_thumbnail_start(old_allowed_width, Height(), IGcodeReader::ImgType::QOI))) {
            display::draw_img(point_ui16(get_old_left(), Top()), *thumbnail_reader);
        }
    }
}

void WindowProgressThumbnail::pauseDeinit() {
    gcode_reader = AnyGcodeFormatReader {};
}

void WindowProgressThumbnail::pauseReinit() {
    gcode_reader = AnyGcodeFormatReader { GCodeInfo::getInstance().GetGcodeFilepath() };
}

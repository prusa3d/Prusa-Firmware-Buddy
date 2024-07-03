/**
 * @file window_thumbnail.cpp
 */

#include "window_thumbnail.hpp"
#include "gcode_thumb_decoder.h"
#include "display.hpp"

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

    FILE f {};
    img::Resource res(&f);

    if (!gcode_reader->stream_thumbnail_start(Width(), Height(), IGcodeReader::ImgType::QOI)) {
        return;
    }

    if (f_gcode_thumb_open(*gcode_reader, &f) != 0) {
        return;
    }

    display::draw_img(point_ui16(Left(), Top()), res);
    f_gcode_thumb_close(&f);
}

//------------------------- Progress Thumbnail -----------------------------------

WindowProgressThumbnail::WindowProgressThumbnail(window_t *parent, Rect16 rect, size_t allowed_old_thumbnail_width)
    : WindowThumbnail(parent, rect)
    , redraw_whole(true)
    , old_allowed_width(allowed_old_thumbnail_width) {
    gcode_reader = AnyGcodeFormatReader { GCodeInfo::getInstance().GetGcodeFilepath() };
}

Rect16::Left_t WindowProgressThumbnail::get_old_left() {
    assert(old_allowed_width < Width()); // currently unsupported otherwise
    const auto cur_rect { GetRect() };
    // center image by moving left by half of the difference of the widths
    return cur_rect.Left() + (cur_rect.Width() - old_allowed_width) / 2;
}

void WindowProgressThumbnail::unconditionalDraw() {
    if (!gcode_reader.is_open()) {
        return;
    }

    // TODO: check if redraw_whole is still needed, Invalidate might be enough now
    if (!redraw_whole) { // No longer drawing image per-progress, so draw is only valid if the whole image wants to be drawn
        return;
    }

    FILE f {};
    img::Resource res(&f);

    bool have_old_alternative { false };

    if (!gcode_reader->stream_thumbnail_start(Width(), Height(), IGcodeReader::ImgType::QOI)) {
        if (old_allowed_width < Width() && !gcode_reader->stream_thumbnail_start(old_allowed_width, Height(), IGcodeReader::ImgType::QOI)) {
            return;
        } else {
            have_old_alternative = true;
        }
    }

    if (f_gcode_thumb_open(*gcode_reader, &f) != 0) {
        return;
    }

    // Draw whole thumbnail:
    display::draw_img(point_ui16(have_old_alternative ? get_old_left() : Left(), Top()), res);

    redraw_whole = false;
    f_gcode_thumb_close(&f);
}

void WindowProgressThumbnail::pauseDeinit() {
    gcode_reader = AnyGcodeFormatReader {};
}

void WindowProgressThumbnail::pauseReinit() {
    gcode_reader = AnyGcodeFormatReader { GCodeInfo::getInstance().GetGcodeFilepath() };
}

void WindowProgressThumbnail::redrawWhole() {
    redraw_whole = true;
}

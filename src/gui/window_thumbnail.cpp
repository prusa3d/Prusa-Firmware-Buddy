/**
 * @file window_thumbnail.cpp
 */

#include "window_thumbnail.hpp"
#include "gcode_thumb_decoder.h"

//-------------------------- Thumbnail --------------------------------------

WindowThumbnail::WindowThumbnail(window_t *parent, Rect16 rect)
    : AddSuperWindow<window_icon_t>(parent, rect, nullptr) {
}

//------------------------- Preview Thumbnail ------------------------------------

WindowPreviewThumbnail::WindowPreviewThumbnail(window_t *parent, Rect16 rect)
    : AddSuperWindow<WindowThumbnail>(parent, rect) {
}

void WindowPreviewThumbnail::unconditionalDraw() {
    gcode_reader.open(GCodeInfo::getInstance().GetGcodeFilepath());
    if (!gcode_reader.is_open()) {
        return;
    }
    IGcodeReader *const reader = gcode_reader.get();

    FILE f {};
    img::Resource res(&f);

    if (!reader->stream_thumbnail_start(Width(), Height(), IGcodeReader::ImgType::QOI)) {
        return;
    }

    if (f_gcode_thumb_open(*reader, &f) != 0) {
        return;
    }

    display::DrawImg(point_ui16(Left(), Top()), res);
}

//------------------------- Progress Thumbnail -----------------------------------

WindowProgressThumbnail::WindowProgressThumbnail(window_t *parent, Rect16 rect, size_t allowed_old_thumbnail_width)
    : AddSuperWindow<WindowThumbnail>(parent, rect)
    , redraw_whole(true)
    , old_allowed_width(allowed_old_thumbnail_width) {
    gcode_reader.open(GCodeInfo::getInstance().GetGcodeFilepath());
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

    IGcodeReader *const reader = gcode_reader.get();

    FILE f {};
    img::Resource res(&f);

    bool have_old_alternative { false };

    if (!reader->stream_thumbnail_start(Width(), Height(), IGcodeReader::ImgType::QOI)) {
        if (old_allowed_width < Width() && !reader->stream_thumbnail_start(old_allowed_width, Height(), IGcodeReader::ImgType::QOI)) {
            return;
        } else {
            have_old_alternative = true;
        }
    }

    if (f_gcode_thumb_open(*reader, &f) != 0) {
        return;
    }

    // Draw whole thumbnail:
    display::DrawImg(point_ui16(have_old_alternative ? get_old_left() : Left(), Top()), res);

    redraw_whole = false;
    f_gcode_thumb_close(&f);
}

void WindowProgressThumbnail::pauseDeinit() {
    gcode_reader.close();
}

void WindowProgressThumbnail::pauseReinit() {
    gcode_reader.open(GCodeInfo::getInstance().GetGcodeFilepath());
}

void WindowProgressThumbnail::redrawWhole() {
    redraw_whole = true;
}

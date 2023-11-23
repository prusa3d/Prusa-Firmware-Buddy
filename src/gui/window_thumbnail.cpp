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

WindowPreviewThumbnail::~WindowPreviewThumbnail() {
}

void WindowPreviewThumbnail::unconditionalDraw() {
    gcode_reader.open(GCodeInfo::getInstance().GetGcodeFilepath());
    if (!gcode_reader.is_open()) {
        return;
    }
    IGcodeReader *const reader = gcode_reader.get();

    FILE f {};
    img::Resource res(&f);

    if (reader->stream_thumbnail_start(Width(), Height(), IGcodeReader::ImgType::QOI)) {
        if (f_gcode_thumb_open(*reader, &f) == 0) {
            display::DrawImg(point_ui16(Left(), Top()), res);
        }
    }
}

//------------------------- Progress Thumbnail -----------------------------------

WindowProgressThumbnail::WindowProgressThumbnail(window_t *parent, Rect16 rect)
    : AddSuperWindow<WindowThumbnail>(parent, rect)
    , progress_percentage(-1)
    , last_percentage_drawn(-1)
    , redraw_whole(true) {
    gcode_reader.open(GCodeInfo::getInstance().GetGcodeFilepath());
}

WindowProgressThumbnail::~WindowProgressThumbnail() {
}

void WindowProgressThumbnail::unconditionalDraw() {
    if (!gcode_reader.is_open()) {
        return;
    }
    IGcodeReader *const reader = gcode_reader.get();

    if (last_percentage_drawn > progress_percentage) {
        last_percentage_drawn = 0;
        redraw_whole = true;
    }
    FILE f {};
    img::Resource res(&f);

    if (reader->stream_thumbnail_start(Width(), Height(), IGcodeReader::ImgType::QOI)) {
        if (f_gcode_thumb_open(*reader, &f) == 0) {

            ropfn raster_op;
            int progress = std::clamp(int(progress_percentage), 0, 100);
            uint16_t progress_local_y = Height() - (progress * Height()) / 100;
            if (redraw_whole) {
                // Draw whole thumbnail:

                // desaturated part
                if (progress_local_y) { // draw only when non-zero height
                    raster_op.desatur = is_desaturated::yes;
                    display::DrawImg(point_ui16(Left(), Top()), res, GetBackColor(), raster_op, Rect16(0, 0, Width(), progress_local_y));
                }

                // file has to be reset to be able to read image again
                if (reader->stream_thumbnail_start(Width(), Height(), IGcodeReader::ImgType::QOI)) {
                    uint16_t saturated_height = Height() - progress_local_y;
                    if (saturated_height) { // draw only when non-zero height
                        // saturated part
                        raster_op.desatur = is_desaturated::no;
                        display::DrawImg(point_ui16(Left(), Top()), res, GetBackColor(), raster_op, Rect16(0, progress_local_y, Width(), saturated_height));
                    }
                }

            } else {
                // Draw few newly saturated lines
                uint16_t progress_change_height = (Height() / 100) * (progress - last_percentage_drawn + 2); // + 2 for round up/down loss
                raster_op.desatur = is_desaturated::no;
                display::DrawImg(point_ui16(Left(), Top()), res, GetBackColor(), raster_op, Rect16(0, progress_local_y, Width(), progress_change_height));
            }

            last_percentage_drawn = progress_percentage;
            redraw_whole = false;
            f_gcode_thumb_close(&f);
        }
    }
}

void WindowProgressThumbnail::pauseDeinit() {
    gcode_reader.close();
}

void WindowProgressThumbnail::pauseReinit() {
    gcode_reader.open(GCodeInfo::getInstance().GetGcodeFilepath());
}

bool WindowProgressThumbnail::updatePercentage(int8_t cmp) {
    bool ret = false;
    if (cmp != progress_percentage) {
        ret = true;
        progress_percentage = cmp;
    }
    return ret;
}

void WindowProgressThumbnail::redrawWhole() {
    redraw_whole = true;
}

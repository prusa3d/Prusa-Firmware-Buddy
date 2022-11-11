/**
 * @file window_thumbnail.cpp
 */

#include "window_thumbnail.hpp"
#include "gcode_thumb_decoder.h"
#include "gcode_file.h"

//-------------------------- Thumbnail --------------------------------------

WindowThumbnail::WindowThumbnail(window_t *parent, Rect16 rect)
    : AddSuperWindow<window_icon_t>(parent, rect, nullptr)
    , gcode_info(GCodeInfo::getInstance()) {
}

//------------------------- Preview Thumbnail ------------------------------------

WindowPreviewThumbnail::WindowPreviewThumbnail(window_t *parent, Rect16 rect)
    : AddSuperWindow<WindowThumbnail>(parent, rect) {
    gcode_info.initFile(GCodeInfo::GI_INIT_t::PREVIEW);
}

WindowPreviewThumbnail::~WindowPreviewThumbnail() {
    gcode_info.deinitFile();
}

void WindowPreviewThumbnail::unconditionalDraw() {
    if (!gcode_info.file)
        return;
    FILE f = { 0 };

    png::Resource res("", 0, 0, 0, 0);
    res.file = &f;

    fseek(gcode_info.file, 0, SEEK_SET);
    GCodeThumbDecoder gd(gcode_info.file, Width(), Height(), true);
    if (f_gcode_thumb_open(&gd, &f) == 0) {
        display::DrawPng(point_ui16(Left(), Top()), res);
        f_gcode_thumb_close(&f);
    }
}

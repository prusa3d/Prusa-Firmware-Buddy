/**
 * @file window_thumbnail.hpp
 * @brief window holding a thumbnail
 */

#pragma once

#include "window_icon.hpp"
#include "gcode_info.hpp"

class WindowThumbnail : public AddSuperWindow<window_icon_t> {
public:
    WindowThumbnail(window_t *parent, Rect16 rect);

protected:
    virtual void unconditionalDraw() = 0; // force derived to override unconditionalDraw because the inherited one is always wrong
    AnyGcodeFormatReader gcode_reader;
};

class WindowProgressThumbnail : public AddSuperWindow<WindowThumbnail> {
    bool redraw_whole; /**< stores information if thumbnail have to be restored whole or not*/

    const size_t old_allowed_width; // Holds the width of an alternative 'old' thumbnail that is less wide than the previous one

    /**
     * @brief Gets the Left() of a rectangle from 'old' thumbnail, using OldSlicerProgressImgWidth instead of SlicerProgressImgWidth
     *
     */
    Rect16::Left_t get_old_left();

public:
    /**
     * @brief
     *
     * @param parent
     * @param rect
     * @param allowed_old_thumbnail_width Sets the value of old thumbnail width, in case the gcode contains 'older' (narrower) img. Default value results in invalid old value
     */
    WindowProgressThumbnail(window_t *parent, Rect16 rect, size_t allowed_old_thumbnail_width = GuiDefaults::ScreenWidth + 1);
    bool updatePercentage(int8_t cmp);
    void redrawWhole();
    void pauseDeinit();
    void pauseReinit();

protected:
    void unconditionalDraw() override;
};

class WindowPreviewThumbnail : public AddSuperWindow<WindowThumbnail> {
public:
    WindowPreviewThumbnail(window_t *parent, Rect16 rect);

protected:
    void unconditionalDraw() override;
};

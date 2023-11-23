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
    virtual void unconditionalDraw() = 0;
    AnyGcodeFormatReader gcode_reader;
};

class WindowProgressThumbnail : public AddSuperWindow<WindowThumbnail> {
    int8_t progress_percentage; /**< stores progress for progress type calculation */
    int8_t last_percentage_drawn; /**< stores last progress percentage for making procentage diff*/
    bool redraw_whole; /**< stores information if thumbnail have to be restored whole or not*/

public:
    WindowProgressThumbnail(window_t *parent, Rect16 rect);
    ~WindowProgressThumbnail();
    bool updatePercentage(int8_t cmp);
    void redrawWhole();
    void pauseDeinit();
    void pauseReinit();

protected:
    virtual void unconditionalDraw() override;
};

class WindowPreviewThumbnail : public AddSuperWindow<WindowThumbnail> {
public:
    WindowPreviewThumbnail(window_t *parent, Rect16 rect);
    ~WindowPreviewThumbnail();

protected:
    virtual void unconditionalDraw() override;
};

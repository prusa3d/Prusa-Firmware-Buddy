/**
 * @file window_icon.hpp
 * @brief window holding a picture
 */

#pragma once

#include "window.hpp"
#include "gcode_info.hpp"
#include "png_resources.hpp"

/** @brief Creates an image rendered to the screen
 *  that is redrawn by the GUI loop as needed
 *
 *  If the image is specified by an open file, the object takes
 *  ownership of it and will close it on SetIdRes,
 *  setFile or on its destruction.
 */
class window_icon_t : public AddSuperWindow<window_aligned_t> {
    const png::Resource *pRes = nullptr;

public:
    void SetRes(const png::Resource *res) {
        if (pRes != res) {
            pRes = res;
            Invalidate();
        }
    }

    window_icon_t(window_t *parent, Rect16 rect, const png::Resource *res, is_closed_on_click_t close = is_closed_on_click_t::no);

    window_icon_t(window_t *parent, const png::Resource *res, point_i16_t pt, padding_ui8_t padding = { 0, 0, 0, 0 }, is_closed_on_click_t close = is_closed_on_click_t::no);

protected:
    virtual void unconditionalDraw() override;
    virtual void setRedLayout() override;
    virtual void setBlackLayout() override;
};

class window_icon_button_t : public AddSuperWindow<window_icon_t> {
    ButtonCallback callback;

public:
    window_icon_button_t(window_t *parent, Rect16 rect, const png::Resource *res, ButtonCallback cb);
    void SetAction(ButtonCallback cb) { callback = cb; }

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

class window_icon_hourglass_t : public AddSuperWindow<window_icon_t> {
    enum { ANIMATION_STEPS = 5,
        ANIMATION_STEP_MS = 500 };
    uint32_t start_time; //todo use window timer
    color_t animation_color;
    uint8_t phase;

public:
    window_icon_hourglass_t(window_t *parent, point_i16_t pt, padding_ui8_t padding = { 0, 0, 0, 0 }, is_closed_on_click_t close = is_closed_on_click_t::no);

protected:
    virtual void invalidate(Rect16 validation_rect) override;
    virtual void unconditionalDraw() override;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

#include "wizard_config.hpp"
class WindowIcon_OkNg : public AddSuperWindow<window_aligned_t> {
    enum { ANIMATION_STEP_MS = 128 };

public:
    WindowIcon_OkNg(window_t *parent, point_i16_t pt, SelftestSubtestState_t state = SelftestSubtestState_t::undef, padding_ui8_t padding = { 0, 0, 0, 0 });
    SelftestSubtestState_t GetState() const;
    void SetState(SelftestSubtestState_t s);

protected:
    virtual void unconditionalDraw() override;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    SelftestSubtestState_t state;
};

class WindowThumbnail : public AddSuperWindow<window_icon_t> {
public:
    WindowThumbnail(window_t *parent, Rect16 rect);

protected:
    virtual void unconditionalDraw() = 0;
    GCodeInfo &gcode_info; /**< information about current gcode (singleton)*/
};

class WindowPreviewThumbnail : public AddSuperWindow<WindowThumbnail> {
public:
    WindowPreviewThumbnail(window_t *parent, Rect16 rect);
    ~WindowPreviewThumbnail();

protected:
    virtual void unconditionalDraw() override;
};

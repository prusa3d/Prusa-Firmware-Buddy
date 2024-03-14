/**
 * @file window_icon.hpp
 * @brief window holding a picture
 */

#pragma once

#include "window.hpp"

/** @brief Creates an image rendered to the screen
 *  that is redrawn by the GUI loop as needed
 *
 */
class window_icon_t : public AddSuperWindow<window_aligned_t> {
    const img::Resource *pRes = nullptr;

public:
    enum class Center { x,
        y };

    void SetRes(const img::Resource *res) {
        if (pRes != res) {
            pRes = res;
            Invalidate();
        }
    }

    bool IsIconValid() { return pRes ? true : false; }

    window_icon_t(window_t *parent, Rect16 rect, const img::Resource *res, is_closed_on_click_t close = is_closed_on_click_t::no);

    window_icon_t(window_t *parent, const img::Resource *res, point_i16_t pt, padding_ui8_t padding = { 0, 0, 0, 0 }, is_closed_on_click_t close = is_closed_on_click_t::no);

    /**
     * @brief ctor co center in one axis (for both axis different parameters would be needed)
     *
     * @param parent        higher level window (frame / screen)
     * @param res           pointer to png resource
     * @param pt            point where png can be drawn (1 coordinate will be shifted)
     * @param center        which axis shall be centered
     * @param center_size   size limiting centering; for example 1px image pt = {0,0}, center_size = 3, will be drown at {1,0}
     * @param close
     */
    window_icon_t(window_t *parent, const img::Resource *res, point_i16_t pt, Center center, size_t center_size, is_closed_on_click_t close = is_closed_on_click_t::no);

protected:
    virtual void unconditionalDraw() override;
    virtual void setRedLayout() override;
    virtual void setBlackLayout() override;
    virtual void setBlueLayout() override;
};

class window_icon_button_t : public AddSuperWindow<window_icon_t> {
    ButtonCallback callback;

public:
    window_icon_button_t(window_t *parent, Rect16 rect, const img::Resource *res, ButtonCallback cb);
    void SetAction(ButtonCallback cb) { callback = cb; }

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

/**
 * @brief Iconned button - fast draw
 * Special version requiring 3 pngs with matching size (normal, focused, disabled)
 * does not support padding
 */
class WindowMultiIconButton : public AddSuperWindow<window_t> {
public:
    struct Pngs {
        const img::Resource &normal;
        const img::Resource &focused;
        const img::Resource &disabled;
    };

private:
    const Pngs *pRes = nullptr; // [normal, focused, shadowed(disabled)]
    ButtonCallback callback;

public:
    void SetRes(const Pngs *res) {
        if (pRes != res) {
            pRes = res;
            Invalidate();
        }
    }

    WindowMultiIconButton(window_t *parent, Rect16 rc, const Pngs *res, ButtonCallback cb);
    WindowMultiIconButton(window_t *parent, point_i16_t pt, const Pngs *res, ButtonCallback cb);
    void SetAction(ButtonCallback cb) { callback = cb; }

protected:
    virtual void unconditionalDraw() override;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

class window_icon_hourglass_t : public AddSuperWindow<window_icon_t> {
    enum { ANIMATION_STEPS = 5,
        ANIMATION_STEP_MS = 500 };
    uint32_t start_time; // todo use window timer
    color_t animation_color;
    uint8_t phase;

public:
    window_icon_hourglass_t(window_t *parent, point_i16_t pt, padding_ui8_t padding = { 0, 0, 0, 0 }, is_closed_on_click_t close = is_closed_on_click_t::no);

protected:
    virtual void invalidate(Rect16 validation_rect) override;
    virtual void unconditionalDraw() override;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

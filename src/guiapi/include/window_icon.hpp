// window_icon.hpp

#pragma once

#include "window.hpp"

struct window_icon_t : public window_t {
    uint16_t id_res;
    uint8_t alignment;
    uint16_t GetIdRes() const { return id_res; }
    void SetIdRes(int16_t id);

    window_icon_t(window_t *parent, Rect16 rect, uint16_t id_res, is_closed_on_click_t close = is_closed_on_click_t::no);
    window_icon_t(window_t *parent, uint16_t id_res, point_i16_t pt, padding_ui8_t padding = { 0, 0, 0, 0 }, is_closed_on_click_t close = is_closed_on_click_t::no);
    bool IsShadowed() const;
    void Shadow();
    void Unshadow();

    static size_ui16_t CalculateMinimalSize(uint16_t id_res); //works for center alignment
protected:
    virtual void unconditionalDraw() override;
};

struct window_icon_button_t : public window_icon_t {
    ButtonCallback callback;

    window_icon_button_t(window_t *parent, Rect16 rect, uint16_t id_res, ButtonCallback cb);

protected:
    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
};

class window_icon_hourglass_t : public window_icon_t {
    enum { ANIMATION_STEPS = 5,
        ANIMATION_STEP_MS = 500 };
    uint32_t start_time; //todo use window timer
    color_t animation_color;
    uint8_t phase;

public:
    window_icon_hourglass_t(window_t *parent, point_i16_t pt, padding_ui8_t padding = { 0, 0, 0, 0 }, is_closed_on_click_t close = is_closed_on_click_t::no);

protected:
    virtual void unconditionalDraw() override;
    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
};

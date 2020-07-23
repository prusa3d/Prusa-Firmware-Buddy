// window_icon.hpp

#pragma once

#include "window.hpp"

struct window_icon_t : public window_t {
    uint16_t id_res;
    uint8_t alignment;
    uint16_t GetIdRes() const { return id_res; }
    void SetIdRes(int16_t id);

    window_icon_t(window_t *parent, rect_ui16_t rect, uint16_t id_res);
    bool IsBWSwapped() const;
    void SwapBW();
    void UnswapBW();

protected:
    virtual void unconditionalDraw() override;
};

struct window_icon_button_t : public window_icon_t {
    ButtonCallback cb;

    window_icon_button_t(window_t *parent, rect_ui16_t rect, uint16_t id_res, ButtonCallback cb);

protected:
    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
};

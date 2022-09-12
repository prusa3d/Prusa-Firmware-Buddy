/**
 * @file window_menu_adv.hpp
 * @brief advanced window representing menu, containing additional widgets like scrollbar
 */

#pragma once

#include "window_menu.hpp"

class WindowMenuAdv : public AddSuperWindow<window_frame_t> {
    WindowMenu menu;

public:
    WindowMenuAdv(window_t *parent, Rect16 rect, IWinMenuContainer *pContainer, uint8_t index = 0);

    void BindContainer(IWinMenuContainer &container) {
        menu.BindContainer(container);
    }

    void InitState(screen_init_variant::menu_t var) { menu.InitState(var); }
    screen_init_variant::menu_t GetCurrentState() const { return menu.GetCurrentState(); }

    void Increment(int dif) { menu.Increment(dif); }
    void Decrement(int dif) { menu.Decrement(dif); }

    void Show(IWindowMenuItem &item) { menu.Show(item); }
    bool Hide(IWindowMenuItem &item) { return menu.Hide(item); }

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

using window_menu_t = WindowMenuAdv;

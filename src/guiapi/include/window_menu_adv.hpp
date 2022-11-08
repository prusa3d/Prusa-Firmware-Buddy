/**
 * @file window_menu_adv.hpp
 * @brief advanced window representing menu, containing additional widgets like scrollbar
 */

#pragma once

#include "window_menu.hpp"
#include "window_menu_bar.hpp"
#include "guiconfig.h"

class WindowMenuAdv : public AddSuperWindow<window_frame_t> {
    WindowMenu menu;
    MenuScrollbar bar;

public:
    WindowMenuAdv(window_t *parent, Rect16 rect, IWinMenuContainer *pContainer, uint8_t index = 0);

    void BindContainer(IWinMenuContainer &container) {
        menu.BindContainer(container);
    }

    void InitState(screen_init_variant::menu_t var) { menu.InitState(var); }
    screen_init_variant::menu_t GetCurrentState() const { return menu.GetCurrentState(); }

    bool SetIndex(uint8_t index) { return menu.SetIndex(index); }
    void Increment(int dif) { menu.Increment(dif); }
    void Decrement(int dif) { menu.Decrement(dif); }
    std::optional<size_t> GetIndex() const { return menu.GetIndex(); }

    void Show(IWindowMenuItem &item) { menu.Show(item); }
    bool Hide(IWindowMenuItem &item) { return menu.Hide(item); }
    bool SwapVisibility(IWindowMenuItem &item0, IWindowMenuItem &item1) { return menu.SwapVisibility(item0, item1); }

    IWindowMenuItem *GetItem(uint8_t index) const { return menu.GetItem(index); }
    IWindowMenuItem *GetActiveItem() const { return menu.GetActiveItem(); }
    bool SetActiveItem(IWindowMenuItem &item) { return menu.SetActiveItem(item); }

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

#if (MENU_HAS_SCROLLBAR)
using window_menu_t = WindowMenuAdv;
#else
using window_menu_t = WindowMenu;
#endif

#pragma once

#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.hpp"
#include "window_menu.hpp"
#include "WinMenuContainer.hpp"
#include "WindowMenuItems.hpp"
#include <stdint.h>
#include "resource.h"
#include "screen.hpp"
#include <new>

//parent to not repeat code in templates
class IScreenMenu : public AddSuperWindow<screen_t> {
protected:
    constexpr static const char *no_labelS = "MISSING";
    static string_view_utf8 no_label;
    window_header_t header;
    window_menu_t menu;
    StatusFooter footer;

public:
    IScreenMenu(window_t *parent, string_view_utf8 label, EFooter FOOTER);
    void unconditionalDrawItem(uint8_t index);

    virtual void InitState(screen_init_variant var) override;
    virtual screen_init_variant GetCurrentState() const override;
};

template <EFooter FOOTER, class... T>
class ScreenMenu : public AddSuperWindow<IScreenMenu> {
protected:
    //std::array<window_t*,sizeof...(T)> pElements;//todo menu item is not a window
    WinMenuContainer<T...> container;

public:
    ScreenMenu(string_view_utf8 label, window_t *parent = nullptr);

    //compiletime access by index
    template <std::size_t I>
    decltype(auto) Item() {
        return std::get<I>(container.menu_items);
    }
    //compiletime access by type
    template <class TYPE>
    decltype(auto) Item() {
        return std::get<TYPE>(container.menu_items);
    }

    template <class ITEM>
    void DisableItem() {
        if (Item<ITEM>().IsEnabled()) {
            Item<ITEM>().Disable();
            Invalidate();
        }
    }
    template <class ITEM>
    void EnableItem() {
        if (!Item<ITEM>().IsEnabled()) {
            Item<ITEM>().Enable();
            Invalidate();
        }
    }
};

template <EFooter FOOTER, class... T>
ScreenMenu<FOOTER, T...>::ScreenMenu(string_view_utf8 label, window_t *parent)
    : AddSuperWindow<IScreenMenu>(parent, label, FOOTER) {
    menu.pContainer = &container;
    menu.GetActiveItem()->SetFocus(); //set focus on new item//containder was not valid during construction, have to set its index again
}

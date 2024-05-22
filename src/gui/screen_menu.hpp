#pragma once

#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.hpp"
#include "window_menu_adv.hpp"
#include "WinMenuContainer.hpp"
#include "WindowMenuItems.hpp"
#include <stdint.h>
#include "screen.hpp"
#include <new>

// parent to not repeat code in templates
class IScreenMenu : public AddSuperWindow<screen_t> {
protected:
    window_header_t header;
    window_menu_t menu;
    StatusFooter footer;

public:
    IScreenMenu(window_t *parent, string_view_utf8 label, EFooter FOOTER);

    virtual void InitState(screen_init_variant var) override;
    virtual screen_init_variant GetCurrentState() const override;
};

template <EFooter FOOTER, class... T>
class ScreenMenu : public AddSuperWindow<IScreenMenu> {
protected:
    // std::array<window_t*,sizeof...(T)> pElements;//todo menu item is not a window
    WinMenuContainer<T...> container;

public:
    ScreenMenu(string_view_utf8 label, window_t *parent = nullptr)
        : AddSuperWindow<IScreenMenu>(parent, label, FOOTER) {
        menu.BindContainer(container);
    }

    // compile time access by index
    template <std::size_t I>
    decltype(auto) Item() {
        return std::get<I>(container.menu_items);
    }
    // compile time access by type
    template <class TYPE>
    decltype(auto) Item() {
        return std::get<TYPE>(container.menu_items);
    }

    template <class ITEM>
    void DisableItem() {
        if (Item<ITEM>().IsEnabled()) {
            Item<ITEM>().Disable(); // This method can fail (you can't disable focused item)
        }
    }
    template <class ITEM>
    void EnableItem() {
        if (!Item<ITEM>().IsEnabled()) {
            Item<ITEM>().Enable();
        }
    }

    // cannot hide focused item
    template <class ITEM>
    bool Hide() {
        return menu.Hide(Item<ITEM>());
    }

    template <class ITEM>
    void Show() {
        menu.Show(Item<ITEM>());
    }

    template <class ITEM1, class ITEM2>
    bool SwapVisibility() {
        return menu.SwapVisibility(Item<ITEM1>(), Item<ITEM2>());
    }
    // ShowDevOnly intentionally not supported, can be set only in ctor
};

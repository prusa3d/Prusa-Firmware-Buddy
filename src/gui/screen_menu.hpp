#pragma once

#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.hpp"
#include "WinMenuContainer.hpp"
#include "WindowMenuItems.hpp"
#include <window_menu.hpp>
#include <stdint.h>
#include "screen.hpp"
#include <new>

template <typename Menu>
class ScreenMenuBase : public screen_t {

public:
    virtual void InitState(screen_init_variant var) override {
        if (auto pos = var.GetMenuPosition()) {
            menu.menu.restore_state(*pos);
        }
    }
    virtual screen_init_variant GetCurrentState() const override {
        screen_init_variant ret;
        ret.SetMenuPosition(menu.menu.get_restore_state());
        return ret;
    }

protected:
    ScreenMenuBase(window_t *parent, const string_view_utf8 &label, EFooter show_footer)
        : screen_t(parent, parent != nullptr ? win_type_t::dialog : win_type_t::normal)
        , header(this)
        , menu(this, show_footer == EFooter::On ? GuiDefaults::RectScreenBody : GuiDefaults::RectScreenNoHeader)
        , footer(this) //
    {
        header.SetText(label);
        footer.set_visible(show_footer == EFooter::On);
        CaptureNormalWindow(menu); // set capture to list
    }

protected:
    window_header_t header;
    WindowExtendedMenu<Menu> menu;
    StatusFooter footer;
};

template <EFooter FOOTER, class... T>
class ScreenMenu : public ScreenMenuBase<WindowMenu> {
protected:
    // std::array<window_t*,sizeof...(T)> pElements;//todo menu item is not a window
    WinMenuContainer<T...> container;

public:
    ScreenMenu(const string_view_utf8 &label, window_t *parent = nullptr)
        : ScreenMenuBase(parent, label, FOOTER) {
        menu.menu.BindContainer(container);
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

    template <class ITEM1, class ITEM2>
    bool SwapVisibility() {
        return menu.menu.SwapVisibility(Item<ITEM1>(), Item<ITEM2>());
    }

    // ShowDevOnly intentionally not supported, can be set only in ctor
};

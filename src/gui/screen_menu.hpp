#pragma once

#include <window_header.hpp>
#include <status_footer.hpp>
#include <WinMenuContainer.hpp>
#include <window_menu.hpp>
#include <screen.hpp>

/// Bits of ScreenMenuBase than can be made non-templated to save flash
class ScreenMenuBase_ : public screen_t {

public:
    virtual void InitState(screen_init_variant var) override {
        if (auto pos = var.GetMenuPosition()) {
            i_menu->restore_state(*pos);
        }
    }

    virtual screen_init_variant GetCurrentState() const override {
        screen_init_variant ret;
        ret.SetMenuPosition(i_menu->get_restore_state());
        return ret;
    }

protected:
    ScreenMenuBase_(window_t *parent, const string_view_utf8 &label, EFooter show_footer, IWindowMenu *menu)
        : screen_t(parent, parent != nullptr ? win_type_t::dialog : win_type_t::normal)
        , header(this)
        , footer(this)
        , i_menu(menu) //
    {
        // Do NOT work with i_menu here, it is not yet constructed!
        header.SetText(label);
        footer.set_visible(show_footer == EFooter::On);
    }

protected:
    window_header_t header;
    StatusFooter footer;

private:
    IWindowMenu *i_menu = nullptr;
};

template <typename Menu>
class ScreenMenuBase : public ScreenMenuBase_ {

protected:
    ScreenMenuBase(window_t *parent, const string_view_utf8 &label, EFooter show_footer)
        : ScreenMenuBase_(parent, label, show_footer, &menu.menu)
        , menu(this, show_footer == EFooter::On ? GuiDefaults::RectScreenBody : GuiDefaults::RectScreenNoHeader) //
    {
        CaptureNormalWindow(menu); // set capture to list
    }

protected:
    WindowExtendedMenu<Menu> menu;
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

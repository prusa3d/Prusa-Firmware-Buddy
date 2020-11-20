#pragma once

#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.h"
#include "window_menu.hpp"
#include "WinMenuContainer.hpp"
#include "WindowMenuItems.hpp"
#include <stdint.h>
#include "resource.h"
#include <new>

enum class EHeader { On,
    Off }; //affect only events
enum class EFooter { On,
    Off };

struct HelperConfig {
    uint16_t lines;
    uint16_t font_id;
};

constexpr static const HelperConfig HelpLines_None = { 0, IDR_FNT_SPECIAL };
constexpr static const HelperConfig HelpLines_Default = { 4, IDR_FNT_SPECIAL };

//parent to not repeat code in templates
class IScreenMenu : public AddSuperWindow<window_frame_t> {
protected:
    constexpr static const char *no_labelS = "MISSING";
    static string_view_utf8 no_label;
    window_header_t header;
    window_menu_t menu;
    window_text_t help;
    status_footer_t footer;

    window_t *prev_capture;

public:
    IScreenMenu(window_t *parent, string_view_utf8 label, Rect16 menu_item_rect, EFooter FOOTER, size_t helper_lines, uint32_t font_id);
    ~IScreenMenu();
    void unconditionalDrawItem(uint8_t index);
};

template <EHeader HEADER, EFooter FOOTER, const HelperConfig &HELP_CNF, class... T>
class ScreenMenu : public AddSuperWindow<IScreenMenu> {
protected:
    //std::array<window_t*,sizeof...(T)> pElements;//todo menu item is not a window
    WinMenuContainer<T...> container;

public:
    ScreenMenu(string_view_utf8 label, window_t *parent = nullptr, Rect16 menu_item_rect = GuiDefaults::RectScreenBody);

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
};

template <EHeader HEADER, EFooter FOOTER, const HelperConfig &HELP_CNF, class... T>
ScreenMenu<HEADER, FOOTER, HELP_CNF, T...>::ScreenMenu(string_view_utf8 label, window_t *parent, Rect16 menu_item_rect)
    : AddSuperWindow<IScreenMenu>(parent, label, menu_item_rect, FOOTER, HELP_CNF.lines, HELP_CNF.font_id) {
    menu.pContainer = &container;
    menu.GetActiveItem()->SetFocus(); //set focus on new item//containder was not valid during construction, have to set its index again
}

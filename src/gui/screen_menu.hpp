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
class IScreenMenu : protected window_menu_t {
protected:
    constexpr static const char *no_label = "MISSING";
    window_frame_t root;
    window_header_t header;
    window_text_t help;
    status_footer_t footer;

public:
    IScreenMenu(const char *label, EFooter FOOTER, size_t helper_lines, uint32_t font_id);
    void Done();
    void Draw() {}
    int Event(window_t *window, uint8_t event, void *param);

    static void CDone(screen_t *screen) {
        reinterpret_cast<IScreenMenu *>(screen->pdata)->Done();
    }

    static void CDraw(screen_t *screen) {
        reinterpret_cast<IScreenMenu *>(screen->pdata)->Draw();
    }
    static int CEvent(screen_t *screen, window_t *window, uint8_t event, void *param) {
        return reinterpret_cast<IScreenMenu *>(screen->pdata)->Event(window, event, param);
    }
};

template <EHeader HEADER, EFooter FOOTER, const HelperConfig &HELP_CNF, class... T>
class ScreenMenu : public IScreenMenu {
protected:
    WinMenuContainer<T...> container;

public:
    ScreenMenu(const char *label);

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

    //C code binding
    static void Create(screen_t *screen, const char *label = no_label) {
        auto *ths = reinterpret_cast<ScreenMenu<HEADER, FOOTER, HELP_CNF, T...> *>(screen->pdata);
        ::new (ths) ScreenMenu<HEADER, FOOTER, HELP_CNF, T...>(label);
    }
};

template <EHeader HEADER, EFooter FOOTER, const HelperConfig &HELP_CNF, class... T>
ScreenMenu<HEADER, FOOTER, HELP_CNF, T...>::ScreenMenu(const char *label)
    : IScreenMenu(label, FOOTER, HELP_CNF.lines, HELP_CNF.font_id) {
    pContainer = &container;
    GetActiveItem()->SetFocus(); //set focus on new item//containder was not valid during construction, have to set its index again
}

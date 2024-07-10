#pragma once

#include "screen_menu.hpp"
#include "MItem_tools.hpp"
#include "MItem_basic_selftest.hpp"
#include <utility_extensions.hpp>
#include <selftest_snake_config.hpp>
#include <printers.h>

namespace SelftestSnake {
static_assert(Action::_first != Action::_last, "Edge case not handled");

class I_MI_STS : public IWindowMenuItem {
public:
    static constexpr size_t max_label_len { 66 }; ///< Buffer for label, needs to fit all languages
    I_MI_STS(Action action);
    void do_click(IWindowMenu &window_menu, Action action);

private:
    char *get_filled_menu_item_label(Action action);
    char label_buffer[max_label_len];
};

template <Action action>
class MI_STS : public I_MI_STS {
public:
    MI_STS()
        : I_MI_STS(action) {}

protected:
    void click(IWindowMenu &window_menu) override {
        do_click(window_menu, action);
    }
};

class I_MI_STS_SUBMENU : public IWindowMenuItem {
public:
    I_MI_STS_SUBMENU(const char *label, Action action, Tool tool);
    void do_click(IWindowMenu &window_menu, Tool tool, Action action);
};

template <Tool tool, Action action>
    requires SubmenuActionC<action>
class MI_STS_SUBMENU : public I_MI_STS_SUBMENU {
public:
    MI_STS_SUBMENU()
        : I_MI_STS_SUBMENU(get_submenu_label(tool, action), action, tool) {}

protected:
    void click(IWindowMenu &window_menu) override {
        do_click(window_menu, tool, action);
    }
};

bool is_menu_draw_enabled(window_t *window);
void do_menu_event(window_t *receiver, window_t *sender, GUI_event_t event, void *param, Action action, bool is_submenu);

namespace detail {
    // Enum to discern whether 'building' a Calibrations or Wizard menu
    enum class MenuType {
        Calibrations,
        Wizard,
    };

    // Primary template, should never be actually instanciated.
    // The specializations build a screen menu with MI_STS items instanciated by all Actions, in the order <_first, _last>.
    template <EFooter FOOTER, MenuType menu_type,
        typename IS = decltype(std::make_index_sequence<ftrstd::to_underlying(Action::_count)>())>
    struct menu_builder;

    // Partial specialization for when building Calibrations menu
    template <EFooter FOOTER, std::size_t... I>
    struct menu_builder<FOOTER, MenuType::Calibrations, std::index_sequence<I...>> {
        using type = ScreenMenu<FOOTER, MI_RETURN,
            MI_STS<static_cast<Action>(I + ftrstd::to_underlying(Action::_first))>...>;
    };

    // Partial specialization for when building Wizard menu
    template <EFooter FOOTER, std::size_t... I>
    struct menu_builder<FOOTER, MenuType::Wizard, std::index_sequence<I...>> {
        using type = ScreenMenu<FOOTER,
            MI_STS<static_cast<Action>(I + ftrstd::to_underlying(Action::_first))>...,
            MI_EXIT>;
    };

    // Helper type so that there's no need to write typename ... ::type
    template <EFooter FOOTER, MenuType menu_type>
    using menu_builder_t = typename menu_builder<FOOTER, menu_type>::type;

    using ScreenMenuSTSCalibrations = menu_builder_t<GuiDefaults::MenuFooter, MenuType::Calibrations>;
    using ScreenMenuSTSWizard = menu_builder_t<GuiDefaults::MenuFooter, MenuType::Wizard>;
} // namespace detail
} // namespace SelftestSnake

class ScreenMenuSTSCalibrations : public SelftestSnake::detail::ScreenMenuSTSCalibrations {
public:
    static constexpr const char *label { N_("CALIBRATIONS & TESTS") };
    ScreenMenuSTSCalibrations();

    virtual void draw() override;
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};

class ScreenMenuSTSWizard : public SelftestSnake::detail::ScreenMenuSTSWizard {
public:
    static constexpr const char *label { N_("Wizard") };
    ScreenMenuSTSWizard();

    virtual void draw() override;
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    bool draw_enabled { false };
};

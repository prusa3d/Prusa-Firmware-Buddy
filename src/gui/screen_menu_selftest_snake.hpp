#pragma once

#include "screen_menu.hpp"
#include <WindowMenuItems.hpp>
#include <utility_extensions.hpp>
#include <selftest_snake_config.hpp>
#include <meta_utils.hpp>

namespace SelftestSnake {

class I_MI_STS : public IWindowMenuItem {
public:
    static constexpr size_t max_label_len { 66 }; ///< Buffer for label, needs to fit all languages
    I_MI_STS(Action action);
    void click(IWindowMenu &) override;
    void Loop() override;

private:
    const Action action;

    string_view_utf8 get_filled_menu_item_label(Action action);
    char label_buffer[max_label_len];
};

template <Action action_>
using MI_STS = WithConstructorArgs<I_MI_STS, action_>;

class I_MI_STS_SUBMENU : public IWindowMenuItem {
public:
    I_MI_STS_SUBMENU(const char *label_template, Action action, PhysicalToolIndex tool);
    void click(IWindowMenu &window_menu) override;
    void Loop() override;

private:
    const Action action;
    const PhysicalToolIndex tool;
    StringViewUtf8Parameters<3> label_params_;
};

class MI_BYPASS_DEPENDENCIES : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *label = N_("Bypass Dependency Checks");

public:
    MI_BYPASS_DEPENDENCIES();

protected:
    void OnChange(size_t old_index) override;
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
        typename IS = decltype(std::make_index_sequence<std::to_underlying(Action::_count)>())>
    struct menu_builder;

    // Partial specialization for when building Calibrations menu
    template <EFooter FOOTER, std::size_t... I>
    struct menu_builder<FOOTER, MenuType::Calibrations, std::index_sequence<I...>> {
        using type = ScreenMenu<FOOTER, MI_RETURN,
            MI_STS<static_cast<Action>(I + std::to_underlying(Action::_first))>...,
            MI_BYPASS_DEPENDENCIES>;
    };

    // Partial specialization for when building Wizard menu
    template <EFooter FOOTER, std::size_t... I>
    struct menu_builder<FOOTER, MenuType::Wizard, std::index_sequence<I...>> {
        using type = ScreenMenu<FOOTER,
            MI_STS<static_cast<Action>(I + std::to_underlying(Action::_first))>...,
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

    [[nodiscard]] static bool should_show();

    virtual void draw() override;
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    bool draw_enabled { false };
};

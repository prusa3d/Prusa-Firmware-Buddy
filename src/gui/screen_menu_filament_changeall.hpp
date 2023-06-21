#pragma once

#include "screen_menu.hpp"
#include "MItem_menus.hpp"
#include "i18n.h"
#include <WindowItemFormatableSpin.hpp>

/**
 * @brief Selector of filament types for one tool.
 * Switch has N+2 items for "Unload" and "Don't change".
 */
class I_MI_FilamentSelect : public WI_LAMBDA_SPIN {
    static constexpr const char *const label_unload = N_("Unload");
    static constexpr const char *const label_nochange = N_("Don't change");
    static constexpr const char *const label_change_fil = N_("Change to"); // Concatenated with filament name, "Change to" + " " + "PLA"
    static constexpr const char *const label_load_fil = N_("Load");        // Concatenated with filament name, "Load" + " " + "PLA"

    const bool loaded;                                                     ///< True if filament is loaded in this tool, difference between "Change to" and "Load"

public:
    static constexpr const size_t unload_index = ftrstd::to_underlying(filament::Type::_last) + 1; ///< Index of "Unload" item
    static constexpr const size_t nochange_index = ftrstd::to_underlying(filament::Type::NONE);    ///< Index of "Don't change" item

    // "No change" needs to cover filament::Type::NONE so proper filament types line up with indices
    static_assert(nochange_index == ftrstd::to_underlying(filament::Type::NONE));

    /**
     * @brief Construct spin filament selector.
     * @param label label for this particular tool
     * @param tool_n index of this particular tool [indexed from 0]
     */
    I_MI_FilamentSelect(const char *const label, int tool_n);
};

/**
 * @brief Selector of filament types for one tool.
 * @param N selector for this tool
 */
template <int N>
class MI_FilamentSelect : public I_MI_FilamentSelect {
    static_assert(N >= 0 && N <= 4, "bad input");
    static consteval const char *get_name() {
        switch (N) {
        case 0:
            return N_("Tool 1 Filament");
        case 1:
            return N_("Tool 2 Filament");
        case 2:
            return N_("Tool 3 Filament");
        case 3:
            return N_("Tool 4 Filament");
        case 4:
            return N_("Tool 5 Filament");
        }
        consteval_assert_false();
        return "";
    }

    static constexpr const char *label = get_name();

public:
    MI_FilamentSelect()
        : I_MI_FilamentSelect(label, N) {}
};

/**
 * @brief Do the changes.
 */
class MI_FilamentApplyChanges : public WI_LABEL_t {
    static constexpr const char *const label = N_("Carry Out the Changes");

public:
    MI_FilamentApplyChanges();

protected:
    virtual void click(IWindowMenu & /*window_menu*/) override;
};

using ScreenChangeAllFilaments__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
    MI_FilamentSelect<0>,
    MI_FilamentSelect<1>,
    MI_FilamentSelect<2>,
    MI_FilamentSelect<3>,
    MI_FilamentSelect<4>,
    MI_FilamentApplyChanges>;

/**
 * @brief Change filament in all tools.
 */
class ScreenChangeAllFilaments : public ScreenChangeAllFilaments__ {
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

public:
    static constexpr size_t tool_count = 5; ///< Number of tools shown in this menu
    constexpr static const char *label = N_("MULTITOOL FILAMENT CHANGE");
    ScreenChangeAllFilaments();
};

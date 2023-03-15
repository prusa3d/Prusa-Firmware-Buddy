#pragma once

#include "screen_menu.hpp"
#include "MItem_menus.hpp"
#include "i18n.h"

/**
 * @brief Selector of filament types for one tool.
 * Switch has N+2 items for unload ("---") and "Don't change".
 */
class IMiFilamentSelect : public WI_SWITCH_t<size_t(filament::Type::_last) + 2> {
    static constexpr const char *const label_unload = N_("Unload");
    static constexpr const char *const label_nochange = N_("Don't change");

public:
    IMiFilamentSelect(const char *const label, int index);
};

/**
 * @brief Selector of filament types for one tool.
 * @param N selector for this tool
 */
template <int N>
class MiFilamentSelect : public IMiFilamentSelect {
    static_assert(N >= 0 && N <= 4, "bad input");
    static constexpr const char *const get_name() {
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
    }

    static constexpr const char *const label = get_name();

public:
    MiFilamentSelect()
        : IMiFilamentSelect(label, N) {}

    virtual void OnChange(size_t old_index) override {
        if (index > size_t(filament::Type::_last))
            return; // should not happen
    }
};

/**
 * @brief Do the changes.
 */
class MiFilamentApplyChanges : public WI_LABEL_t {
    static constexpr const char *const label = N_("Carry Out the Changes");

public:
    MiFilamentApplyChanges();

protected:
    virtual void click(IWindowMenu & /*window_menu*/) override;
};

using ScreenChangeAllFilaments__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
    MiFilamentSelect<0>,
    MiFilamentSelect<1>,
    MiFilamentSelect<2>,
    MiFilamentSelect<3>,
    MiFilamentSelect<4>,
    MiFilamentApplyChanges>;

/**
 * @brief Change filament in all tools.
 */
class ScreenChangeAllFilaments : public ScreenChangeAllFilaments__ {
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

public:
    constexpr static const char *label = N_("PICK FILAMENTS TO LOAD");
    ScreenChangeAllFilaments();
};

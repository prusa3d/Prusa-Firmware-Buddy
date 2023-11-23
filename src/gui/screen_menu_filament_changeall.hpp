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
    static constexpr const char *label_unload = N_("Unload");
    static constexpr const char *label_nochange = N_("Don't change");
    static constexpr const char *label_change_fil = N_("Change to"); // Concatenated with filament name, "Change to" + " " + "PLA"
    static constexpr const char *label_load_fil = N_("Load"); // Concatenated with filament name, "Load" + " " + "PLA"

    const bool loaded; ///< True if filament is loaded in this tool, difference between "Change to" and "Load"

public:
    static constexpr size_t max_I_MI_FilamentSelect_idx { 4 };
    static constexpr const size_t unload_index = ftrstd::to_underlying(filament::Type::_last) + 1; ///< Index of "Unload" item
    static constexpr const size_t nochange_index = ftrstd::to_underlying(filament::Type::NONE); ///< Index of "Don't change" item

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
public:
    MI_FilamentSelect()
        : I_MI_FilamentSelect(label, N) {}

    static constexpr size_t filament_select_idx { N };

private:
    static_assert(N >= 0 && N <= max_I_MI_FilamentSelect_idx, "bad input");
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
};

/**
 * @brief Do the changes.
 */
class MI_FilamentApplyChanges : public WI_LABEL_t {
public:
    static constexpr const char *label = N_("Carry Out the Changes");
    MI_FilamentApplyChanges();

protected:
    virtual void click(IWindowMenu & /*window_menu*/) override;
};

namespace detail {
using ScreenChangeAllFilaments = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
    MI_FilamentSelect<0>,
    MI_FilamentSelect<1>,
    MI_FilamentSelect<2>,
    MI_FilamentSelect<3>,
    MI_FilamentSelect<4>,
    MI_FilamentApplyChanges>;
} // namespace detail

/**
 * @brief Change filament in all tools.
 */
class ScreenChangeAllFilaments : public detail::ScreenChangeAllFilaments {
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

public:
    static constexpr size_t tool_count = 5; ///< Number of tools shown in this menu
    static constexpr const char *label = N_("MULTITOOL FILAMENT CHANGE");
    ScreenChangeAllFilaments();
};

// Holds code related to converting the screen to be used as a dialog
namespace dialog_change_all_filaments {
namespace detail {
    class DialogEnabledMI {
    public:
        DialogEnabledMI();
        void set_parent(IDialog *new_parent);

    protected:
        IDialog *parent;
    };
} // namespace detail

/**
 * @brief Dialog enabled MI_RETURN
 *
 */
class DMI_RETURN : public detail::DialogEnabledMI, public WI_LABEL_t {
public:
    static constexpr const char *label = MI_RETURN::label;
    DMI_RETURN();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class DMI_FilamentApplyChanges : public detail::DialogEnabledMI, public WI_LABEL_t {
public:
    static constexpr const char *label = MI_FilamentApplyChanges::label;
    DMI_FilamentApplyChanges();

protected:
    virtual void click(IWindowMenu & /*window_menu*/) override;
};

namespace detail {
    using DialogChangeAllFilaments = WinMenuContainer<DMI_RETURN,
        MI_FilamentSelect<0>,
        MI_FilamentSelect<1>,
        MI_FilamentSelect<2>,
        MI_FilamentSelect<3>,
        MI_FilamentSelect<4>,
        DMI_FilamentApplyChanges>;
} // namespace detail

class DialogChangeAllFilaments final : public AddSuperWindow<IDialog> {
public:
    static constexpr const char *label = ScreenChangeAllFilaments::label;

    /**
     * @brief Construct dialog for changing filament in all tools.
     * @param default_selections default selections for each tool
     * @param exit_on_media_ if true, exit on media removed or error
     */
    DialogChangeAllFilaments(std::array<size_t, I_MI_FilamentSelect::max_I_MI_FilamentSelect_idx + 1> default_selections, bool exit_on_media_, std::array<std::optional<filament::Colour>, ScreenChangeAllFilaments::tool_count> colors_);

    static constexpr size_t tool_count = ScreenChangeAllFilaments::tool_count; ///< Number of tools shown in this menu

    /// True if the dialog was exited by media removed or error
    inline bool was_exited_by_media() const { return exited_by_media; }

private:
    std::atomic<bool> exit_on_media_blocked = false; ///< True when changing filaments and closing this dialog is blocked
    const bool exit_on_media; ///< If true, exit on media removed or error
    bool exited_by_media = false; ///< True when exited by USB removal or error

    detail::DialogChangeAllFilaments container;

    std::array<std::optional<filament::Colour>, ScreenChangeAllFilaments::tool_count> colors; // colors to be displayed as 'we want to load this' in load action

    window_menu_t menu;
    window_header_t header;

    void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
} // namespace dialog_change_all_filaments

/**
 * @brief Show change filament in all tools as a dialog.
 * @param default_selections default selections for each tool
 * @param exit_on_media if true, exit on media removed or error
 * @return true if exited by USB removal or error
 */
bool ChangeAllFilamentsBox(std::array<size_t, I_MI_FilamentSelect::max_I_MI_FilamentSelect_idx + 1> default_selections = {}, bool exit_on_media = false, std::array<std::optional<filament::Colour>, ScreenChangeAllFilaments::tool_count> colors = {});

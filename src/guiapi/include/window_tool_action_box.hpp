#pragma once

#include "MItem_tools.hpp"
#include "IDialogMarlin.hpp"
#include "screen_menu.hpp"
#include <utility_extensions.hpp>
#include "ScreenHandler.hpp"
#include <module/prusa/toolchanger.h>
#include <WindowMenuItems.hpp>
#include <module/planner.h>
#include <img_resources.hpp>
#include <window_dlg_wait.hpp>

namespace ToolBox {
/**
 * Values reflect inner marlin values, name reflects user values
 */
enum class Tool {
    Tool1 = 0,
    Tool2 = 1,
    Tool3 = 2,
    Tool4 = 3,
    Tool5 = 4,
    None_Any = PrusaToolChanger::MARLIN_NO_TOOL_PICKED, // Park value
};

enum class Action {
    PickInactive,
    PickCurrent,
    CalibrateDock,
    Park,
    Return,
    Select,
};

enum class DialogResult {
    Unknown,
    Tool1,
    Tool2,
    Tool3,
    Tool4,
    Tool5,
    Park,
    Return,
};

constexpr DialogResult to_dialog_result(Tool tool, Action action) {
    switch (action) {
    case Action::Park:
        return DialogResult::Park;
    case Action::Return:
        return DialogResult::Return;
    default:
        switch (tool) {
        case Tool::Tool1:
            return DialogResult::Tool1;
        case Tool::Tool2:
            return DialogResult::Tool2;
        case Tool::Tool3:
            return DialogResult::Tool3;
        case Tool::Tool4:
            return DialogResult::Tool4;
        case Tool::Tool5:
            return DialogResult::Tool5;
        default:
            assert(false && "Only real tools at this point are allowed");
            return DialogResult::Return;
        }
    }
}

consteval auto get_label(Tool tool, Action action) -> const char * {
    struct ToolText {
        Tool tool;
        Action action;
        const char *label;
    };
    const ToolText tooltexts[] {
        { Tool::Tool1, Action::PickInactive, N_("Pick Tool 1") },
        { Tool::Tool3, Action::PickInactive, N_("Pick Tool 3") },
        { Tool::Tool2, Action::PickInactive, N_("Pick Tool 2") },
        { Tool::Tool4, Action::PickInactive, N_("Pick Tool 4") },
        { Tool::Tool5, Action::PickInactive, N_("Pick Tool 5") },
        { Tool::Tool1, Action::Select, N_("Pick Tool 1") },
        { Tool::Tool3, Action::Select, N_("Pick Tool 3") },
        { Tool::Tool2, Action::Select, N_("Pick Tool 2") },
        { Tool::Tool4, Action::Select, N_("Pick Tool 4") },
        { Tool::Tool5, Action::Select, N_("Pick Tool 5") },
        { Tool::Tool1, Action::PickCurrent, N_("Pick Tool 1 (Current)") },
        { Tool::Tool3, Action::PickCurrent, N_("Pick Tool 3 (Current)") },
        { Tool::Tool2, Action::PickCurrent, N_("Pick Tool 2 (Current)") },
        { Tool::Tool4, Action::PickCurrent, N_("Pick Tool 4 (Current)") },
        { Tool::Tool5, Action::PickCurrent, N_("Pick Tool 5 (Current)") },
        { Tool::Tool1, Action::CalibrateDock, N_("Calibrate Dock 1") },
        { Tool::Tool2, Action::CalibrateDock, N_("Calibrate Dock 2") },
        { Tool::Tool3, Action::CalibrateDock, N_("Calibrate Dock 3") },
        { Tool::Tool4, Action::CalibrateDock, N_("Calibrate Dock 4") },
        { Tool::Tool5, Action::CalibrateDock, N_("Calibrate Dock 5") },
        { Tool::None_Any, Action::Park, N_("Park Current Tool") },
        { Tool::None_Any, Action::Return, N_("Return") },
    };

    if (auto it = std::ranges::find_if(tooltexts, [&](const auto &elem) {
            return elem.tool == tool && elem.action == action;
        });
        it != std::end(tooltexts)) {
        return it->label;
    } else {
        consteval_assert_false("Unable to find a label for this combination");
        return "";
    }
}

class I_MI_TOOL : public IWindowMenuItem {
public:
    I_MI_TOOL(const char *label, Tool tool, Action action, bool hidden_if_inactive);

    void set_parent(IDialog *parent_);

protected:
    void do_click(IWindowMenu &window_menu, Tool tool, Action action, bool close_on_click);

    IDialog *parent { nullptr };
};

enum class is_hidden_if_inactive_t {
    yes,
    no
};

template <Tool tool, Action action, is_closed_on_click_t close_on_click, is_hidden_if_inactive_t hidden_if_inactive>
class MI_TOOL : public I_MI_TOOL {
public:
    static constexpr ToolBox::Tool TOOL = tool;

    MI_TOOL()
        : I_MI_TOOL(get_label(tool, action), tool, action, hidden_if_inactive == is_hidden_if_inactive_t::yes) {
        has_return_behavior_ = (action == Action::Return);
    }

protected:
    void click(IWindowMenu &window_menu) override {
        do_click(window_menu, tool, action, close_on_click == is_closed_on_click_t::yes);
    }
};

using MI_Return = MI_TOOL<Tool::None_Any, Action::Return, is_closed_on_click_t::yes, is_hidden_if_inactive_t::no>;

template <Tool tool, Action action>
using MI_PickPark = MI_TOOL<tool, action, is_closed_on_click_t::no, is_hidden_if_inactive_t::no>;

template <Tool tool>
using MI_PickPark_ToolBox = MI_PickPark<tool, Action::PickInactive>;
using MI_PickPark_ParkCurrent = MI_PickPark<Tool::None_Any, Action::Park>;

using MenuPickPark = WinMenuContainer<MI_Return,
    MI_PickPark_ParkCurrent,
    MI_PickPark_ToolBox<Tool::Tool1>,
    MI_PickPark_ToolBox<Tool::Tool2>,
    MI_PickPark_ToolBox<Tool::Tool3>,
    MI_PickPark_ToolBox<Tool::Tool4>,
    MI_PickPark_ToolBox<Tool::Tool5>>;

template <Tool tool>
using MI_Select = MI_TOOL<tool, Action::Select, is_closed_on_click_t::yes, is_hidden_if_inactive_t::no>;

using MenuSelect = WinMenuContainer<MI_Return,
    MI_Select<Tool::Tool1>,
    MI_Select<Tool::Tool2>,
    MI_Select<Tool::Tool3>,
    MI_Select<Tool::Tool4>,
    MI_Select<Tool::Tool5>>;

template <Tool tool>
using MI_PickAndGo_Current = MI_TOOL<tool, Action::PickCurrent, is_closed_on_click_t::yes, is_hidden_if_inactive_t::yes>;

template <Tool tool>
using MI_PickAndGo_Inactive = MI_TOOL<tool, Action::PickInactive, is_closed_on_click_t::yes, is_hidden_if_inactive_t::yes>;

using MenuPickAndGo = WinMenuContainer<MI_Return,
    MI_PickAndGo_Current<Tool::Tool1>,
    MI_PickAndGo_Inactive<Tool::Tool1>,
    MI_PickAndGo_Current<Tool::Tool2>,
    MI_PickAndGo_Inactive<Tool::Tool2>,
    MI_PickAndGo_Current<Tool::Tool3>,
    MI_PickAndGo_Inactive<Tool::Tool3>,
    MI_PickAndGo_Current<Tool::Tool4>,
    MI_PickAndGo_Inactive<Tool::Tool4>,
    MI_PickAndGo_Current<Tool::Tool5>,
    MI_PickAndGo_Inactive<Tool::Tool5>>;

template <Tool tool>
using MI_CalibrateDock = MI_TOOL<tool, Action::CalibrateDock, is_closed_on_click_t::yes, is_hidden_if_inactive_t::yes>;

using MenuCalibrateDock = WinMenuContainer<MI_Return,
    MI_CalibrateDock<Tool::Tool1>,
    MI_CalibrateDock<Tool::Tool2>,
    MI_CalibrateDock<Tool::Tool3>,
    MI_CalibrateDock<Tool::Tool4>,
    MI_CalibrateDock<Tool::Tool5>>;

template <typename ActionMenuT>
concept ActionMenuC = is_any_of<ActionMenuT, MenuPickPark, MenuSelect, MenuPickAndGo, MenuCalibrateDock>;

template <typename ActionMenuT>
concept ActionMenuHasFooterC = ActionMenuC<ActionMenuT> && is_any_of<ActionMenuT, MenuPickPark>;

template <ActionMenuC ActionMenuT>
consteval Rect16 get_rect_screen_body() {
    if (ActionMenuHasFooterC<ActionMenuT>) {
        return GuiDefaults::RectScreenBody;
    } else {
        return GuiDefaults::RectScreenNoHeader;
    }
}

template <ActionMenuC ToolMenuT>
class DialogToolActionBox : public IDialog {
    ToolMenuT tool_menu;

    window_menu_t menu;
    window_header_t header;

    static constexpr const char *_label { "Pick A Tool" };

    DialogResult result { DialogResult::Unknown };

public:
    DialogToolActionBox()
        : IDialog(get_rect_screen_body<ToolMenuT>())
        , menu(this, get_rect_screen_body<ToolMenuT>(), &tool_menu)
        , header(this) {

        header.SetText(_(_label));
        CaptureNormalWindow(menu);

        // assign this to all menu items  as a parent, so that we can be notified about results
        std::apply([&](auto &...menu_items) {
            (..., [&](auto &menu_item) {
                // this just loops over all menu items, passing each item to this function
                static_assert(
                    requires { menu_item.TOOL; }, "Dialog can contain only toolbox menu items");

                menu_item.set_parent(this);
            }(menu_items));
        },
            tool_menu.menu_items);
    }

    using IsToolEnabledFP = bool (*)(uint8_t tool_nr);

    void DisableNotAvailable(IsToolEnabledFP is_tool_enabled) {
        if (!is_tool_enabled) {
            assert(false && "This function needs a valid function pointer");
            return;
        }

        std::apply([is_tool_enabled](auto &...menu_items) {
            (..., [is_tool_enabled](auto &menu_item) {
                // this just loops over all menu items, passing each item to this function

                // Only disable items that are related to specific tools (exclude return, etc)
                if constexpr (requires { menu_item.TOOL; } && menu_item.TOOL != Tool::None_Any) {
                    if (!is_tool_enabled(ftrstd::to_underlying(menu_item.TOOL))) {
                        // if is_tool_enabled returns false for this particular tool, hide this menu item
                        menu_item.set_enabled(false);
                    }
                }
            }(menu_items));
        },
            tool_menu.menu_items);
    }

    void Preselect(size_t index) {
        // This index works only on enabled menu items, so index actually corresponds to tool number
        menu.menu.move_focus_to_index(index);
    }

    void windowEvent(window_t *sender, GUI_event_t event, void *param) override {
        switch (event) {
        case GUI_event_t::CHILD_CLICK: {
            event_conversion_union un { .pvoid = param };
            result = static_cast<DialogResult>(un.i_val);
            if (flags.close_on_click == is_closed_on_click_t::yes) {
                Screens::Access()->Close();
            } else if (GetParent()) {
                GetParent()->WindowEvent(this, GUI_event_t::CHILD_CLICK, param);
            }
            break;
        }
        default:
            IDialog::windowEvent(sender, event, param);
        }
    }

    DialogResult get_result() const {
        return result;
    }
};

} // namespace ToolBox

template <ToolBox::ActionMenuC ToolMenuT>
ToolBox::DialogResult ToolActionBox() {
    ToolBox::DialogToolActionBox<ToolMenuT> d;
    Screens::Access()->gui_loop_until_dialog_closed();
    return d.get_result();
}

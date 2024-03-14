#include <window_tool_action_box.hpp>
#include <img_resources.hpp>
#include <marlin_client.hpp>

using namespace ToolBox;

namespace {
void wait_until_done() {
    static constexpr int wait_duration { 10 }; // ms
    static constexpr int show_in_progress_after_cnt { 50 / wait_duration }; // ms / ms

    for (int cnt = 0; queue.has_commands_queued() || planner.processing(); cnt++) {
        osDelay(wait_duration);
        if (cnt > show_in_progress_after_cnt) { // show in progress notification after waiting for a while
            gui_dlg_wait([] {
                if (!(queue.has_commands_queued() || planner.processing())) {
                    Screens::Access()->Close();
                }
            });
            break;
        }
    }
}

is_hidden_t get_hidden_state(Tool tool, Action action, bool hidden_if_inactive) {
    const auto idx { ftrstd::to_underlying(tool) };
    switch (action) {
    case Action::Park:
    case Action::PickCurrent:
        if (!hidden_if_inactive) {
            return is_hidden_t::no;
        }
        if (tool == Tool::None_Any) {
            return prusa_toolchanger.is_any_tool_active() ? is_hidden_t::no : is_hidden_t::yes;
        } else {
            return prusa_toolchanger.is_tool_active(idx) ? is_hidden_t::no : is_hidden_t::yes;
        }
    case Action::CalibrateDock:
        return prusa_toolchanger.is_tool_enabled(idx) ? is_hidden_t::no : is_hidden_t::yes;
    case Action::PickInactive:
        if (!prusa_toolchanger.is_tool_enabled(idx)) {
            return is_hidden_t::yes;
        }
        if (!hidden_if_inactive) {
            return is_hidden_t::no;
        }
        return prusa_toolchanger.is_tool_active(idx) ? is_hidden_t::yes : is_hidden_t::no;
    case Action::Return:
        return is_hidden_t::no;
    }
    assert(false && "Unknown configuration");
    return is_hidden_t::yes;
}

const img::Resource *get_icon(Action action) {
    switch (action) {
    case Action::Return:
        return &img::folder_up_16x16;
    default:
        return nullptr;
    }
}

} // unnamed namespace

ToolBox::I_MI_TOOL::I_MI_TOOL(const char *label, Tool tool, Action action, bool hidden_if_inactive)
    : WI_LABEL_t(_(label), get_icon(action), is_enabled_t::yes, get_hidden_state(tool, action, hidden_if_inactive)) {}

void ToolBox::I_MI_TOOL::set_parent(IDialog *parent_) {
    parent = parent_;
}

void ToolBox::I_MI_TOOL::do_click(IWindowMenu &window_menu, Tool tool, Action action, bool close_on_click) {
    switch (action) {
    case Action::PickInactive:
        marlin_client::gcode("G27 P0 Z5"); // Lift Z if not high enough
        marlin_client::gcode_printf("T%d S1 L0 D0", ftrstd::to_underlying(tool));
        wait_until_done();
        break;
    case Action::Park:
        marlin_client::gcode("G27 P0 Z5"); // Lift Z if not high enough
        marlin_client::gcode_printf("T%d S1 L0 D0", PrusaToolChanger::MARLIN_NO_TOOL_PICKED);
        wait_until_done();
        break;
    case Action::CalibrateDock:
#if HAS_SELFTEST()
        marlin_client::test_start_for_tools(stmDocks, 1 << ftrstd::to_underlying(tool));
#endif
        break;
    case Action::PickCurrent: // do nothing (Pick what is already picked)
    case Action::Return: // do nothing (Just close)
        break;
    }

    if (close_on_click) {
        window_menu.Hide(); // Don't redraw the toolbox menu (Hide() works, Validate() doesn't)
        Screens::Access()->Close();
        if (parent) {
            event_conversion_union un { .i_val = ftrstd::to_underlying(to_dialog_result(tool, action)) };
            parent->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, un.pvoid);
        }
    }
}

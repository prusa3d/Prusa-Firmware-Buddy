#include "screen_menu_selftest_snake.hpp"
#include <selftest_snake_submenus.hpp>
#include <img_resources.hpp>
#include <marlin_client.hpp>
#include <ScreenHandler.hpp>
#include <selftest_types.hpp>
#include <RAII.hpp>
#include <option/has_toolchanger.h>
#include "queue.h"
#include "Marlin/src/gcode/queue.h"
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif
#include "selftest/i_selftest.hpp"

using namespace SelftestSnake;

namespace {

inline bool is_multitool() {
#if HAS_TOOLCHANGER()
    return prusa_toolchanger.is_toolchanger_enabled();
#else
    return false;
#endif
}

Action _get_valid_action(Action start_action, int step) {
    assert(step == 1 || step == -1); // other values would cause weird behaviour (endless loop / go beyond array)
    if (is_multitool()) {
        while (is_singletool_only_action(start_action)) {
            start_action = static_cast<Action>(ftrstd::to_underlying(start_action) + step);
        }
    } else { // singletool
        while (is_multitool_only_action(start_action)) {
            start_action = static_cast<Action>(ftrstd::to_underlying(start_action) + step);
        }
    }
    return start_action;
}

Action get_first_action() {
    return _get_valid_action(Action::_first, 1);
}

Action get_last_action() {
    return _get_valid_action(Action::_last, -1);
}

// Can't (shouldn't) be called with last action
Action get_next_action(Action action) {
    assert(get_last_action() != action && "Unhandled edge case");
    return _get_valid_action(static_cast<Action>(ftrstd::to_underlying(action) + 1), 1);
}

// Can't (shouldn't) be called with first action
Action get_previous_action(Action action) {
    assert(get_first_action() != action && "Unhandled edge case");
    return _get_valid_action(static_cast<Action>(ftrstd::to_underlying(action) - 1), -1);
}

bool are_previous_completed(Action action) {
    if (action == get_first_action()) {
        return true;
    }

    for (Action act = action; act > get_first_action(); act = get_previous_action(act)) {
        if (get_test_result(get_previous_action(act), Tool::_all_tools) != TestResult_Passed) {
            return false;
        }
    }

    return true;
}

const img::Resource *get_icon(Action action, Tool tool) {
    switch (get_test_result(action, tool)) {
    case TestResult_Passed:
        return &img::ok_color_16x16;
    case TestResult_Skipped:
    case TestResult_Unknown:
        return &img::na_color_16x16;
    case TestResult_Failed:
        return &img::nok_color_16x16;
    }

    assert(false);
    return &img::error_16x16;
}

struct SnakeConfig {
    enum class State {
        reset,
        first,
        not_first,
    };

    void reset() {
        break_after_submenu = false;
        in_progress = false;
        last_action = get_last_action();
        last_tool = Tool::_first;
        state = State::reset;
    }

    void next(Action action, Tool tool) {
        in_progress = true;
        last_action = action;
        last_tool = tool;
        if (state == State::reset) {
            state = State::first;
        } else if (state == State::first) {
            state = State::not_first;
        }
    }

    bool break_after_submenu { false }; ///< User selected to do one submenu and then stop
    bool in_progress { false };
    Action last_action { Action::_last };
    Tool last_tool { Tool::_first };
    State state { State::reset };
};

} // namespace

static SnakeConfig snake_config {};

namespace {

void do_snake(Action action, Tool tool = Tool::_first) {
    if (!are_previous_completed(action) && !snake_config.in_progress) {
        if (MsgBoxQuestion(_("Previous Calibrations & Tests are not all done. Continue anyway?"), Responses_YesNo, 1) == Response::No) {
            snake_config.reset();
            return;
        }
    }

    // Note: "gcode" tests are handled separately, partly because
    //       there are not enough bits in the selftest mask.
    {
        bool has_test_special_handling = true;

        switch (action) {

#if HAS_PHASE_STEPPING()
        case Action::PhaseSteppingCalibration:
            marlin_client::gcode("M1977");
            break;
#endif

        default:
            has_test_special_handling = false;
            break;
        }

        if (has_test_special_handling) {
            marlin_client::gcode("M118 nop"); // No operation gcode to fill the queue until selftest is done
            snake_config.next(action, tool);
            return;
        }
    }

    if (has_submenu(action)) {
        if (snake_config.state == SnakeConfig::State::reset || tool == Tool::_first) { // Ask only for first tool or if it is selected in submenu
            ask_config(action);
        }
        marlin_client::test_start_with_data(get_test_mask(action), get_tool_mask(tool));
    } else {
        ask_config(action);
        marlin_client::test_start(get_test_mask(action));
    }

    snake_config.next(action, tool);
};

void continue_snake() {
    const TestResult last_test_result = get_test_result(snake_config.last_action, snake_config.last_tool);
    if ((last_test_result != TestResult_Passed && last_test_result != TestResult_Skipped)
        || SelftestInstance().IsAborted()) { // last selftest didn't pass
        snake_config.reset();
        return;
    }

    // if the last action was the last action possible
    if (snake_config.last_action == get_last_action()
        && (!has_submenu(get_last_action()) || snake_config.last_tool == get_last_enabled_tool())) {
        snake_config.reset();
        return;
    }

    if (snake_config.break_after_submenu && has_submenu(snake_config.last_action) && snake_config.last_tool == get_last_enabled_tool()) {
        snake_config.reset();
        return; // Stop when submenu is finished
    }

    if (snake_config.state == SnakeConfig::State::first // ran only one action so far
        && (snake_config.last_action != get_first_action() || get_test_result(get_next_action(get_first_action()), Tool::_all_tools) == TestResult_Passed)) {

        Response resp = Response::Stop;
        if (is_multitool() && has_submenu(snake_config.last_action) && snake_config.last_tool != get_last_enabled_tool()) {
            resp = MsgBoxQuestion(_("FINISH remaining calibrations without proceeding to other tests, or perform ALL Calibrations and Tests?\n\nIf you QUIT, all data up to this point is saved."), { Response::Finish, Response::All, Response::Quit }, 2);
            snake_config.break_after_submenu = (resp == Response::Finish); // Continue running tests but stop at the end of submenu
        } else {
            resp = MsgBoxQuestion(_("Continue running Calibrations & Tests?"), { Response::Continue, Response::Quit }, 1);
        }
        if (resp == Response::Quit) {
            snake_config.reset();
            return; // stop after running the first one
        }
    }

    if (!is_multitool()
        || !has_submenu(snake_config.last_action)
        || snake_config.last_tool == get_last_enabled_tool()) { // singletool or wasn't submenu or was last in a submenu
        do_snake(get_next_action(snake_config.last_action));
    } else { // current submenu not yet finished
        do_snake(snake_config.last_action, get_next_tool(snake_config.last_tool));
    }
}

is_hidden_t get_subitem_hidden_state(Tool tool) {
#if HAS_TOOLCHANGER()
    const auto idx { ftrstd::to_underlying(tool) };
    return prusa_toolchanger.is_tool_enabled(idx) ? is_hidden_t::no : is_hidden_t::yes;
#else
    return tool == Tool::Tool1 ? is_hidden_t::no : is_hidden_t::yes;
#endif
}

is_hidden_t get_mainitem_hidden_state(Action action) {
    if constexpr (!option::has_toolchanger) {
        if (requires_toolchanger(action)) {
            return is_hidden_t::yes;
        }
    }

    if ((is_multitool() && is_singletool_only_action(action))
        || (!is_multitool() && is_multitool_only_action(action))) {
        return is_hidden_t::yes;
    } else {
        return is_hidden_t::no;
    }
}

expands_t get_expands(Action action) {
    if (!is_multitool()) {
        return expands_t::no;
    }
    return has_submenu(action) ? expands_t::yes : expands_t::no;
}

constexpr IWindowMenuItem::ColorScheme not_yet_ready_scheme {
    .text { .focused = GuiDefaults::MenuColorBack, .unfocused = GuiDefaults::MenuColorDisabled },
    .back { .focused = GuiDefaults::MenuColorDisabled, .unfocused = GuiDefaults::MenuColorBack },
    .rop {
        .focused { is_inverted::no, has_swapped_bw::no, is_shadowed::no, is_desaturated::no },
        .unfocused { is_inverted::no, has_swapped_bw::no, is_shadowed::no, is_desaturated::no } }
};

} // namespace

// returns the parameter, filled
string_view_utf8 I_MI_STS::get_filled_menu_item_label(Action action) {
    // holds menu indices, indexed by Action
    static const std::array<size_t, ftrstd::to_underlying(Action::_count)> action_indices {
        []() {
            std::array<size_t, ftrstd::to_underlying(Action::_count)> indices { { {} } };

            int idx { 1 }; // start number
            for (Action act = get_first_action();; act = get_next_action(act)) {
                indices[ftrstd::to_underlying(act)] = idx++;
                if (act == get_last_action()) { // explicitly done this way to avoid getting next action of the last action
                    break;
                }
            }
            return indices;
        }()
    };

    if (auto it = std::ranges::find_if(blank_item_texts, [&](const auto &elem) {
            return elem.action == action;
        });
        it != std::end(blank_item_texts)) {

        char buffer[max_label_len];
        _(it->label).copyToRAM(buffer, max_label_len);
        snprintf(label_buffer, max_label_len, buffer, action_indices[ftrstd::to_underlying(action)]);
    } else {
        assert(false && "Unable to find a label for this combination");
    }

    return string_view_utf8::MakeRAM(label_buffer);
}

I_MI_STS::I_MI_STS(Action action)
    : IWindowMenuItem(get_filled_menu_item_label(action),
        get_icon(action, Tool::_all_tools), is_enabled_t::yes, get_mainitem_hidden_state(action), get_expands(action)) {
    if (is_multitool()) {
        set_icon_position(IconPosition::right);
    } else {
        set_icon_position(IconPosition::replaces_extends);
    }
    if (!are_previous_completed(action)) {
        set_color_scheme(&not_yet_ready_scheme);
    }
}

void I_MI_STS::do_click([[maybe_unused]] IWindowMenu &window_menu, Action action) {
    if (!has_submenu(action) || !is_multitool()) {
        do_snake(action);
    } else {
        open_submenu(action);
    }
}

I_MI_STS_SUBMENU::I_MI_STS_SUBMENU(const char *label, Action action, Tool tool)
    : IWindowMenuItem(_(label), get_icon(action, tool), is_enabled_t::yes, get_subitem_hidden_state(tool)) {
    set_icon_position(IconPosition::right);
}

void I_MI_STS_SUBMENU::do_click([[maybe_unused]] IWindowMenu &window_menu, Tool tool, Action action) {
    do_snake(action, tool);
}

namespace SelftestSnake {
void do_menu_event(window_t *receiver, [[maybe_unused]] window_t *sender, GUI_event_t event, [[maybe_unused]] void *param, Action action, bool is_submenu) {
    if (receiver->GetFirstDialog() || event != GUI_event_t::LOOP || !snake_config.in_progress || SelftestInstance().IsInProgress() || queue.has_commands_queued()) {
        // G-code selftests may take a few ticks to execute, do not continue snake while gcode is still in the queue or in progress (no operation gcode is enqueued behind it)
        return;
    }

    // snake is in progress and previous selftest is done
    continue_snake();

    if (!snake_config.in_progress) { // force redraw of current snake menu
        Screens::Access()->Get()->Invalidate();
    }

    if (is_submenu) {
        if (snake_config.last_action == action && snake_config.last_tool == get_last_enabled_tool()) { // finished testing this submenu
            Screens::Access()->Close();
        }
    }
}

bool is_menu_draw_enabled(window_t *window) {
    return !snake_config.in_progress // don't draw if snake is ongoing
        || window->GetFirstDialog(); // always draw if msgbox is being shown
}
} // namespace SelftestSnake

ScreenMenuSTSCalibrations::ScreenMenuSTSCalibrations()
    : SelftestSnake::detail::ScreenMenuSTSCalibrations(_(label)) {
    ClrMenuTimeoutClose(); // No timeout for snake
}

void ScreenMenuSTSCalibrations::draw() {
    if (SelftestSnake::is_menu_draw_enabled(this)) {
        window_frame_t::draw();
    }
}

void ScreenMenuSTSCalibrations::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    do_menu_event(this, sender, event, param, get_first_action(), false);
}

ScreenMenuSTSWizard::ScreenMenuSTSWizard()
    : SelftestSnake::detail::ScreenMenuSTSWizard(_(label)) {
    header.SetIcon(&img::wizard_16x16);
    ClrMenuTimeoutClose(); // No timeout for wizard's snake
}

void ScreenMenuSTSWizard::draw() {
    if ((draw_enabled && !snake_config.in_progress) // don't draw if starting/ending or snake in progress
        || GetFirstDialog() // Always draw when there is a dialog shown
    ) {
        window_frame_t::draw();
    }
}

void ScreenMenuSTSWizard::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (GetFirstDialog()) {
        return;
    }

    static bool ever_shown_wizard_box { false };
    if (!ever_shown_wizard_box) {
        ever_shown_wizard_box = true;

        if (MsgBoxPepaCentered(_("Run selftests and calibrations now?"), { Response::Yes, Response::No }) != Response::Yes) {
            Screens::Access()->Close();
            return;
        }

        // Now show always, bed heater selftest can fail if there is no sheet on the bed
        MsgBoxInfo(_("Before you continue, make sure the print sheet is installed on the heatbed."), Responses_Ok);

        do_snake(get_first_action());
        return;
    }

    do_menu_event(this, sender, event, param, get_first_action(), false);

    if (snake_config.in_progress) {
        draw_enabled = false;
    } else {
        draw_enabled = true;
    }

    if (get_test_result(get_last_action(), Tool::_all_tools) == TestResult_Passed && are_previous_completed(get_last_action())) {
        MsgBoxPepaCentered(_("Happy printing!"),
            { Response::Continue, Response::_none, Response::_none, Response::_none });
        Screens::Access()->Close();
    }
}

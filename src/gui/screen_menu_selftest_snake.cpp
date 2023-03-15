#include "screen_menu_selftest_snake.hpp"
#include <png_resources.hpp>
#include <marlin_client.hpp>
#include <ScreenHandler.hpp>
#include <ScreenSelftest.hpp>
#include <DialogHandler.hpp>
#include <selftest_types.hpp>
#include <RAII.hpp>

using namespace SelftestSnake;

namespace {
constexpr Tool operator-(Tool tool, int i) {
    assert(ftrstd::to_underlying(tool) - i >= ftrstd::to_underlying(Tool::_first));
    return static_cast<Tool>(ftrstd::to_underlying(tool) - i);
}

constexpr Tool operator+(Tool tool, int i) {
    assert(ftrstd::to_underlying(tool) + i <= ftrstd::to_underlying(Tool::_last));
    return static_cast<Tool>(ftrstd::to_underlying(tool) + i);
}

// helper rename for clarity
inline bool is_multitool() {
    return prusa_toolchanger.is_toolchanger_enabled();
}

Action get_first_action() {
    if (is_multitool()) {
        return Action::_first;
    }
    Action checked_action { Action::_first };
    while (is_multitool_only_action(checked_action)) {
        checked_action = static_cast<Action>(ftrstd::to_underlying(checked_action) + 1);
    }
    return checked_action;
}

Action get_last_action() {
    if (is_multitool()) {
        return Action::_last;
    }
    Action checked_action { Action::_last };
    while (is_multitool_only_action(checked_action)) {
        checked_action = static_cast<Action>(ftrstd::to_underlying(checked_action) - 1);
    }
    return checked_action;
}

// Can't (shouldn't) be called with last action
Action get_next_action(Action action) {
    assert(get_last_action() != action && "Unhandled edge case");

    action = static_cast<Action>(ftrstd::to_underlying(action) + 1);
    if (is_multitool()) {
        return action;
    }

    while (is_multitool_only_action(action)) {
        action = static_cast<Action>(ftrstd::to_underlying(action) + 1);
    }

    return action;
}

// Can't (shouldn't) be called with first action
Action get_previous_action(Action action) {
    assert(get_first_action() != action && "Unhandled edge case");

    action = static_cast<Action>(ftrstd::to_underlying(action) - 1);
    if (is_multitool()) {
        return action;
    }

    while (is_multitool_only_action(action)) {
        action = static_cast<Action>(ftrstd::to_underlying(action) - 1);
    }

    return action;
}

TestResult get_test_result(Action action, Tool tool) {
    auto evaluate_results = [](std::same_as<TestResult> auto... results) constexpr->TestResult {
        static_assert(sizeof...(results) > 0, "Pass at least one result");

        if (((results == TestResult_Passed) && ... && true)) { // all passed
            return TestResult_Passed;
        } else if (((results == TestResult_Failed) || ... || false)) { // any failed
            return TestResult_Failed;
        } else if (((results == TestResult_Skipped) || ... || false)) { // any skipped
            return TestResult_Skipped;
        } else { // only unknowns and passed (max n-1) are left
            return TestResult_Unknown;
        }
    };

    auto merge_hotends_evaluations = [&](std::invocable<int8_t> auto evaluate_one) {
        TestResult res { TestResult_Passed };
        for (int8_t e = 0; e < HOTENDS; e++) {
            if (!prusa_toolchanger.is_tool_enabled(e)) {
                continue;
            }
            res = evaluate_results(res, evaluate_one(e));
        }
        return res;
    };

    SelftestResult sr;
    eeprom_get_selftest_results(&sr);

    switch (action) {
    case Action::Fans:
        return merge_hotends_evaluations(
            [&](int8_t e) {
                return evaluate_results(sr.tools[e].printFan, sr.tools[e].heatBreakFan);
            });
    case Action::ZAlign:
        return evaluate_results(sr.zalign);
    case Action::XYCheck:
        return evaluate_results(sr.xaxis, sr.yaxis);
    case Action::KennelCalibration:
        if (tool == Tool::_all_tools) {
            return merge_hotends_evaluations(
                [&](int8_t e) {
                    return evaluate_results(sr.tools[e].kenneloffset);
                });
        } else {
            return evaluate_results(sr.tools[ftrstd::to_underlying(tool)].kenneloffset);
        }
    case Action::Loadcell:
        if (tool == Tool::_all_tools) {
            return merge_hotends_evaluations(
                [&](int8_t e) {
                    return evaluate_results(sr.tools[e].loadcell);
                });
        } else {
            return evaluate_results(sr.tools[ftrstd::to_underlying(tool)].loadcell);
        }
    case Action::ToolOffsetsCalibration:
        return merge_hotends_evaluations(
            [&](int8_t e) {
                return evaluate_results(sr.tools[e].tooloffset);
            });
    case Action::ZCheck:
        return evaluate_results(sr.zaxis);
    case Action::Heaters:
        return evaluate_results(sr.bed, merge_hotends_evaluations([&](int8_t e) {
            return evaluate_results(sr.tools[e].nozzle);
        }));
    case Action::FilamentSensorCalibration:
        if (tool == Tool::_all_tools) {
            return merge_hotends_evaluations(
                [&](int8_t e) {
                    return evaluate_results(sr.tools[e].fsensor);
                });
        } else {
            return evaluate_results(sr.tools[ftrstd::to_underlying(tool)].fsensor);
        }
    case Action::_count:
        break;
    }
    return TestResult_Unknown;
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

const png::Resource *get_icon(Action action, Tool tool) {
    switch (get_test_result(action, tool)) {
    case TestResult_Passed:
        return &png::ok_color_16x16;
    case TestResult_Skipped:
    case TestResult_Unknown:
        return &png::na_color_16x16;
    case TestResult_Failed:
        return &png::nok_color_16x16;
    }

    assert(false);
    return &png::error_16x16;
}

uint8_t get_tool_mask(Tool tool) {
    switch (tool) {
    case Tool::Tool1:
        return ToolMask::ToolO;
    case Tool::Tool2:
        return ToolMask::Tool1;
    case Tool::Tool3:
        return ToolMask::Tool2;
    case Tool::Tool4:
        return ToolMask::Tool3;
    case Tool::Tool5:
        return ToolMask::Tool4;
        break;
    default:
        assert(false);
        break;
    }
    return ToolMask::AllTools;
}

uint64_t get_test_mask(Action action) {
    switch (action) {
    case Action::Fans:
        return stmFans;
    case Action::ZAlign:
        return stmZcalib;
    case Action::XYCheck:
        return stmXYAxis;
    case Action::KennelCalibration:
        return stmKennels;
    case Action::Loadcell:
        return stmLoadcell;
    case Action::ToolOffsetsCalibration:
        return stmToolOffsets;
    case Action::ZCheck:
        return stmZAxis;
    case Action::Heaters:
        return stmHeaters;
    case Action::FilamentSensorCalibration:
        return stmFSensor;
    default:
        assert(false);
        return stmNone;
    }
}

Tool get_last_enabled_tool() {
    return static_cast<Tool>(prusa_toolchanger.get_num_enabled_tools() - 1);
}

constexpr bool has_submenu(Action action) {
    switch (action) {
    case Action::KennelCalibration:
    case Action::Loadcell:
    case Action::FilamentSensorCalibration:
        return true;
    default:
        return false;
    }
}

struct SnakeConfig {
    enum class State {
        reset,
        first,
        not_first,
    };

    void reset() {
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

    bool in_progress { false };
    Action last_action { Action::_last };
    Tool last_tool { Tool::_first };
    State state { State::reset };
};

SnakeConfig snake_config {};
bool querying_user { false };

void do_snake(Action action, Tool tool = Tool::_first) {
    if (!are_previous_completed(action) && !snake_config.in_progress) {
        AutoRestore ar(querying_user, true);

        if (MsgBoxQuestion(_("Previous Calibrations & Tests are not all done. Continue anyway?"), Responses_YesNo, 1) == Response::No) {
            snake_config.reset();
            return;
        }
    }

    if (has_submenu(action)) {
        marlin_test_start_for_tools(get_test_mask(action), get_tool_mask(tool));
    } else {
        marlin_test_start(get_test_mask(action));
    }

    snake_config.next(action, tool);
};

void continue_snake() {
    if (get_test_result(snake_config.last_action, snake_config.last_tool) != TestResult_Passed) { // last selftest didn't pass
        snake_config.reset();
        return;
    }

    // if the last action was the last action possible
    if (snake_config.last_action == get_last_action()
        && (!has_submenu(get_last_action()) || snake_config.last_tool == get_last_enabled_tool())) {
        snake_config.reset();
        return;
    }

    if (snake_config.state == SnakeConfig::State::first // ran only one action so far
        && (snake_config.last_action != get_first_action() || get_test_result(get_next_action(get_first_action()), Tool::_all_tools) == TestResult_Passed)) {

        AutoRestore ar(querying_user, true);
        if (MsgBoxQuestion(_("Continue running Calibrations & Tests?"), Responses_YesNo, 1) == Response::No) {
            snake_config.reset();
            return; // stop after running the first one
        }
    }

    if (!is_multitool()
        || !has_submenu(snake_config.last_action)
        || snake_config.last_tool == get_last_enabled_tool()) { // singletool or wasn't submenu or was last in a submenu
        do_snake(get_next_action(snake_config.last_action));
    } else { // current submenu not yet finished
        do_snake(snake_config.last_action, snake_config.last_tool + 1);
    }
}

is_hidden_t get_subitem_hidden_state(Tool tool) {
    const auto idx { ftrstd::to_underlying(tool) };
    return prusa_toolchanger.is_tool_enabled(idx) ? is_hidden_t::no : is_hidden_t::yes;
}

is_hidden_t get_mainitem_hidden_state(Action action) {
    return !is_multitool() && is_multitool_only_action(action) ? is_hidden_t::yes : is_hidden_t::no;
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

} // unnamed namespace

// returns the parameter, filled
char *I_MI_STS::get_filled_menu_item_label(Action action) {
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

    struct ToolText {
        Action action;
        const char *label;
    };

    static constexpr ToolText blank_texts[] {
        { Action::Fans, N_("%d Fan Test") },
        { Action::ZAlign, N_("%d Z Alignment Calibration") },
        { Action::XYCheck, N_("%d XY Axis Test") },
        { Action::KennelCalibration, N_("%d Kennel Position Calibration") },
        { Action::Loadcell, N_("%d Loadcell Test") },
        { Action::ToolOffsetsCalibration, N_("%d Tool Offset Calibration") },
        { Action::ZCheck, N_("%d Z Axis Test") },
        { Action::Heaters, N_("%d Heater Test") },
        { Action::FilamentSensorCalibration, N_("%d Filament Sensor Calibration") },
    }; // could have been done with an array of texts directly, but there would be an order dependancy

    // std::array of labels indexed by action

    if (auto it = std::ranges::find_if(blank_texts, [&](const auto &elem) {
            return elem.action == action;
        });
        it != std::end(blank_texts)) {

        char buffer[max_label_len];
        _(it->label).copyToRAM(buffer, max_label_len);
        snprintf(label_buffer, max_label_len, buffer, action_indices[ftrstd::to_underlying(action)]);
    } else {
        assert(false && "Unable to find a label for this combination");
    }

    return label_buffer;
}

I_MI_STS::I_MI_STS(Action action)
    : WI_LABEL_t(_(get_filled_menu_item_label(action)),
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

void I_MI_STS::do_click(IWindowMenu &window_menu, Action action) {
    if (!has_submenu(action) || !is_multitool()) {
        do_snake(action);
    } else if (action == Action::KennelCalibration) {
        Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuKennelCalibration>);
    } else if (action == Action::Loadcell) {
        Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuLoadcellTest>);
    } else if (action == Action::FilamentSensorCalibration) {
        Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuFilamentSensorsCalibration>);
    } else {
        assert(false);
    }
}

I_MI_STS_SUBMENU::I_MI_STS_SUBMENU(const char *label, Action action, Tool tool)
    : WI_LABEL_t(_(label), get_icon(action, tool), is_enabled_t::yes, get_subitem_hidden_state(tool)) {
    set_icon_position(IconPosition::right);
}

void I_MI_STS_SUBMENU::do_click(IWindowMenu &window_menu, Tool tool, Action action) {
    do_snake(action, tool);
}

namespace {
void do_menu_event(window_t *sender, GUI_event_t event, void *param, Action action, bool is_submenu) {
    if (querying_user || event != GUI_event_t::LOOP || !snake_config.in_progress || SelftestInstance().IsInProgress()) {
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

inline bool is_menu_draw_enabled() {
    return !snake_config.in_progress // don't draw if snake is ongoing
        || querying_user;            // always draw if msgbox is being shown
}
} // unnamed namespace

ScreenMenuKennelCalibration::ScreenMenuKennelCalibration()
    : detail::ScreenMenuKennelCalibration(_(label)) {}

void ScreenMenuKennelCalibration::draw() {
    if (is_menu_draw_enabled()) {
        window_frame_t::draw();
    }
}

void ScreenMenuKennelCalibration::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    do_menu_event(sender, event, param, action, true);
}

ScreenMenuLoadcellTest::ScreenMenuLoadcellTest()
    : detail::ScreenMenuLoadcellTest(_(label)) {}

void ScreenMenuLoadcellTest::draw() {
    if (is_menu_draw_enabled()) {
        window_frame_t::draw();
    }
}

void ScreenMenuLoadcellTest::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    do_menu_event(sender, event, param, action, true);
}

ScreenMenuFilamentSensorsCalibration::ScreenMenuFilamentSensorsCalibration()
    : detail::ScreenMenuFilamentSensorsCalibration(_(label)) {}

void ScreenMenuFilamentSensorsCalibration::draw() {
    if (is_menu_draw_enabled()) {
        window_frame_t::draw();
    }
}

void ScreenMenuFilamentSensorsCalibration::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    do_menu_event(sender, event, param, action, true);
}

ScreenMenuSelftestSnake::ScreenMenuSelftestSnake()
    : SelftestSnake::detail::ScreenMenuSelftestSnake(_(label)) {
    ClrMenuTimeoutClose(); // No timeout for snake
}

void ScreenMenuSelftestSnake::draw() {
    if (is_menu_draw_enabled()) {
        window_frame_t::draw();
    }
}

void ScreenMenuSelftestSnake::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    do_menu_event(sender, event, param, get_first_action(), false);
}

ScreenMenuSTSWizard::ScreenMenuSTSWizard()
    : SelftestSnake::detail::ScreenMenuSTSWizard(_(label)) {
    header.SetIcon(&png::wizard_16x16);
    ClrMenuTimeoutClose(); // No timeout for wizard's snake
}

void ScreenMenuSTSWizard::draw() {
    if ((draw_enabled && !snake_config.in_progress) // don't draw if starting/ending or snake in progress
        || querying_user) {                         // but always draw if asking user
        window_frame_t::draw();
    }
}

void ScreenMenuSTSWizard::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (querying_user) {
        return;
    }

    static bool ever_shown_wizard_box { false };
    if (!ever_shown_wizard_box) {
        ever_shown_wizard_box = true;

        AutoRestore ar(querying_user, true);
        if (MsgBoxPepaCentered(_("Hi, this is your\nOriginal Prusa XL printer.\n"
                                 "I would like to guide you\nthrough the setup process."),
                { Response::Continue, Response::Cancel })
            == Response::Cancel) {
            Screens::Access()->Close();
        } else {
            do_snake(get_first_action());
        }
        return;
    }

    do_menu_event(sender, event, param, get_first_action(), false);

    if (snake_config.in_progress) {
        draw_enabled = false;
    } else {
        draw_enabled = true;
    }

    if (get_test_result(get_last_action(), Tool::_all_tools) == TestResult_Passed && are_previous_completed(get_last_action())) {
        AutoRestore ar(querying_user, true);

        MsgBoxPepaCentered(_("Happy printing!"),
            { Response::Continue, Response::_none, Response::_none, Response::_none });
        Screens::Access()->Close();
    }
}

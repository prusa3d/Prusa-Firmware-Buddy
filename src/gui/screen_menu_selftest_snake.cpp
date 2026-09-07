#include "screen_menu_selftest_snake.hpp"
#include <config_store/store_instance.hpp>
#include <img_resources.hpp>
#include <marlin_client.hpp>
#include <ScreenHandler.hpp>
#include <selftest_types.hpp>
#include <option/has_phase_stepping_selftest.h>
#include <option/has_side_fsensor_remap.h>
#include <option/has_gearbox_alignment.h>
#include <option/has_door_sensor_calibration.h>
#include <option/has_manual_belt_tuning.h>
#include <option/has_selftest_dependencies.h>
#include <printers.h>
#include <option/has_heaters_selftest_gcode.h>
#include <bsod/bsod.h>
#include "selftest/i_selftest.hpp"
#include <selftest/selftest_invocation.hpp>
#include <gui/wizard/screen_selftest_submenu.hpp>
#include <lang/i18n.h>
#include <selftest_action_helpers.hpp>
#include <selftest_dependencies.hpp>

#include <option/has_side_fsensor_remap.h>
#if HAS_SIDE_FSENSOR_REMAP()
    #include <feature/filament_sensor/filament_sensors_handler_remap.hpp>
#endif

#include <option/has_tool_offset_sensor.h>
#if HAS_TOOL_OFFSET_SENSOR()
    #include <feature/tool_offset_calibration/tool_offset_calibration.hpp>
#endif

using namespace SelftestSnake;

namespace SelftestSnake {

PhysicalToolIndex get_first_enabled_tool() {
    for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
        return tool;
    }
    return PhysicalToolIndex::from_raw(0);
}

PhysicalToolIndex get_last_enabled_tool() {
    auto result = PhysicalToolIndex::from_raw(0);
    for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
        result = tool;
    }
    return result;
}

PhysicalToolIndex get_next_tool(PhysicalToolIndex tool) {
    debug_assert(tool != get_last_enabled_tool() && "Unhandled edge case");
    do {
        tool = PhysicalToolIndex::from_raw(tool.to_raw() + 1);
    } while (!tool.is_enabled());
    return tool;
}

const img::Resource *get_icon(Action action, ToolMask mask) {
    switch (get_test_result(action, mask)) {
    case TestResult::passed:
        return &img::ok_color_16x16;
    case TestResult::skipped:
        return &img::ok_16x16;
    case TestResult::unknown:
        return &img::na_color_16x16;
    case TestResult::failed:
        return &img::nok_color_16x16;
    }

    debug_assert(false);
    return &img::error_16x16;
}

struct SnakeConfig {
    enum class AutoContinue {
        /// Ask whether to continue after finishing each test
        ask,

        /// Finish all tests in the submenu, then ask
        submenu,

        /// Continue doing all the tests without asking
        all,
    };

    void reset() {
        *this = {};
        last_action = get_last_action();
    }

    void next(Action action, PhysicalToolIndex tool) {
        in_progress = true;
        last_action = action;
        last_tool = tool;
    }

    bool in_progress { false }; ///< Is snake currently running?
    AutoContinue auto_continue { AutoContinue::ask }; ///< Should we continue running other selftests after finishing the current one?

    Action last_action { Action::_last }; ///< Last action that we'have done
    PhysicalToolIndex last_tool { PhysicalToolIndex::from_raw(0) };
};

} // namespace SelftestSnake

static SnakeConfig snake_config {};
static bool bypass_dependencies { false };

namespace {

void do_snake(Action action, PhysicalToolIndex tool) {
    // Reset invocation state so continue_snake sees aborts only from THIS test, not a previous one.
    selftest_invocation::begin();

    // Note: "gcode" tests are handled separately, partly because
    //       there are not enough bits in the selftest mask.
    {
        bool has_test_special_handling = true;

        switch (action) {

#if HAS_PHASE_STEPPING_SELFTEST()
        case Action::PhaseSteppingCalibration:
            marlin_client::gcode("M1977");
            break;
#endif
        case Action::Fans:
            marlin_client::gcode("M1978");
            break;
#if HAS_HEATERS_SELFTEST_GCODE()
        case Action::Heaters:
            marlin_client::gcode("M1987");
            break;
#endif

        case Action::FilamentSensorCalibration:
#if HAS_SIDE_FSENSOR_REMAP()
            if (!snake_config.in_progress || tool == PhysicalToolIndex::from_raw(0)) {
                // Ask user whether to remap filament sensors
                side_fsensor_remap::ask_to_remap();
            }
#endif
            marlin_client::gcode_printf("M1981 T%d", tool.to_raw());
            break;

#if HAS_GEARBOX_ALIGNMENT()
        case Action::Gears:
            marlin_client::gcode_printf("M1979 T%d", tool.to_raw());
            break;
#endif
#if HAS_DOOR_SENSOR_CALIBRATION()
        case Action::DoorSensor:
            marlin_client::gcode("M1980");
            break;
#endif
#if HAS_PRECISE_HOMING_COREXY()
        case Action::PreciseHoming:
            marlin_client::gcode("G28 XY C");
            break;
#endif
#if HAS_MANUAL_BELT_TUNING()
        case Action::BeltTuning:
            marlin_client::gcode("M961");
            break;
#endif
#if HAS_INDX()
        case Action::DockCalibration:
            marlin_client::gcode("M1982");
            break;
        case Action::NozzleCleanerCalibration:
            marlin_client::gcode("M1983");
            break;
        case Action::InputShaper:
            marlin_client::gcode("M1959");
            break;
#endif
#if HAS_TOOL_OFFSET_SENSOR()
        case Action::ToolOffsetsCalibration:
            if (tool_offset_calibration::is_hardware_available()) { // XLS: CAN bridge + sensor present
                marlin_client::gcode("M1985");
                break; // handled as a gcode test
            }
            has_test_special_handling = false; // plain XL: fall through to pin-based selftest mask
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
        marlin_client::test_start_with_data(get_test_mask(action), tool);
    } else {
        marlin_client::test_start(get_test_mask(action));
    }

    snake_config.next(action, tool);
};

void continue_snake() {
    const TestResult last_test_result = get_test_result(snake_config.last_action, snake_config.last_tool);
    if (!is_completed(last_test_result)
        || selftest_invocation::is_aborted()) { // last selftest didn't pass
        snake_config.reset();
        return;
    }

    // if the last action was the last action possible
    if (snake_config.last_action == get_last_action()
        && (!has_submenu(get_last_action()) || snake_config.last_tool == get_last_enabled_tool())) {
        snake_config.reset();
        return;
    }

    if (snake_config.auto_continue == SnakeConfig::AutoContinue::submenu && has_submenu(snake_config.last_action) && snake_config.last_tool == get_last_enabled_tool()) {
        snake_config.auto_continue = SnakeConfig::AutoContinue::ask;
    }

    if (snake_config.auto_continue == SnakeConfig::AutoContinue::ask) {
        if (is_multitool() && has_submenu(snake_config.last_action) && snake_config.last_tool != get_last_enabled_tool()) {
            const auto resp = MsgBoxQuestion(_("FINISH remaining calibrations without proceeding to other tests, or perform ALL Calibrations and Tests?\n\nIf you QUIT, all data up to this point is saved."), { Response::Finish, Response::All, Response::Quit }, 2);
            switch (resp) {

            case Response::Finish:
                snake_config.auto_continue = SnakeConfig::AutoContinue::submenu;
                break;

            case Response::All:
                snake_config.auto_continue = SnakeConfig::AutoContinue::all;
                break;

            case Response::Quit:
                snake_config.reset();
                return;

            default:
                bsod_unreachable();
            }

        } else {
            const auto resp = MsgBoxQuestion(_("Continue running Calibrations & Tests?"), { Response::Continue, Response::All, Response::Quit }, 2);
            switch (resp) {

            case Response::Continue:
                snake_config.auto_continue = SnakeConfig::AutoContinue::ask;
                break;

            case Response::All:
                snake_config.auto_continue = SnakeConfig::AutoContinue::all;
                break;

            case Response::Quit:
                snake_config.reset();
                return;

            default:
                bsod_unreachable();
            }
        }
    }

    if (!is_multitool()
        || !has_submenu(snake_config.last_action)
        || snake_config.last_tool == get_last_enabled_tool()) { // singletool or wasn't submenu or was last in a submenu
        do_snake(get_next_action(snake_config.last_action), get_first_enabled_tool());
    } else { // current submenu not yet finished
        do_snake(snake_config.last_action, get_next_tool(snake_config.last_tool));
    }
}

constexpr bool requires_toolchanger([[maybe_unused]] Action action) {
#if PRINTER_IS_PRUSA_XL()
    return action == Action::DockCalibration || action == Action::ToolOffsetsCalibration;
#else
    return false;
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
    static const std::array<size_t, std::to_underlying(Action::_count)> action_indices {
        []() {
            std::array<size_t, std::to_underlying(Action::_count)> indices { { {} } };

            int idx { 1 }; // start number
            for (Action act : valid_actions()) {
                indices[std::to_underlying(act)] = idx++;
            }
            return indices;
        }()
    };

    char name[max_label_len];
    _(get_action_label(action)).copyToRAM(name, max_label_len);
    snprintf(label_buffer, max_label_len, "%d %s", (int)action_indices[std::to_underlying(action)], name);

    return string_view_utf8::MakeRAM(label_buffer);
}

I_MI_STS::I_MI_STS(Action action)
    : IWindowMenuItem(get_filled_menu_item_label(action), nullptr, is_enabled_t::yes, get_mainitem_hidden_state(action), get_expands(action))
    , action(action) {}

static bool check_prerequisites_or_warn(Action action) {
    if (bypass_dependencies) {
        return true;
    }
#if HAS_SELFTEST_DEPENDENCIES()
    if (!are_dependencies_met(action)) {
        show_unmet_dependencies_warning(action);
        snake_config.reset();
        return false;
    }
#else
    if (!are_previous_completed(action)) {
        if (MsgBoxQuestion(_("Previous Calibrations & Tests are not all done. Continue anyway?"), Responses_YesNo, 1) == Response::No) {
            snake_config.reset();
            return false;
        }
    }
#endif
    return true;
}

void I_MI_STS::click(IWindowMenu &) {
    const bool can_run = check_prerequisites_or_warn(action);
    if (!can_run) {
        return;
    }
    if (!has_submenu(action) || !is_multitool()) {
        do_snake(action, get_first_enabled_tool());
    } else {
        Screens::Access()->Open(ScreenFactory::ScreenWithArg<ScreenSelftestSubmenu>(action));
    }
}

void I_MI_STS::Loop() {
    SetIconId(get_icon(action, AllTools {}));
    set_icon_position(IconPosition::before_extension);

    // NOTE: setting color scheme in ctor is not reliable because some selftests dont rebuild the menu, so the ctor would not re-run
#if HAS_SELFTEST_DEPENDENCIES()
    const bool ready = are_dependencies_met(action);
#else
    const bool ready = are_previous_completed(action);
#endif
    set_color_scheme(ready ? nullptr : &not_yet_ready_scheme);
}

I_MI_STS_SUBMENU::I_MI_STS_SUBMENU(const char *label_template, Action action, PhysicalToolIndex tool)
    : IWindowMenuItem(string_view_utf8 {}, nullptr, tool.is_enabled() ? is_enabled_t::yes : is_enabled_t::no, is_hidden_t::no)
    , action(action)
    , tool(tool) {
    SetLabel(_(label_template).formatted(label_params_, tool.display_index()));
    set_icon_position(IconPosition::before_extension);
}

void I_MI_STS_SUBMENU::click(IWindowMenu &) {
    do_snake(action, tool);
}

void I_MI_STS_SUBMENU::Loop() {
    SetIconId(get_icon(action, tool));
}

MI_BYPASS_DEPENDENCIES::MI_BYPASS_DEPENDENCIES()
    : WI_ICON_SWITCH_OFF_ON_t(bypass_dependencies, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

void MI_BYPASS_DEPENDENCIES::OnChange(size_t) {
    bypass_dependencies = value();
}

namespace SelftestSnake {
void do_menu_event(window_t *receiver, [[maybe_unused]] window_t *sender, GUI_event_t event, [[maybe_unused]] void *param, Action action, bool is_submenu) {
    if (receiver->GetFirstDialog() || event != GUI_event_t::LOOP || !snake_config.in_progress || SelftestInstance().IsInProgress() || marlin_vars().is_processing.get()) {
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

bool ScreenMenuSTSWizard::should_show() {
#if PRINTER_IS_PRUSA_iX()
    return false;
#else
    // A crude heuristic to make the wizard show only "on the first run".
    // Yes, we are ignoring other selftest results outside of this struct, but this is good enough for the purpose.
    return config_store().selftest_result.get() == config_store_ns::defaults::selftest_result;
#endif
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
    if (event != GUI_event_t::LOOP || GetFirstDialog()) {
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

        do_snake(get_first_action(), get_first_enabled_tool());
        snake_config.auto_continue = SnakeConfig::AutoContinue::all;
        return;
    }

    do_menu_event(this, sender, event, param, get_first_action(), false);

    if (snake_config.in_progress) {
        draw_enabled = false;
    } else {
        draw_enabled = true;
    }

#if HAS_SELFTEST_DEPENDENCIES()
    if (are_all_actions_completed()) {
#else
    if (is_completed(get_test_result(get_last_action(), AllTools {})) && are_previous_completed(get_last_action())) {
#endif
        Screens::Access()->Close();
    }
}

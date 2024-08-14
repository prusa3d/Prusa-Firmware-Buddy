/**
 * @file screen_crash_recovery.cpp
 * @brief GUI screen providing information about crash recovery
 * @date 2021-11-04
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "config_features.h"
#include "img_resources.hpp"
#include <guiconfig/guiconfig.h>
#include <option/has_side_leds.h>
#include <find_error.hpp>

#if HAS_SIDE_LEDS()
    #include <leds/side_strip_control.hpp>
#endif
// TODO do it in cmake
#if ENABLED(CRASH_RECOVERY)

    #include "screen_crash_recovery.hpp"
    #include "i18n.h"
    #include <guiconfig/wizard_config.hpp>
    #include "crash_recovery_type.hpp"
    #include "marlin_client.hpp" // marlin_client::FSM_response
    #include "sound.hpp"

    #include <option/has_toolchanger.h>

    #if HAS_TOOLCHANGER()
        #include <module/prusa/toolchanger.h>
    #endif /*HAS_TOOLCHANGER()*/

using namespace crash_recovery;

    #if HAS_MINI_DISPLAY()
static constexpr size_t row_1 = 109; // icon
static constexpr size_t row_3 = 205; // line
static constexpr size_t col_0 = 10;
static constexpr Rect16 icon_nozzle_rc { 97, row_1 - 5, 48, 48 };
static constexpr Rect16 icon_nozzle_crash_rc { 97 - 26, row_1, 48, 48 };
static constexpr size_t row_nok_shift = -31;
    #endif
    #if HAS_LARGE_DISPLAY()
static constexpr size_t row_1 = 120; // icon
static constexpr size_t row_3 = 226; // line
static constexpr size_t col_0 = 130;
static constexpr Rect16 icon_nozzle_rc { 216, row_1 - 5, 48, 48 };
static constexpr Rect16 icon_nozzle_crash_rc { 216 - 26, row_1, 48, 48 };
static constexpr size_t row_nok_shift = -70;
    #endif

static constexpr size_t char_h = 24;
static constexpr size_t row_0 = GuiDefaults::HeaderHeight; // long text
static constexpr size_t line_w = 220;
static constexpr size_t line_h = 3;

static constexpr size_t row_2 = row_3 - 24; // text
static constexpr size_t row_4 = row_3 + 7; // X
static constexpr size_t row_5 = row_4 + char_h + 1; // Y
static constexpr size_t text_h = row_1 - row_0;

static constexpr size_t col_1 = col_0 + 5;
static constexpr size_t col_3 = col_0 + line_w;
static constexpr size_t col_2 = col_3 - 18 - 5;
static constexpr size_t col_1_w = col_2 - col_1;

// Tool text and icons positions
static constexpr size_t tool_row_careful = row_1 + char_h; // Y
static constexpr size_t tool_row_0 = tool_row_careful + char_h; // Y
static constexpr size_t tool_row_1 = tool_row_0 + char_h; // Y
static constexpr size_t tool_row_2 = tool_row_1 + char_h; // Y
static constexpr size_t tool_col_0 = col_0; // X
static constexpr size_t tool_col_1 = col_2; // X
static constexpr size_t tool_text_w = 30;

static constexpr Rect16 text_long_rc { 0, row_0, GuiDefaults::ScreenWidth, row_1 - row_0 };
static constexpr Rect16 text_long_nok_rc { 0, row_0, GuiDefaults::ScreenWidth, row_3 + row_nok_shift - row_0 };
static constexpr Rect16 text_checking_axis_rc { col_0, row_2, col_3 - col_1, char_h };
static constexpr Rect16 line_rc { col_0, row_3, line_w, line_h };
static constexpr Rect16 text_x_axis_rc { col_1, row_4, col_1_w, char_h };
static constexpr Rect16 text_y_axis_rc { col_1, row_5, col_1_w, char_h };

static constexpr Rect16 line_nok_rc { col_0, row_3 + row_nok_shift, line_w, line_h };
static constexpr Rect16 text_x_axis_nok_rc { col_1, row_4 + row_nok_shift, col_1_w, char_h };
static constexpr Rect16 text_y_axis_nok_rc { col_1, row_5 + row_nok_shift, col_1_w, char_h };

static constexpr Rect16 text_long_repeat_rc { 0, row_0, GuiDefaults::ScreenWidth, row_1 - row_0 };
static constexpr Rect16 text_repeat_info_rc { 0, row_2, GuiDefaults::ScreenWidth, 2 * char_h };
static constexpr const char *en_text_long_check = N_("A printer collision\nhas been detected.");
static constexpr const char *en_text_axis_test = N_("Checking axes");
static constexpr const char *en_text_home_axes = N_("Homing");
static constexpr const char *en_text_X_axis = N_("X-axis");
static constexpr const char *en_text_Y_axis = N_("Y-axis");

static constexpr const char *en_text_long_short = find_error(ErrCode::CONNECT_CRASH_RECOVERY_AXIS_SHORT).err_text;
/**
 * There is no known way how this might happen on MINI printer to ordinary user.
 * If the motor is electrically disconnected, axis is too short.
 * If the pulley is loose, axis is too long, but the screen is not shown
 * as there is homing attempt before showing the screen which fails after 45 tries,
 * printer resets with homing failed red screen.
 */
static constexpr const char *en_text_long_long = find_error(ErrCode::CONNECT_CRASH_RECOVERY_AXIS_LONG).err_text;
static constexpr const char *en_text_long_repeat = find_error(ErrCode::CONNECT_CRASH_RECOVERY_REPEATED_CRASH).err_text;
static constexpr const char *en_text_repeat_info = N_("Try checking belt tension or decreasing\nsensitivity in the tune menu.");
    #if HAS_TOOLCHANGER()
static constexpr const char *en_text_repeat_info_tool = N_("Try checking belt tension, decreasing sensitivity\nin the tune menu or recalibrating dock position.");
static constexpr const char *en_text_long_tool = find_error(ErrCode::CONNECT_CRASH_RECOVERY_TOOL_PICKUP).err_text;
    #endif
static constexpr const char *en_text_long_homefail = find_error(ErrCode::CONNECT_CRASH_RECOVERY_HOME_FAIL).err_text;
static constexpr const char *en_text_homefail_info = N_("Try checking belt tension\nor debris on the axes.");
static constexpr const char *en_text_tool_careful = N_("!! Careful, tools are hot !!");

RepeatedBeep::RepeatedBeep() {
    Sound_Play(eSOUND_TYPE::WaitingBeep);
}

RepeatedBeep::~RepeatedBeep() {
    Sound_Stop();
}

SingleBeep::SingleBeep() {
    Sound_Play(eSOUND_TYPE::SingleBeep);
}

SingleBeep::~SingleBeep() {
    // No need to even stop the sound
}

WinsCheckAxis::WinsCheckAxis(ScreenCrashRecovery &screen)
    : text_long(&screen, text_long_rc, is_multiline::yes, is_closed_on_click_t::no, _(en_text_long_check))
    , icon_nozzle_crash(&screen, icon_nozzle_crash_rc, &img::nozzle_crash_101x64)
    , icon_nozzle(&screen, icon_nozzle_rc, &img::nozzle_shape_48x48)
    , text_checking_axis(&screen, text_checking_axis_rc, is_multiline::no, is_closed_on_click_t::no, _(en_text_axis_test))
    , line(&screen, line_rc)
    , text_x_axis(&screen, text_x_axis_rc, is_multiline::no, is_closed_on_click_t::no, _(en_text_X_axis))
    , icon_x_axis(&screen, { col_2, row_4 })
    , text_y_axis(&screen, text_y_axis_rc, is_multiline::no, is_closed_on_click_t::no, _(en_text_Y_axis))
    , icon_y_axis(&screen, { col_2, row_5 }) {

    line.SetBackColor(COLOR_ORANGE);
    text_long.SetAlignment(Align_t::Center());
    icon_x_axis.SetState(SelftestSubtestState_t::running);
    #if HAS_SIDE_LEDS()
    leds::side_strip_control.PresentColor(leds::Color(255, 0, 0), 400, 100);
    #endif
}

WinsHome::WinsHome(ScreenCrashRecovery &screen)
    : text_long(&screen, text_long_rc, is_multiline::yes, is_closed_on_click_t::no, _(en_text_long_check))
    , icon_nozzle_crash(&screen, icon_nozzle_crash_rc, &img::nozzle_crash_101x64)
    , icon_nozzle(&screen, icon_nozzle_rc, &img::nozzle_shape_48x48)
    , line(&screen, line_rc)
    , text_home_axes(&screen, text_x_axis_rc, is_multiline::no, is_closed_on_click_t::no, _(en_text_home_axes))
    , icon_home_axes(&screen, { col_2, row_4 }) {

    line.SetBackColor(COLOR_ORANGE);
    text_long.SetAlignment(Align_t::Center());
    icon_home_axes.SetState(SelftestSubtestState_t::running);
    #if HAS_SIDE_LEDS()
    leds::side_strip_control.PresentColor(leds::Color(255, 0, 0), 400, 100);
    #endif
}

WinsAxisNok::WinsAxisNok(ScreenCrashRecovery &screen)
    : text_long(&screen, text_long_nok_rc, is_multiline::yes, is_closed_on_click_t::no, string_view_utf8::MakeNULLSTR())
    , line(&screen, line_nok_rc)
    , text_x_axis(&screen, text_x_axis_nok_rc, is_multiline::no, is_closed_on_click_t::no, _(en_text_X_axis))
    , icon_x_axis(&screen, { col_2, row_4 + row_nok_shift })
    , text_y_axis(&screen, text_y_axis_nok_rc, is_multiline::no, is_closed_on_click_t::no, _(en_text_Y_axis))
    , icon_y_axis(&screen, { col_2, row_5 + row_nok_shift })
    , radio(&screen, GuiDefaults::GetButtonRect_AvoidFooter(screen.GetRect()), ClientResponses::GetResponses(PhasesCrashRecovery::axis_NOK), &texts) {

    line.SetBackColor(COLOR_ORANGE);
    text_long.SetAlignment(Align_t::Center());
    #if HAS_SIDE_LEDS()
    leds::side_strip_control.PresentColor(leds::Color(255, 0, 0), 400, 100);
    #endif
}

WinsRepeatedCrash::WinsRepeatedCrash(ScreenCrashRecovery &screen)
    : text_long(&screen, text_long_repeat_rc, is_multiline::yes, is_closed_on_click_t::no, _(en_text_long_repeat))
    , icon_nozzle_crash(&screen, icon_nozzle_crash_rc, &img::nozzle_crash_101x64)
    , icon_nozzle(&screen, icon_nozzle_rc, &img::nozzle_shape_48x48)
    , text_info(&screen, text_repeat_info_rc, is_multiline::yes, is_closed_on_click_t::no,
    #if HAS_TOOLCHANGER()
          prusa_toolchanger.is_toolchanger_enabled() ? _(en_text_repeat_info_tool) : _(en_text_repeat_info)
    #else
          _(en_text_repeat_info)
    #endif /*HAS_TOOLCHANGER()*/
              )
    , radio(&screen, GuiDefaults::GetButtonRect_AvoidFooter(screen.GetRect()), ClientResponses::GetResponses(PhasesCrashRecovery::repeated_crash), &texts) {

    text_long.SetAlignment(Align_t::Center());
    text_info.SetAlignment(Align_t::Center());
    text_info.set_font(Font::small);
    #if HAS_SIDE_LEDS()
    leds::side_strip_control.PresentColor(leds::Color(255, 0, 0), 400, 100);
    #endif
}

WinsHomeFail::WinsHomeFail(ScreenCrashRecovery &screen)
    : text_long(&screen, text_long_repeat_rc, is_multiline::yes, is_closed_on_click_t::no, _(en_text_long_homefail))
    , icon_nozzle_crash(&screen, icon_nozzle_crash_rc, &img::nozzle_crash_101x64)
    , icon_nozzle(&screen, icon_nozzle_rc, &img::nozzle_shape_48x48)
    , text_info(&screen, text_repeat_info_rc, is_multiline::yes, is_closed_on_click_t::no, _(en_text_homefail_info))
    , radio(&screen, GuiDefaults::GetButtonRect_AvoidFooter(screen.GetRect()), ClientResponses::GetResponses(PhasesCrashRecovery::home_fail), &texts) {

    text_long.SetAlignment(Align_t::Center());
    text_info.SetAlignment(Align_t::Center());
    text_info.set_font(Font::small);
    #if HAS_SIDE_LEDS()
    leds::side_strip_control.PresentColor(leds::Color(255, 0, 0), 400, 100);
    #endif
}

    #if HAS_TOOLCHANGER()
WinsToolRecovery::WinsToolRecovery(ScreenCrashRecovery &screen)
    : text_long(&screen, text_long_rc, is_multiline::yes, is_closed_on_click_t::no, _(en_text_long_tool))
    , text_careful(&screen, { 0, tool_row_careful, GuiDefaults::ScreenWidth, char_h }, is_multiline::yes, is_closed_on_click_t::no, _(en_text_tool_careful))
    , text_tool {
        { &screen, { tool_col_0 - tool_text_w, tool_row_0, tool_text_w, char_h }, is_multiline::no, is_closed_on_click_t::no, _("T1") },
        { &screen, { tool_col_0 - tool_text_w, tool_row_1, tool_text_w, char_h }, is_multiline::no, is_closed_on_click_t::no, _("T2") },
        { &screen, { tool_col_0 - tool_text_w, tool_row_2, tool_text_w, char_h }, is_multiline::no, is_closed_on_click_t::no, _("T3") },
        { &screen, { tool_col_1 - tool_text_w, tool_row_0, tool_text_w, char_h }, is_multiline::no, is_closed_on_click_t::no, _("T4") },
        { &screen, { tool_col_1 - tool_text_w, tool_row_1, tool_text_w, char_h }, is_multiline::no, is_closed_on_click_t::no, _("T5") },
        { &screen, { tool_col_1 - tool_text_w, tool_row_2, tool_text_w, char_h }, is_multiline::no, is_closed_on_click_t::no, _("T6") },
    }
    , icon_tool {
        { &screen, { tool_col_0 + 5, tool_row_0 } },
        { &screen, { tool_col_0 + 5, tool_row_1 } },
        { &screen, { tool_col_0 + 5, tool_row_2 } },
        { &screen, { tool_col_1 + 5, tool_row_0 } },
        { &screen, { tool_col_1 + 5, tool_row_1 } },
        { &screen, { tool_col_1 + 5, tool_row_2 } },
    }
    , radio(&screen, GuiDefaults::GetButtonRect_AvoidFooter(screen.GetRect()), ClientResponses::GetResponses(PhasesCrashRecovery::tool_recovery), &texts) {

    text_long.SetAlignment(Align_t::Center());
    text_careful.SetAlignment(Align_t::Center());
    text_tool[0].SetAlignment(Align_t::Right());
    text_tool[1].SetAlignment(Align_t::Right());
    text_tool[2].SetAlignment(Align_t::Right());
    text_tool[3].SetAlignment(Align_t::Right());
    text_tool[4].SetAlignment(Align_t::Right());
    text_tool[5].SetAlignment(Align_t::Right());
    radio.Hide(); // Disable until all are parked

    static_assert(EXTRUDERS == 6, "This screen is made for EXTRUDERS=6");

        #if HAS_SIDE_LEDS()
    leds::side_strip_control.PresentColor(leds::Color(255, 0, 0), 400, 100);
        #endif
}
    #endif /*HAS_TOOLCHANGER()*/

ScreenCrashRecovery::ScreenCrashRecovery()
    : screen_t()
    , header(this)
    , footer(this)
    , window { WinsCheckAxis(*this) } {

    ScreenCrashRecovery::ClrMenuTimeoutClose(); // don't close on menu timeout
    header.SetText(_("CRASH DETECTED"));
    header.SetIcon(&img::nozzle_16x16);
}

ScreenCrashRecovery::~ScreenCrashRecovery() {
    Sound_Stop();
}

bool ScreenCrashRecovery::Change(fsm::BaseData data) {
    change_phase(GetEnumFromPhaseIndex<PhasesCrashRecovery>(data.GetPhase()));

    const auto visitor = [&]<typename T>(T &mem) {
        if constexpr (std::is_same_v<T, WinsCheckAxis> || std::is_same_v<T, WinsAxisNok>) {
            Crash_recovery_fsm state(data.GetData());
            mem.icon_x_axis.SetState(state.x);
            mem.icon_y_axis.SetState(state.y);

        } else if constexpr (std::is_same_v<T, WinsHome>) {
            Crash_recovery_fsm state(data.GetData());
            mem.icon_home_axes.SetState(state.x);
        }

    #if HAS_TOOLCHANGER()
        else if constexpr (std::is_same_v<T, WinsToolRecovery>) {
            Crash_recovery_tool_fsm state(data.GetData());
            for (int i = 0; i < buddy::puppies::DWARF_MAX_COUNT; i++) {
                const auto tool_mask = (1 << i);
                if (state.enabled & tool_mask) { // This tool exists
                    if (state.parked & tool_mask) {
                        mem.icon_tool[i].SetState(SelftestSubtestState_t::ok); // Parked
                    } else {
                        mem.icon_tool[i].SetState(SelftestSubtestState_t::running); // Waiting to be parked
                    }

                } else { // Hide disabled tools
                    mem.text_tool[i].Hide();
                    mem.icon_tool[i].Hide();
                }
            }
            if (state.enabled == state.parked) {
                mem.radio.Show();
            } else {
                mem.radio.Hide(); // Disable button until all are parked
            }
        }
    #endif
    };
    std::visit(visitor, window);

    return true;
}

template <typename T>
concept WithRadio = requires(T a) {
    { a.radio } -> std::convertible_to<RadioButton &>;
};

void ScreenCrashRecovery::windowEvent(window_t * /*sender*/, GUI_event_t event, [[maybe_unused]] void *param) {
    const auto visitor = [&](auto &mem) -> IRadioButton * {
        if constexpr (WithRadio<decltype(mem)>) {
            return &mem.radio;
        } else {
            return nullptr;
        }
    };

    if (auto radio = std::visit(visitor, window)) {
        switch (event) {

        case GUI_event_t::CLICK: {
            Response response = radio->Click();
            auto send_phase = current_phase;
            if (!send_phase.has_value()) {
                break;
            }
            if (send_phase == PhasesCrashRecovery::axis_short || send_phase == PhasesCrashRecovery::axis_long) {
                send_phase = PhasesCrashRecovery::axis_NOK;
            }
            marlin_client::FSM_response(*send_phase, response);
            break;
        }

        case GUI_event_t::ENC_UP:
            ++(*radio);
            break;

        case GUI_event_t::ENC_DN:
            --(*radio);
            break;

        default:
            break;
        }
    }
}

void ScreenCrashRecovery::change_phase(PhasesCrashRecovery new_phase) {
    if (current_phase == new_phase) {
        return;
    }

    current_phase = new_phase;

    switch (new_phase) {

    case PhasesCrashRecovery::check_X:
    case PhasesCrashRecovery::check_Y:
        window.emplace<WinsCheckAxis>(*this);
        break;

    case PhasesCrashRecovery::home:
        window.emplace<WinsHome>(*this);
        break;

    case PhasesCrashRecovery::axis_NOK:
        break;

    case PhasesCrashRecovery::axis_short: {
        auto &win = window.emplace<WinsAxisNok>(*this);
        win.text_long.SetText(_(en_text_long_short));
        break;
    }

    case PhasesCrashRecovery::axis_long: {
        auto &win = window.emplace<WinsAxisNok>(*this);
        win.text_long.SetText(_(en_text_long_long));
        break;
    }

    case PhasesCrashRecovery::repeated_crash:
        window.emplace<WinsRepeatedCrash>(*this);
        break;

    case PhasesCrashRecovery::home_fail:
        window.emplace<WinsHomeFail>(*this);
        break;

    #if HAS_TOOLCHANGER()
    case PhasesCrashRecovery::tool_recovery:
        window.emplace<WinsToolRecovery>(*this);
        break;
    #endif /*HAS_TOOLCHANGER()*/
    }
}

#endif // ENABLED(CRASH_RECOVERY)

/**
 * @file screen_crash_recovery.cpp
 * @brief GUI screen providing information about crash recovery
 * @date 2021-11-04
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "config_features.h"
#include "png_resources.hpp"
// TODO do it in cmake
#if ENABLED(CRASH_RECOVERY)

    #include "screen_crash_recovery.hpp"
    #include "i18n.h"
    #include "wizard_config.hpp"
    #include "crash_recovery_type.hpp"
    #include "marlin_client.hpp" // marlin_FSM_response
    #include "sound.hpp"

using namespace crash_recovery;

    #ifdef USE_ST7789
static constexpr size_t row_1 = 109; // icon
static constexpr size_t row_3 = 205; // line
static constexpr size_t col_0 = 10;
static constexpr Rect16 icon_nozzle_rc { 97, row_1 - 5, 48, 48 };
static constexpr Rect16 icon_nozzle_crash_rc { 97 - 26, row_1, 48, 48 };
static constexpr size_t row_nok_shift = -31;
    #endif

static constexpr size_t char_h = 24;
static constexpr size_t row_0 = GuiDefaults::HeaderHeight; // long text
static constexpr size_t line_w = 220;
static constexpr size_t line_h = 3;

static constexpr size_t row_2 = row_3 - 24;         // text
static constexpr size_t row_4 = row_3 + 7;          // X
static constexpr size_t row_5 = row_4 + char_h + 1; // Y
static constexpr size_t text_h = row_1 - row_0;

static constexpr size_t col_1 = col_0 + 5;
static constexpr size_t col_3 = col_0 + line_w;
static constexpr size_t col_2 = col_3 - 18 - 5;
static constexpr size_t col_1_w = col_2 - col_1;

static constexpr Rect16 text_long_rc { 0, row_0, GuiDefaults::ScreenWidth, row_1 - row_0 };
static constexpr Rect16 text_long_nok_rc { 0, row_0, GuiDefaults::ScreenWidth, row_3 + row_nok_shift - row_0 };
static constexpr Rect16 text_checking_axis_rc { col_0, row_2, col_3 - col_1, char_h };
static constexpr Rect16 line_rc { col_0, row_3, line_w, line_h };
static constexpr Rect16 text_x_axis_rc { col_1, row_4, col_1_w, char_h };
static constexpr Rect16 text_y_axis_rc { col_1, row_5, col_1_w, char_h };

static constexpr Rect16 line_nok_rc { col_0, row_3 + row_nok_shift, line_w, line_h };
static constexpr Rect16 text_x_axis_nok_rc { col_1, row_4 + row_nok_shift, col_1_w, char_h };
static constexpr Rect16 text_y_axis_nok_rc { col_1, row_5 + row_nok_shift, col_1_w, char_h };

static constexpr int repeat_nozzle_shift = 20;
static constexpr Rect16 text_long_repeat_rc { 0, row_0, GuiDefaults::ScreenWidth, row_1 - row_0 };

static constexpr const char *en_text_long_check = N_("A printer collision\nhas been detected.");
static constexpr const char *en_text_axis_test = N_("Checking axes");
static constexpr const char *en_text_home_axes = N_("Homing");
static constexpr const char *en_text_X_axis = N_("X-axis");
static constexpr const char *en_text_Y_axis = N_("Y-axis");

static constexpr const char *en_text_long_short = N_("Length of an axis is too short.\nThere's an obstacle or bearing issue.\nRetry check, pause or resume the print?");
/**
 * There is no known way how this might happen on MINI printer to ordinary user.
 * If the motor is electrically disconnected, axis is too short.
 * If the pulley is loose, axis is too long, but the screen is not shown
 * as there is homing attempt before showing the screen which fails after 45 tries,
 * printer resets with homing failed red screen.
 */
static constexpr const char *en_text_long_long = N_("Length of an axis is too long.\nMotor current is too low, probably.\nRetry check, pause or resume the print?");
static constexpr const char *en_text_long_repeat = N_("Repeated collision\nhas been detected.\nDo you want to resume\nor pause the print?");

WinsCheckAxis::WinsCheckAxis(ScreenCrashRecovery &screen)
    : text_long(&screen, text_long_rc, is_multiline::yes, is_closed_on_click_t::no, _(en_text_long_check))
    , icon_nozzle_crash(&screen, icon_nozzle_crash_rc, &png::nozzle_crash_101x64)
    , icon_nozzle(&screen, icon_nozzle_rc, &png::nozzle_shape_48x48)
    , text_checking_axis(&screen, text_checking_axis_rc, is_multiline::no, is_closed_on_click_t::no, _(en_text_axis_test))
    , line(&screen, line_rc, line_h, COLOR_ORANGE, COLOR_ORANGE)
    , text_x_axis(&screen, text_x_axis_rc, is_multiline::no, is_closed_on_click_t::no, _(en_text_X_axis))
    , icon_x_axis(&screen, { col_2, row_4 })
    , text_y_axis(&screen, text_y_axis_rc, is_multiline::no, is_closed_on_click_t::no, _(en_text_Y_axis))
    , icon_y_axis(&screen, { col_2, row_5 }) {

    text_long.SetAlignment(Align_t::Center());
    icon_x_axis.SetState(SelftestSubtestState_t::running);
    Sound_Play(eSOUND_TYPE::SingleBeep);
}

WinsHome::WinsHome(ScreenCrashRecovery &screen)
    : text_long(&screen, text_long_rc, is_multiline::yes, is_closed_on_click_t::no, _(en_text_long_check))
    , icon_nozzle_crash(&screen, icon_nozzle_crash_rc, &png::nozzle_crash_101x64)
    , icon_nozzle(&screen, icon_nozzle_rc, &png::nozzle_shape_48x48)
    , line(&screen, line_rc, line_h, COLOR_ORANGE, COLOR_ORANGE)
    , text_home_axes(&screen, text_x_axis_rc, is_multiline::no, is_closed_on_click_t::no, _(en_text_home_axes))
    , icon_home_axes(&screen, { col_2, row_4 }) {

    text_long.SetAlignment(Align_t::Center());
    icon_home_axes.SetState(SelftestSubtestState_t::running);
    Sound_Play(eSOUND_TYPE::SingleBeep);
}

WinsAxisNok::WinsAxisNok(ScreenCrashRecovery &screen)
    : text_long(&screen, text_long_nok_rc, is_multiline::yes, is_closed_on_click_t::no, string_view_utf8::MakeNULLSTR())
    , line(&screen, line_nok_rc, line_h, COLOR_ORANGE, COLOR_ORANGE)
    , text_x_axis(&screen, text_x_axis_nok_rc, is_multiline::no, is_closed_on_click_t::no, _(en_text_X_axis))
    , icon_x_axis(&screen, { col_2, row_4 + row_nok_shift })
    , text_y_axis(&screen, text_y_axis_nok_rc, is_multiline::no, is_closed_on_click_t::no, _(en_text_Y_axis))
    , icon_y_axis(&screen, { col_2, row_5 + row_nok_shift })
    , radio(&screen, GuiDefaults::GetButtonRect_AvoidFooter(screen.GetRect()), ClientResponses::GetResponses(PhasesCrashRecovery::axis_NOK), &texts) {

    text_long.SetAlignment(Align_t::Center());
    Sound_Play(eSOUND_TYPE::WaitingBeep);
}

WinsRepeatedCrash::WinsRepeatedCrash(ScreenCrashRecovery &screen)
    : text_long(&screen, text_long_repeat_rc + Rect16::Height_t(repeat_nozzle_shift), is_multiline::yes, is_closed_on_click_t::no, _(en_text_long_repeat))
    , icon_nozzle_crash(&screen, icon_nozzle_crash_rc + Rect16::Y_t(repeat_nozzle_shift), &png::nozzle_crash_101x64)
    , icon_nozzle(&screen, icon_nozzle_rc + Rect16::Y_t(repeat_nozzle_shift), &png::nozzle_shape_48x48)
    , radio(&screen, GuiDefaults::GetButtonRect_AvoidFooter(screen.GetRect()), ClientResponses::GetResponses(PhasesCrashRecovery::repeated_crash), &texts) {

    text_long.SetAlignment(Align_t::Center());
    Sound_Play(eSOUND_TYPE::WaitingBeep);
}

WinUnion::WinUnion(ScreenCrashRecovery &screen)
    : parent_screen(screen) {
    New(PhasesCrashRecovery::_first);
}

WinUnion::screen_type WinUnion::ScreenType(PhasesCrashRecovery ph) {
    switch (ph) {
    case PhasesCrashRecovery::home:
        return WinUnion::Home;
    case PhasesCrashRecovery::axis_NOK:
    case PhasesCrashRecovery::axis_short:
    case PhasesCrashRecovery::axis_long:
        return WinUnion::AxisNok;
    case PhasesCrashRecovery::repeated_crash:
        return WinUnion::RepeatedCrash;
    case PhasesCrashRecovery::check_X:
    case PhasesCrashRecovery::check_Y:
        return WinUnion::CheckAxis;
    }
    return WinUnion::CheckAxis;
}

void WinUnion::ChangePhase(PhasesCrashRecovery ph) {
    if (ScreenType(phase) == ScreenType(ph)) {
        phase = ph;
        return;
    }
    Destroy();
    New(ph);
}

//The C++ language does allow a program to call a destructor directly, and, since it is not possible to destroy
//the object using a delete expression, that is how one destroys an object that was constructed via a pointer
//placement new expression
void WinUnion::Destroy() {
    Sound_Stop();
    switch (phase) {
    case PhasesCrashRecovery::check_X:
    case PhasesCrashRecovery::check_Y:
        checkAxis->~WinsCheckAxis();
        return;
    case PhasesCrashRecovery::home:
        home->~WinsHome();
        return;
    case PhasesCrashRecovery::axis_NOK:
        return;
    case PhasesCrashRecovery::axis_short:
    case PhasesCrashRecovery::axis_long:
        axisNok->~WinsAxisNok();
        return;
    case PhasesCrashRecovery::repeated_crash:
        repeatedCrash->~WinsRepeatedCrash();
        return;
    }
}

void WinUnion::New(PhasesCrashRecovery ph) {
    phase = ph;
    switch (phase) {
    case PhasesCrashRecovery::check_X:
    case PhasesCrashRecovery::check_Y:
        checkAxis = new (&mem_space) WinsCheckAxis(parent_screen);
        return;
    case PhasesCrashRecovery::home:
        home = new (&mem_space) WinsHome(parent_screen);
        return;
    case PhasesCrashRecovery::axis_NOK:
        return;
    case PhasesCrashRecovery::axis_short:
        axisNok = new (&mem_space) WinsAxisNok(parent_screen);
        axisNok->text_long.SetText(_(en_text_long_short));
        return;
    case PhasesCrashRecovery::axis_long:
        axisNok = new (&mem_space) WinsAxisNok(parent_screen);
        axisNok->text_long.SetText(_(en_text_long_long));
        return;
    case PhasesCrashRecovery::repeated_crash:
        repeatedCrash = new (&mem_space) WinsRepeatedCrash(parent_screen);
        return;
    }
}

ScreenCrashRecovery::ScreenCrashRecovery()
    : AddSuperWindow<screen_t>()
    , header(this)
    , footer(this)
    , win_union(*this) {

    ScreenCrashRecovery::ClrMenuTimeoutClose(); // don't close on menu timeout
    header.SetText(_("CRASH DETECTED"));
    header.SetIcon(&png::nozzle_16x16);
    ths = this;
}

ScreenCrashRecovery::~ScreenCrashRecovery() {
    Sound_Stop();
    ths = nullptr;
}

bool ScreenCrashRecovery::Change(fsm::BaseData data) {
    win_union.ChangePhase(GetEnumFromPhaseIndex<PhasesCrashRecovery>(data.GetPhase()));

    switch (win_union.phase) {
    case PhasesCrashRecovery::check_X:
    case PhasesCrashRecovery::check_Y: {
        Crash_recovery_fsm state(data.GetData());
        win_union.checkAxis->icon_x_axis.SetState(state.x);
        win_union.checkAxis->icon_y_axis.SetState(state.y);
        break;
    }
    case PhasesCrashRecovery::home: {
        Crash_recovery_fsm state(data.GetData());
        win_union.home->icon_home_axes.SetState(state.x);
        break;
    }
    case PhasesCrashRecovery::axis_NOK:
    case PhasesCrashRecovery::axis_short:
    case PhasesCrashRecovery::axis_long: {
        Crash_recovery_fsm state(data.GetData());
        win_union.axisNok->icon_x_axis.SetState(state.x);
        win_union.axisNok->icon_y_axis.SetState(state.y);
        break;
    }
    case PhasesCrashRecovery::repeated_crash:;
    }

    return true;
}

void ScreenCrashRecovery::windowEvent(EventLock /*has private ctor*/, window_t * /*sender*/, GUI_event_t event, void *param) {
    win_union.ButtonEvent(event);
}

//static variables and member functions
ScreenCrashRecovery *ScreenCrashRecovery::ths = nullptr;

ScreenCrashRecovery *ScreenCrashRecovery::GetInstance() { return ths; }

void WinUnion::ButtonEvent(GUI_event_t event) {
    RadioButton *radio = nullptr;
    switch (phase) {
    case PhasesCrashRecovery::check_X:
    case PhasesCrashRecovery::check_Y:
    case PhasesCrashRecovery::home:
        return;
    case PhasesCrashRecovery::repeated_crash:
        radio = &repeatedCrash->radio;
        break;
    case PhasesCrashRecovery::axis_NOK:
    case PhasesCrashRecovery::axis_short:
    case PhasesCrashRecovery::axis_long:
        radio = &axisNok->radio;
        break;
    }

    //has radio button
    if (radio) {
        switch (event) {
        case GUI_event_t::CLICK: {
            Response response = radio->Click();
            PhasesCrashRecovery send_phase = phase;
            if (phase == PhasesCrashRecovery::axis_short || phase == PhasesCrashRecovery::axis_long)
                send_phase = PhasesCrashRecovery::axis_NOK;
            marlin_FSM_response(send_phase, response);
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

#endif // ENABLED(CRASH_RECOVERY)

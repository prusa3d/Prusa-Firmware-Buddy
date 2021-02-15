//screen_printing.cpp
#include "dbg.h"
#include "screen_printing.hpp"
#include "marlin_client.h"
#include "print_utils.hpp"
#include "ffconf.h"
#include "ScreenHandler.hpp"
#include "screen_menus.hpp"
#include <ctime>
#include "wui_api.h"
#include "../lang/format_print_will_end.hpp"
#include "window_dlg_popup.hpp"

#ifdef DEBUG_FSENSOR_IN_HEADER
    #include "filament_sensor.hpp"
#endif

enum class Btn {
    Tune = 0,
    Pause,
    Stop
};

const uint16_t printing_icons[static_cast<size_t>(item_id_t::count)] = {
    IDR_PNG_settings_58px,
    IDR_PNG_pause_58px,
    IDR_PNG_pause_58px, //same as pause
    IDR_PNG_stop_58px,
    IDR_PNG_resume_48px,
    IDR_PNG_resume_48px,
    IDR_PNG_resume_48px, //reheating is same as resume, bud disabled
    IDR_PNG_reprint_48px,
    IDR_PNG_home_58px,
};

const char *printing_labels[static_cast<size_t>(item_id_t::count)] = {
    N_("Tune"),
    N_("Pause"),
    N_("Pausing..."),
    N_("Stop"),
    N_("Resume"),
    N_("Resuming..."),
    N_("Heating..."),
    N_("Reprint"),
    N_("Home"),
};

void screen_printing_data_t::invalidate_print_state() {
    state__readonly__use_change_print_state = printing_state_t::COUNT;
}
printing_state_t screen_printing_data_t::GetState() const {
    return state__readonly__use_change_print_state;
}

void screen_printing_data_t::tuneAction() {
    if (btn_tune.ico.IsShadowed()) {
        return;
    }
    switch (GetState()) {
    case printing_state_t::PRINTING:
    case printing_state_t::PAUSED:
        Screens::Access()->Open(GetScreenMenuTune);
        break;
    default:
        break;
    }
}

void screen_printing_data_t::pauseAction() {
    if (btn_pause.ico.IsShadowed()) {
        return;
    }
    switch (GetState()) {
    case printing_state_t::PRINTING:
        marlin_print_pause();
        change_print_state();
        break;
    case printing_state_t::PAUSED:
        marlin_print_resume();
        change_print_state();
        break;
    case printing_state_t::PRINTED:
        screen_printing_reprint();
        change_print_state();
        break;
    default:
        break;
    }
}

void screen_printing_data_t::stopAction() {
    if (btn_stop.ico.IsShadowed()) {
        return;
    }
    switch (GetState()) {
    case printing_state_t::PRINTED:
        Screens::Access()->Close();
        return;
    case printing_state_t::PAUSING:
    case printing_state_t::RESUMING:
        return;
    default: {
        if (MsgBoxWarning(_("Are you sure to stop this printing?"), Responses_YesNo, 1)
            == Response::Yes) {
            stop_pressed = true;
            waiting_for_abort = true;
            change_print_state();
        } else
            return;
    }
    }
}

/******************************************************************************/

screen_printing_data_t::screen_printing_data_t()
    : AddSuperWindow<ScreenPrintingModel>(_(caption))
    , w_filename(this, Rect16(10, 33, 220, 29))
    , w_progress(this, { 10, 70 }, HasNumber_t::yes)
    , w_time_label(this, Rect16(10, 128, 101, 20), is_multiline::no)
    , w_time_value(this, Rect16(10, 148, 101, 20), is_multiline::no)
    , w_etime_label(this, Rect16(130, 128, 101, 20), is_multiline::no)
    , w_etime_value(this, Rect16(30, 148, 201, 20), is_multiline::no)
    , last_print_duration(-1)
    , last_time_to_end(-1)
    , message_timer(0)
    , stop_pressed(false)
    , waiting_for_abort(false)
    , state__readonly__use_change_print_state(printing_state_t::COUNT)
    , popup_rect(Rect16::Merge(std::array<Rect16, 4>({ w_time_label.rect, w_time_value.rect, w_etime_label.rect, w_etime_value.rect }))) {
    marlin_error_clr(MARLIN_ERR_ProbingFailed);

    marlin_vars_t *vars = marlin_vars();

    strlcpy(text_time_dur.data(), "0m", text_time_dur.size());
    strlcpy(text_filament.data(), "999m", text_filament.size());

    w_filename.font = resource_font(IDR_FNT_BIG);
    w_filename.SetPadding({ 0, 0, 0, 0 });
    w_filename.SetAlignment(Align_t::LeftBottom());
    // this MakeRAM is safe - vars->media_LFN is statically allocated (even though it may not be obvious at the first look)
    w_filename.SetText(vars->media_LFN ? string_view_utf8::MakeRAM((const uint8_t *)vars->media_LFN) : string_view_utf8::MakeNULLSTR());

    w_etime_label.font = resource_font(IDR_FNT_SMALL);
    w_etime_label.SetAlignment(Align_t::RightBottom());
    w_etime_label.SetPadding({ 0, 2, 0, 2 });
    w_etime_label.SetText(_("Remaining Time"));

    w_etime_value.font = resource_font(IDR_FNT_SMALL);
    w_etime_value.SetAlignment(Align_t::RightBottom());
    w_etime_value.SetPadding({ 0, 2, 0, 2 });
    // this MakeRAM is safe - text_etime is allocated in RAM for the lifetime of pw
    w_etime_value.SetText(string_view_utf8::MakeRAM((const uint8_t *)text_etime.data()));

    w_time_label.font = resource_font(IDR_FNT_SMALL);
    w_time_label.SetAlignment(Align_t::RightBottom());
    w_time_label.SetPadding({ 0, 2, 0, 2 });
    w_time_label.SetText(_("Printing time"));

    w_time_value.font = resource_font(IDR_FNT_SMALL);
    w_time_value.SetAlignment(Align_t::RightBottom());
    w_time_value.SetPadding({ 0, 2, 0, 2 });
    // this MakeRAM is safe - text_time_dur is allocated in RAM for the lifetime of pw
    w_time_value.SetText(string_view_utf8::MakeRAM((const uint8_t *)text_time_dur.data()));

    initAndSetIconAndLabel(btn_tune, res_tune);
    initAndSetIconAndLabel(btn_pause, res_pause);
    initAndSetIconAndLabel(btn_stop, res_stop);
    change_etime();
}

#ifdef DEBUG_FSENSOR_IN_HEADER
extern int _is_in_M600_flg;
extern uint32_t *pCommand;
#endif

void screen_printing_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
#ifdef DEBUG_FSENSOR_IN_HEADER
    static int _last = 0;
    if (HAL_GetTick() - _last > 300) {
        _last = HAL_GetTick();

        static char buff[] = "Sx Mx x xxxx";                         //"x"s are replaced
        buff[1] = FS_instance().Get() + '0';                         // S0 init, S1 has filament, S2 no filament, S3 not connected, S4 disabled
        buff[4] = FS_instance().GetM600_send_on();                   // Me edge, Ml level, Mn never, Mx undefined
        buff[6] = FS_instance().WasM600_send() ? 's' : 'n';          // s == send, n== not send
        buff[8] = _is_in_M600_flg ? 'M' : '0';                       // M == marlin is doing M600
        buff[9] = marlin_event(MARLIN_EVT_CommandBegin) ? 'B' : '0'; // B == Event begin
        buff[10] = marlin_command() == MARLIN_CMD_M600 ? 'C' : '0';  // C == Command M600
        buff[11] = *pCommand == MARLIN_CMD_M600 ? 's' : '0';         // s == server - Command M600
        header.SetText(buff);
    }

#endif

    /// check stop clicked when MBL is running
    printing_state_t p_state = GetState();
    if (stop_pressed && waiting_for_abort && marlin_command() != MARLIN_CMD_G29 && (p_state == printing_state_t::ABORTING || p_state == printing_state_t::PAUSED)) {
        marlin_print_abort();
        waiting_for_abort = false;
        return;
    }

    if (p_state == printing_state_t::PRINTED && marlin_error(MARLIN_ERR_ProbingFailed)) {
        marlin_error_clr(MARLIN_ERR_ProbingFailed);
        if (MsgBox(_("Bed leveling failed. Try again?"), Responses_YesNo) == Response::Yes) {
            screen_printing_reprint();
        } else {
            Screens::Access()->Close();
            return;
        }
    }

    change_print_state();

    if (marlin_vars()->print_duration != last_print_duration)
        update_print_duration(marlin_vars()->print_duration);
    if (marlin_vars()->time_to_end != last_time_to_end) {
        change_etime();
    }

    /// -- close screen when print is done / stopped and USB media is removed
    if (!marlin_vars()->media_inserted && p_state == printing_state_t::PRINTED) {
        Screens::Access()->Close();
        return;
    }

    /// -- check when media is or isn't inserted
    if (event == GUI_event_t::MEDIA) {
        /// -- check for enable/disable resume button
        set_pause_icon_and_label();
    }

    SuperWindowEvent(sender, event, param);
}

void screen_printing_data_t::change_etime() {
    time_t sec = sntp_get_system_time();
    if (sec != 0) {
        // store string_view_utf8 for later use - should be safe, we get some static string from flash, no need to copy it into RAM
        // theoretically it can be removed completely in case the string is constant for the whole run of the screen
        w_etime_label.SetText(label_etime = _("Print will end"));
        update_end_timestamp(sec, marlin_vars()->print_speed);
    } else {
        // store string_view_utf8 for later use - should be safe, we get some static string from flash, no need to copy it into RAM
        w_etime_label.SetText(label_etime = _("Remaining Time"));
        update_remaining_time(marlin_vars()->time_to_end, marlin_vars()->print_speed);
    }
    last_time_to_end = marlin_vars()->time_to_end;
}

void screen_printing_data_t::disable_tune_button() {
    btn_tune.ico.Shadow();
    btn_tune.ico.Disable(); // can't be focused

    // move to reprint when tune is focused
    if (btn_tune.ico.IsFocused()) {
        btn_pause.ico.SetFocus();
    }
    btn_tune.ico.Invalidate();
}

void screen_printing_data_t::enable_tune_button() {
    btn_tune.ico.Unshadow();
    btn_tune.ico.Enable(); // can be focused
    btn_tune.ico.Invalidate();
}

void screen_printing_data_t::update_remaining_time(uint32_t sec, uint16_t print_speed) {
    bool is_time_valid = sec < (60 * 60 * 24 * 365); // basic check, check of year in tm struct, does not work
    w_etime_value.color_text = is_time_valid ? GuiDefaults::COLOR_VALUE_VALID : GuiDefaults::COLOR_VALUE_INVALID;
    if (is_time_valid) {
        time_t rawtime = time_t(sec);
        if (print_speed != 100) {
            // multiply by 100 is safe, it limits time_to_end to ~21mil. seconds (248 days)
            rawtime = (rawtime * 100) / print_speed;
        }
        const struct tm *timeinfo = localtime(&rawtime);
        //standard would be:
        //strftime(array.data(), array.size(), "%jd %Hh", timeinfo);
        if (timeinfo->tm_yday) {
            snprintf(text_etime.data(), MAX_END_TIMESTAMP_SIZE, "%id %2ih", timeinfo->tm_yday, timeinfo->tm_hour);
        } else if (timeinfo->tm_hour) {
            snprintf(text_etime.data(), MAX_END_TIMESTAMP_SIZE, "%ih %2im", timeinfo->tm_hour, timeinfo->tm_min);
        } else {
            snprintf(text_etime.data(), MAX_END_TIMESTAMP_SIZE, "%im", timeinfo->tm_min);
        }
    } else {
        if (print_speed != 100)
            strlcat(text_etime.data(), "?", MAX_END_TIMESTAMP_SIZE);
        else
            strlcpy(text_etime.data(), "N/A", MAX_END_TIMESTAMP_SIZE);
    }
    // this MakeRAM is safe - text_etime is allocated in RAM for the lifetime of pw
    w_etime_value.SetText(string_view_utf8::MakeRAM((const uint8_t *)text_etime.data()));
}

void screen_printing_data_t::update_end_timestamp(time_t now_sec, uint16_t print_speed) {

    bool time_invalid = false;
    if (marlin_vars()->time_to_end == TIME_TO_END_INVALID) {
        w_etime_value.color_text = GuiDefaults::COLOR_VALUE_INVALID;
        time_invalid = true;
    } else {
        w_etime_value.color_text = GuiDefaults::COLOR_VALUE_VALID;
    }

    static const uint32_t full_day_in_seconds = 86400;
    time_t print_end_sec, tomorrow_sec;

    if (print_speed != 100)
        // multiply by 100 is safe, it limits time_to_end to ~21mil. seconds (248 days)
        print_end_sec = now_sec + (100 * marlin_vars()->time_to_end / print_speed);
    else
        print_end_sec = now_sec + marlin_vars()->time_to_end;

    tomorrow_sec = now_sec + full_day_in_seconds;

    struct tm tomorrow, print_end, now;
    localtime_r(&now_sec, &now);
    localtime_r(&tomorrow_sec, &tomorrow);
    localtime_r(&print_end_sec, &print_end);

    if (now.tm_mday == print_end.tm_mday && // if print end is today
        now.tm_mon == print_end.tm_mon && now.tm_year == print_end.tm_year) {
        //strftime(pw->text_etime.data(), MAX_END_TIMESTAMP_SIZE, "Today at %H:%M?", &print_end); //@@TODO translate somehow
        FormatMsgPrintWillEnd::Today(text_etime.data(), MAX_END_TIMESTAMP_SIZE, &print_end, true);
    } else if (tomorrow.tm_mday == print_end.tm_mday && // if print end is tomorrow
        tomorrow.tm_mon == print_end.tm_mon && tomorrow.tm_year == print_end.tm_year) {
        //        strftime(pw->text_etime.data(), MAX_END_TIMESTAMP_SIZE, "%a at %H:%MM", &print_end);
        FormatMsgPrintWillEnd::DayOfWeek(text_etime.data(), MAX_END_TIMESTAMP_SIZE, &print_end, true);
    } else {
        //        strftime(pw->text_etime.data(), MAX_END_TIMESTAMP_SIZE, "%m-%d at %H:%MM", &print_end);
        FormatMsgPrintWillEnd::Date(text_etime.data(), MAX_END_TIMESTAMP_SIZE, &print_end, true, FormatMsgPrintWillEnd::ISO);
    }
    if (print_speed != 100)
        strlcat(text_etime.data(), "?", MAX_END_TIMESTAMP_SIZE);

    if (time_invalid == false) {
        text_etime[text_etime.size() - 1] = 0; // safety \0 termination in all cases
    }
    // this MakeRAM is safe - text_etime is allocated in RAM for the lifetime of pw
    w_etime_value.SetText(string_view_utf8::MakeRAM((const uint8_t *)text_etime.data()));
}
void screen_printing_data_t::update_print_duration(time_t rawtime) {
    w_time_value.color_text = GuiDefaults::COLOR_VALUE_VALID;
    const struct tm *timeinfo = localtime(&rawtime);
    if (timeinfo->tm_yday) {
        snprintf(text_time_dur.data(), MAX_TIMEDUR_STR_SIZE, "%id %2ih", timeinfo->tm_yday, timeinfo->tm_hour);
    } else if (timeinfo->tm_hour) {
        snprintf(text_time_dur.data(), MAX_TIMEDUR_STR_SIZE, "%ih %2im", timeinfo->tm_hour, timeinfo->tm_min);
    } else if (timeinfo->tm_min) {
        snprintf(text_time_dur.data(), MAX_TIMEDUR_STR_SIZE, "%im %2is", timeinfo->tm_min, timeinfo->tm_sec);
    } else {
        snprintf(text_time_dur.data(), MAX_TIMEDUR_STR_SIZE, "%is", timeinfo->tm_sec);
    }
    // this MakeRAM is safe - text_time_dur is allocated in RAM for the lifetime of pw
    w_time_value.SetText(string_view_utf8::MakeRAM((const uint8_t *)text_time_dur.data()));
}

void screen_printing_data_t::screen_printing_reprint() {
    print_begin(marlin_vars()->media_SFN_path);
    w_etime_label.SetText(_("Remaining Time"));
    btn_stop.txt.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)printing_labels[static_cast<size_t>(item_id_t::stop)]));
    btn_stop.ico.SetIdRes(printing_icons[static_cast<size_t>(item_id_t::stop)]);

#ifndef DEBUG_FSENSOR_IN_HEADER
    header.SetText(_("PRINTING"));
#endif
}

//todo use it
/*static void mesh_err_stop_print() {
    float target_nozzle = marlin_vars()->target_nozzle;
    float target_bed = marlin_vars()->target_bed;
    marlin_print_abort();
    while (marlin_vars()->sd_printing) {
        gui_loop();
    }
    //marlin_park_head();
    marlin_gcode_printf("M104 S%F", (double)target_nozzle);
    marlin_gcode_printf("M140 S%F", (double)target_bed);
    marlin_gcode("G0 Z30"); //Z 30mm
    marlin_gcode("M84");    //Disable steppers
    while (marlin_vars()->pqueue) {
        gui_loop();
    }
}*/

void screen_printing_data_t::set_icon_and_label(item_id_t id_to_set, window_icon_t *p_button, window_text_t *lbl) {
    size_t index = static_cast<size_t>(id_to_set);
    if (p_button->GetIdRes() != printing_icons[index])
        p_button->SetIdRes(printing_icons[index]);
    lbl->SetText(_(printing_labels[index]));
}

void screen_printing_data_t::enable_button(window_icon_t *p_button) {
    if (p_button->IsShadowed()) {
        p_button->Unshadow();
        p_button->Enable();
        p_button->Invalidate();
    }
}

void screen_printing_data_t::disable_button(window_icon_t *p_button) {
    if (!p_button->IsShadowed()) {
        p_button->Shadow();
        p_button->Disable();
        p_button->Invalidate();
    }
}

void screen_printing_data_t::set_pause_icon_and_label() {
    window_icon_t *const p_button = &btn_pause.ico;
    window_text_t *const pLabel = &btn_pause.txt;

    //todo it is static, because menu tune is not dialog
    //switch (state__readonly__use_change_print_state)
    switch (GetState()) {
    case printing_state_t::COUNT:
    case printing_state_t::INITIAL:
    case printing_state_t::PRINTING:
    case printing_state_t::MBL_FAILED:
        enable_button(p_button);
        set_icon_and_label(item_id_t::pause, p_button, pLabel);
        break;
    case printing_state_t::PAUSING:
        disable_button(p_button);
        set_icon_and_label(item_id_t::pausing, p_button, pLabel);
        break;
    case printing_state_t::PAUSED:
        enable_button(p_button);
        set_icon_and_label(item_id_t::resume, p_button, pLabel);
        if (!marlin_vars()->media_inserted) {
            disable_button(p_button);
        }
        break;
    case printing_state_t::RESUMING:
        disable_button(p_button);
        set_icon_and_label(item_id_t::resuming, p_button, pLabel);
        break;
    case printing_state_t::REHEATING:
    case printing_state_t::REHEATING_DONE:
        disable_button(p_button);
        set_icon_and_label(item_id_t::reheating, p_button, pLabel);
        break;
    case printing_state_t::PRINTED:
        enable_button(p_button);
        set_icon_and_label(item_id_t::reprint, p_button, pLabel);
        break;
    case printing_state_t::ABORTING:
        disable_button(p_button);
        break;
    }
}

void screen_printing_data_t::set_tune_icon_and_label() {
    window_icon_t *const p_button = &btn_tune.ico;
    window_text_t *const pLabel = &btn_tune.txt;

    //must be before switch
    set_icon_and_label(item_id_t::settings, p_button, pLabel);

    switch (GetState()) {
    case printing_state_t::PRINTING:
        // case printing_state_t::PAUSED:
        enable_tune_button();
        break;
    case printing_state_t::ABORTING:
        disable_button(p_button);
        break;
    default:
        disable_tune_button();
        break;
    }
}

void screen_printing_data_t::set_stop_icon_and_label() {
    window_icon_t *const p_button = &btn_stop.ico;
    window_text_t *const pLabel = &btn_stop.txt;

    switch (GetState()) {
    case printing_state_t::PRINTED:
        enable_button(p_button);
        set_icon_and_label(item_id_t::home, p_button, pLabel);
        break;
    case printing_state_t::PAUSING:
    case printing_state_t::RESUMING:
        disable_button(p_button);
        set_icon_and_label(item_id_t::stop, p_button, pLabel);
        break;
    case printing_state_t::ABORTING:
        disable_button(p_button);
        break;
    default:
        enable_button(p_button);
        set_icon_and_label(item_id_t::stop, p_button, pLabel);
        break;
    }
}

void screen_printing_data_t::change_print_state() {
    printing_state_t st = printing_state_t::COUNT;

    switch (marlin_vars()->print_state) {
    case mpsIdle:
        st = printing_state_t::INITIAL;
        break;
    case mpsPrinting:
        st = printing_state_t::PRINTING;
        break;
    case mpsPaused:
        // stop_pressed = false;
        st = printing_state_t::PAUSED;
        break;
    case mpsPausing_Begin:
    case mpsPausing_WaitIdle:
    case mpsPausing_ParkHead:
        st = printing_state_t::PAUSING;
        break;
    case mpsResuming_Reheating:
        stop_pressed = false;
        st = printing_state_t::REHEATING;
        break;
    case mpsResuming_Begin:
    case mpsResuming_UnparkHead:
        stop_pressed = false;
        st = printing_state_t::RESUMING;
        break;
    case mpsAborting_Begin:
    case mpsAborting_WaitIdle:
    case mpsAborting_ParkHead:
        stop_pressed = false;
        st = printing_state_t::ABORTING;
        break;
    case mpsFinishing_WaitIdle:
    case mpsFinishing_ParkHead:
        st = printing_state_t::PRINTING;
        break;
    case mpsAborted:
        st = printing_state_t::PRINTED;
        break;
    case mpsFinished:
        st = printing_state_t::PRINTED;
        break;
    }
    if (stop_pressed) {
        st = printing_state_t::ABORTING;
    }
    if (state__readonly__use_change_print_state != st) {
        state__readonly__use_change_print_state = st;
        set_pause_icon_and_label();
        set_tune_icon_and_label();
        set_stop_icon_and_label();
    }
}

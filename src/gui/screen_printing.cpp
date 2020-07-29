#include "dbg.h"
#include "gui.hpp"
#include "config.h"
#include "window_header.hpp"
#include "status_footer.h"
#include "marlin_client.h"
#include "marlin_server.h"
#include "print_utils.h"
#include "screens.h"
#include "ffconf.h"
#include <array>
#include <ctime>
#include "wui_api.h"
#include "../lang/i18n.h"
#include "../lang/format_print_will_end.hpp"

#ifdef DEBUG_FSENSOR_IN_HEADER
    #include "filament_sensor.h"
#endif

#define COLOR_VALUE_VALID COLOR_WHITE
//#define COLOR_VALUE_INVALID COLOR_YELLOW
#define COLOR_VALUE_INVALID COLOR_WHITE

enum class Btn {
    Tune = 0,
    Pause,
    Stop
};

constexpr static const size_t POPUP_MSG_DUR_MS = 5000;
constexpr static const size_t MAX_END_TIMESTAMP_SIZE = 14 + 12 + 5; // "dd.mm.yyyy at hh:mm:ss" + safty measures for 3digit where 2 digits should be
constexpr static const size_t MAX_TIMEDUR_STR_SIZE = 9;

enum class printing_state_t : uint8_t {
    INITIAL,
    PRINTING,
    PAUSING,
    PAUSED,
    RESUMING,
    ABORTING,
    REHEATING,
    REHEATING_DONE,
    MBL_FAILED,
    PRINTED,
    COUNT //setting this state == forced update
};

enum class item_id_t {
    settings,
    pause,
    pausing,
    stop,
    resume,
    resuming,
    reheating,
    reprint,
    home,
    count
};

const uint16_t printing_icons[static_cast<size_t>(item_id_t::count)] = {
    IDR_PNG_menu_icon_settings,
    IDR_PNG_menu_icon_pause,
    IDR_PNG_menu_icon_pause, //same as pause
    IDR_PNG_menu_icon_stop,
    IDR_PNG_menu_icon_resume,
    IDR_PNG_menu_icon_resume,
    IDR_PNG_menu_icon_resume, //reheating is same as resume, bud disabled
    IDR_PNG_menu_icon_reprint,
    IDR_PNG_menu_icon_home,
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

struct screen_printing_data_t {
    window_frame_t root;

    window_header_t header;
    window_text_t w_filename;
    window_progress_t w_progress;
    window_text_t w_time_label;
    window_text_t w_time_value;
    window_text_t w_etime_label;
    window_text_t w_etime_value;

    window_icon_t w_buttons[3];
    window_text_t w_labels[3];

    status_footer_t footer;

    uint32_t last_print_duration;
    uint32_t last_time_to_end;

    std::array<char, MAX_TIMEDUR_STR_SIZE> text_time_dur;
    std::array<char, MAX_END_TIMESTAMP_SIZE> text_etime;
    //std::array<char, 15> label_etime;  // "Remaining Time" or "Print will end" // nope, if you have only 2 static const strings, you can swap pointers
    string_view_utf8 label_etime;      // not sure if we really must keep this in memory
    std::array<char, 5> text_filament; // 999m\0 | 1.2m\0

    window_text_t w_message; //Messages from onStatusChanged()
    uint32_t message_timer;
    bool message_flag;
    bool stop_pressed;
    printing_state_t state__readonly__use_change_print_state;
    uint8_t last_sd_percent_done;
};

void screen_printing_init(screen_t *screen);
void screen_printing_done(screen_t *screen);
void screen_printing_draw(screen_t *screen);
int screen_printing_event(screen_t *screen, window_t *window, uint8_t event, void *param);

#define pw ((screen_printing_data_t *)screen->pdata)

static void invalidate_print_state(screen_t *screen) {
    pw->state__readonly__use_change_print_state = printing_state_t::COUNT;
}
static printing_state_t get_state(screen_t *screen) {
    return pw->state__readonly__use_change_print_state;
}

static void screen_printing_reprint(screen_t *screen);
static void change_print_state(screen_t *screen);
static void update_progress(screen_t *screen, uint8_t percent, uint16_t print_speed);
static void update_remaining_time(screen_t *screen, time_t rawtime, uint16_t print_speed);
static void update_end_timestamp(screen_t *screen, time_t now_sec, uint16_t print_speed);
static void update_print_duration(screen_t *screen, time_t print_duration);
static void set_pause_icon_and_label(screen_t *screen);

screen_t screen_printing = {
    0,
    0,
    screen_printing_init,
    screen_printing_done,
    screen_printing_draw,
    screen_printing_event,
    sizeof(screen_printing_data_t), //data_size
    0,                              //pdata
};
screen_t *const get_scr_printing() { return &screen_printing; }

void screen_printing_init(screen_t *screen) {
    marlin_error_clr(MARLIN_ERR_ProbingFailed);

    pw->stop_pressed = false;
    marlin_vars_t *vars = marlin_vars();

    strlcpy(pw->text_time_dur.data(), "0m", pw->text_time_dur.size());
    strlcpy(pw->text_filament.data(), "999m", pw->text_filament.size());

    int16_t root = window_create_ptr(WINDOW_CLS_FRAME, -1,
        rect_ui16(0, 0, 0, 0),
        &(pw->root));

    window_create_ptr(WINDOW_CLS_HEADER, root, gui_defaults.header_sz, &(pw->header));
    p_window_header_set_icon(&(pw->header), IDR_PNG_status_icon_printing);
#ifndef DEBUG_FSENSOR_IN_HEADER
    static const char pr[] = "PRINTING";
    p_window_header_set_text(&(pw->header), _(pr));
#endif
    window_create_ptr(WINDOW_CLS_TEXT, root,
        rect_ui16(10, 33, 220, 29),
        &(pw->w_filename));
    pw->w_filename.font = resource_font(IDR_FNT_BIG);
    pw->w_filename.SetPadding(padding_ui8(0, 0, 0, 0));
    pw->w_filename.SetAlignment(ALIGN_LEFT_BOTTOM);
    // this MakeRAM is safe - vars->media_LFN is statically allocated (even though it may not be obvious at the first look)
    pw->w_filename.SetText(vars->media_LFN ? string_view_utf8::MakeRAM((const uint8_t *)vars->media_LFN) : string_view_utf8::MakeNULLSTR());

    window_create_ptr(WINDOW_CLS_PROGRESS, root,
        rect_ui16(10, 70, 220, 50),
        &(pw->w_progress));
    pw->w_progress.color_progress = COLOR_ORANGE;
    pw->w_progress.font = resource_font(IDR_FNT_BIG);
    pw->w_progress.height_progress = 14;

    window_create_ptr(WINDOW_CLS_TEXT, root,
        rect_ui16(130, 128, 101, 20),
        &(pw->w_etime_label));
    pw->w_etime_label.font = resource_font(IDR_FNT_SMALL);
    pw->w_etime_label.SetAlignment(ALIGN_RIGHT_BOTTOM);
    pw->w_etime_label.SetPadding(padding_ui8(0, 2, 0, 2));
    pw->w_etime_label.SetText(_("Remaining Time"));

    window_create_ptr(WINDOW_CLS_TEXT, root,
        rect_ui16(30, 148, 201, 20),
        &(pw->w_etime_value));
    pw->w_etime_value.font = resource_font(IDR_FNT_SMALL);
    pw->w_etime_value.SetAlignment(ALIGN_RIGHT_BOTTOM);
    pw->w_etime_value.SetPadding(padding_ui8(0, 2, 0, 2));
    // this MakeRAM is safe - text_etime is allocated in RAM for the lifetime of pw
    pw->w_etime_value.SetText(string_view_utf8::MakeRAM((const uint8_t *)pw->text_etime.data()));

    window_create_ptr(WINDOW_CLS_TEXT, root,
        rect_ui16(10, 128, 101, 20),
        &(pw->w_time_label));
    pw->w_time_label.font = resource_font(IDR_FNT_SMALL);
    pw->w_time_label.SetAlignment(ALIGN_RIGHT_BOTTOM);
    pw->w_time_label.SetPadding(padding_ui8(0, 2, 0, 2));
    pw->w_time_label.SetText(_("Printing time"));

    window_create_ptr(WINDOW_CLS_TEXT, root,
        rect_ui16(10, 148, 101, 20),
        &(pw->w_time_value));
    pw->w_time_value.font = resource_font(IDR_FNT_SMALL);
    pw->w_time_value.SetAlignment(ALIGN_RIGHT_BOTTOM);
    pw->w_time_value.SetPadding(padding_ui8(0, 2, 0, 2));
    // this MakeRAM is safe - text_time_dur is allocated in RAM for the lifetime of pw
    pw->w_time_value.SetText(string_view_utf8::MakeRAM((const uint8_t *)pw->text_time_dur.data()));

    window_create_ptr(WINDOW_CLS_TEXT, root,
        rect_ui16(10, 75, 230, 95),
        &(pw->w_message));
    pw->w_message.font = resource_font(IDR_FNT_NORMAL);
    pw->w_message.SetAlignment(ALIGN_LEFT_TOP);
    pw->w_message.SetPadding(padding_ui8(0, 2, 0, 2));
    pw->w_message.SetText(_("No messages"));
    pw->w_message.Hide();
    pw->message_flag = false;

    // buttons
    const uint16_t icon_y = gui_defaults.footer_sz.y - gui_defaults.padding.bottom - 22 - 64;
    const uint16_t text_y = gui_defaults.footer_sz.y - gui_defaults.padding.bottom - 22;
    for (uint8_t col = 0; col < 3; col++) {
        window_create_ptr(
            WINDOW_CLS_ICON, root,
            rect_ui16(8 + (15 + 64) * col, icon_y, 64, 64),
            &(pw->w_buttons[col]));
        //pw->w_buttons[col].SetBackColor(COLOR_GRAY); //this did not work before, do we want it?
        pw->w_buttons[col].SetTag(col + 1);
        pw->w_buttons[col].Enable();

        window_create_ptr(
            WINDOW_CLS_TEXT, root,
            rect_ui16(80 * col, text_y, 80, 22),
            &(pw->w_labels[col]));
        pw->w_labels[col].font = resource_font(IDR_FNT_SMALL);
        pw->w_labels[col].SetPadding(padding_ui8(0, 0, 0, 0));
        pw->w_labels[col].SetAlignment(ALIGN_CENTER);
    }

    invalidate_print_state(screen); //must invalidate, to ensure correct draw of buttons

    pw->last_print_duration = -1;
    pw->last_time_to_end = -1;
    pw->last_sd_percent_done = -1;

    status_footer_init(&(pw->footer), root);
}

void screen_printing_done(screen_t *screen) {
    window_destroy(pw->root.id);
}

void screen_printing_draw(screen_t *screen) {
}

static void open_popup_message(screen_t *screen) {
    pw->w_etime_label.Hide();
    pw->w_etime_value.Hide();
    pw->w_progress.Hide();
    pw->w_time_label.Hide();
    pw->w_time_value.Hide();

    // this MakeRAM is safe - msg stack and its items are allocated in RAM for the lifetime of pw
    pw->w_message.SetText(string_view_utf8::MakeRAM((const uint8_t *)msg_stack.msg_data[0]));

    pw->w_message.Show();
    pw->message_timer = HAL_GetTick();
    pw->message_flag = true;
}

static void close_popup_message(screen_t *screen) {
    pw->w_etime_label.Show();
    pw->w_etime_value.Show();
    pw->w_progress.Show();
    pw->w_time_label.Show();
    pw->w_time_value.Show();

    pw->w_message.SetText(string_view_utf8::MakeNULLSTR());

    pw->w_message.Hide();
    pw->message_flag = false;
}

#ifdef DEBUG_FSENSOR_IN_HEADER
extern int _is_in_M600_flg;
extern uint32_t *pCommand;
#endif

int screen_printing_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
#ifdef DEBUG_FSENSOR_IN_HEADER
    static int _last = 0;
    if (HAL_GetTick() - _last > 300) {
        _last = HAL_GetTick();

        static char buff[] = "Sx Mx x xxxx";                         //"x"s are replaced
        buff[1] = fs_get_state() + '0';                              // S0 init, S1 has filament, S2 nofilament, S3 not connected, S4 disabled
        buff[4] = fs_get_send_M600_on();                             // Me edge, Ml level, Mn never, Mx undefined
        buff[6] = fs_was_M600_send() ? 's' : 'n';                    // s == send, n== not send
        buff[8] = _is_in_M600_flg ? 'M' : '0';                       // M == marlin is doing M600
        buff[9] = marlin_event(MARLIN_EVT_CommandBegin) ? 'B' : '0'; // B == Event begin
        buff[10] = marlin_command() == MARLIN_CMD_M600 ? 'C' : '0';  // C == Command M600
        buff[11] = *pCommand == MARLIN_CMD_M600 ? 's' : '0';         // s == server - Command M600
        p_window_header_set_text(&(pw->header), buff);
    }

#endif

    if (event == WINDOW_EVENT_MESSAGE && msg_stack.count > 0) {
        open_popup_message(screen);
        return 0;
    }

    if ((!is_abort_state(marlin_vars()->print_state)) && pw->message_flag && (HAL_GetTick() - pw->message_timer >= POPUP_MSG_DUR_MS)) {
        close_popup_message(screen);
    }

    if (status_footer_event(&(pw->footer), window, event, param)) {
        return 1;
    }

    if ((pw->state__readonly__use_change_print_state == printing_state_t::PRINTED) && marlin_error(MARLIN_ERR_ProbingFailed)) {
        marlin_error_clr(MARLIN_ERR_ProbingFailed);
        if (gui_msgbox(_("Bed leveling failed. Try again?"), MSGBOX_BTN_YESNO) == MSGBOX_RES_YES) {
            screen_printing_reprint(screen);
        } else {
            screen_close();
            return 1;
        }
    }

    change_print_state(screen);

    if (marlin_vars()->print_duration != pw->last_print_duration)
        update_print_duration(screen, marlin_vars()->print_duration);
    if (marlin_vars()->time_to_end != pw->last_time_to_end) {
        time_t sec = sntp_get_system_time();
        if (sec != 0) {
            // store string_view_utf8 for later use - should be safe, we get some static string from flash, no need to copy it into RAM
            // theoretically it can be removed completely in case the string is constant for the whole run of the screen
            pw->w_etime_label.SetText(pw->label_etime = _("Print will end"));
            update_end_timestamp(screen, sec, marlin_vars()->print_speed);
        } else {
            // store string_view_utf8 for later use - should be safe, we get some static string from flash, no need to copy it into RAM
            pw->w_etime_label.SetText(pw->label_etime = _("Remaining Time"));
            update_remaining_time(screen, marlin_vars()->time_to_end, marlin_vars()->print_speed);
        }
        pw->last_time_to_end = marlin_vars()->time_to_end;
    }
    if (marlin_vars()->sd_percent_done != pw->last_sd_percent_done)
        update_progress(screen, marlin_vars()->sd_percent_done, marlin_vars()->print_speed);

    /// -- close screen when print is done / stopped and USB media is removed
    if (!marlin_vars()->media_inserted && get_state(screen) == printing_state_t::PRINTED) {
        screen_close();
        return 1;
    }

    /// -- check when media is or isn't inserted
    if (p_window_header_event_clr(&(pw->header), MARLIN_EVT_MediaRemoved) || p_window_header_event_clr(&(pw->header), MARLIN_EVT_MediaInserted)) {
        /// -- check for enable/disable resume button
        set_pause_icon_and_label(screen);
    }

    if (event != WINDOW_EVENT_CLICK) {
        return 0;
    }

    int pi = reinterpret_cast<int>(param) - 1;
    // -- pressed button is disabled - dont propagate event further
    if (pw->w_buttons[pi].f_disabled) {
        return 0;
    }

    switch (static_cast<Btn>(pi)) {
    case Btn::Tune:
        switch (get_state(screen)) {
        case printing_state_t::PRINTING:
        case printing_state_t::PAUSED:
            screen_open(get_scr_menu_tune()->id);
            break;
        default:
            break;
        }
        return 1;
        break;
    case Btn::Pause: {
        switch (get_state(screen)) {
        case printing_state_t::PRINTING:
            marlin_print_pause();
            break;
        case printing_state_t::PAUSED:
            marlin_print_resume();
            break;
        case printing_state_t::PRINTED:
            screen_printing_reprint(screen);
            break;
        default:
            break;
        }
        break;
    }
    case Btn::Stop:
        switch (get_state(screen)) {
        case printing_state_t::PRINTED:
            screen_close();
            return 1;
        case printing_state_t::PAUSING:
        case printing_state_t::RESUMING:
            return 0;
        default: {
            if (gui_msgbox(_("Are you sure to stop this printing?"),
                    MSGBOX_BTN_YESNO | MSGBOX_ICO_WARNING | MSGBOX_DEF_BUTTON1)
                == MSGBOX_RES_YES) {
                pw->stop_pressed = true;
                change_print_state(screen);
                marlin_print_abort();
            } else
                return 0;
        }
        }
        break;
    }
    return 0;
}

static void disable_tune_button(screen_t *screen) {
    window_icon_t *p_button = &pw->w_buttons[static_cast<size_t>(Btn::Tune)];
    p_button->f_disabled = 1;
    p_button->f_enabled = 0; // can't be focused

    // move to reprint when tune is focused
    if (p_button->IsFocused()) {
        window_ptr(pw->w_buttons[static_cast<size_t>(Btn::Pause)].id)->SetFocus();
    }
    p_button->Invalidate();
}

static void enable_tune_button(screen_t *screen) {
    window_icon_t *p_button = &pw->w_buttons[static_cast<size_t>(Btn::Tune)];

    p_button->f_disabled = 0;
    p_button->f_enabled = 1; // can be focused
    p_button->Invalidate();
}

static void update_progress(screen_t *screen, uint8_t percent, uint16_t print_speed) {
    pw->w_progress.color_text = (percent <= 100) && (print_speed == 100) ? COLOR_VALUE_VALID : COLOR_VALUE_INVALID;
    pw->w_progress.SetValue(percent);
}

static void update_remaining_time(screen_t *screen, time_t rawtime, uint16_t print_speed) {
    pw->w_etime_value.color_text = rawtime != time_t(-1) ? COLOR_VALUE_VALID : COLOR_VALUE_INVALID;
    if (rawtime != time_t(-1)) {
        if (print_speed != 100)
            // multiply by 100 is safe, it limits time_to_end to ~21mil. seconds (248 days)
            rawtime = (rawtime * 100) / print_speed;
        const struct tm *timeinfo = localtime(&rawtime);
        //standard would be:
        //strftime(array.data(), array.size(), "%jd %Hh", timeinfo);
        if (timeinfo->tm_yday) {
            snprintf(pw->text_etime.data(), MAX_END_TIMESTAMP_SIZE, "%id %2ih", timeinfo->tm_yday, timeinfo->tm_hour);
        } else if (timeinfo->tm_hour) {
            snprintf(pw->text_etime.data(), MAX_END_TIMESTAMP_SIZE, "%ih %2im", timeinfo->tm_hour, timeinfo->tm_min);
        } else {
            snprintf(pw->text_etime.data(), MAX_END_TIMESTAMP_SIZE, "%im", timeinfo->tm_min);
        }
    } else {
        if (print_speed != 100)
            strlcat(pw->text_etime.data(), "?", MAX_END_TIMESTAMP_SIZE);
        else
            strlcpy(pw->text_etime.data(), "N/A", MAX_END_TIMESTAMP_SIZE);
    }
    // this MakeRAM is safe - text_etime is allocated in RAM for the lifetime of pw
    pw->w_etime_value.SetText(string_view_utf8::MakeRAM((const uint8_t *)pw->text_etime.data()));
}

static void update_end_timestamp(screen_t *screen, time_t now_sec, uint16_t print_speed) {

    bool time_invalid = false;
    if (marlin_vars()->time_to_end == TIME_TO_END_INVALID) {
        pw->w_etime_value.color_text = COLOR_VALUE_INVALID;
        time_invalid = true;
    } else {
        pw->w_etime_value.color_text = COLOR_VALUE_VALID;
    }

    static const uint32_t full_day_in_seconds = 86400;
    time_t print_end_sec, tommorow_sec;

    if (print_speed != 100)
        // multiply by 100 is safe, it limits time_to_end to ~21mil. seconds (248 days)
        print_end_sec = now_sec + (100 * marlin_vars()->time_to_end / print_speed);
    else
        print_end_sec = now_sec + marlin_vars()->time_to_end;

    tommorow_sec = now_sec + full_day_in_seconds;

    struct tm tommorow, print_end, now;
    localtime_r(&now_sec, &now);
    localtime_r(&tommorow_sec, &tommorow);
    localtime_r(&print_end_sec, &print_end);

    if (now.tm_mday == print_end.tm_mday && // if print end is today
        now.tm_mon == print_end.tm_mon && now.tm_year == print_end.tm_year) {
        //strftime(pw->text_etime.data(), MAX_END_TIMESTAMP_SIZE, "Today at %H:%M?", &print_end); //@@TODO translate somehow
        FormatMsgPrintWillEnd::Today(pw->text_etime.data(), MAX_END_TIMESTAMP_SIZE, &print_end, true);
    } else if (tommorow.tm_mday == print_end.tm_mday && // if print end is tommorow
        tommorow.tm_mon == print_end.tm_mon && tommorow.tm_year == print_end.tm_year) {
        //        strftime(pw->text_etime.data(), MAX_END_TIMESTAMP_SIZE, "%a at %H:%MM", &print_end);
        FormatMsgPrintWillEnd::DayOfWeek(pw->text_etime.data(), MAX_END_TIMESTAMP_SIZE, &print_end, true);
    } else {
        //        strftime(pw->text_etime.data(), MAX_END_TIMESTAMP_SIZE, "%m-%d at %H:%MM", &print_end);
        FormatMsgPrintWillEnd::Date(pw->text_etime.data(), MAX_END_TIMESTAMP_SIZE, &print_end, true, FormatMsgPrintWillEnd::ISO);
    }
    if (print_speed != 100)
        strlcat(pw->text_etime.data(), "?", MAX_END_TIMESTAMP_SIZE);

    if (time_invalid == false) {
        pw->text_etime[pw->text_etime.size() - 1] = 0; // safety \0 termination in all cases
    }
    // this MakeRAM is safe - text_etime is allocated in RAM for the lifetime of pw
    pw->w_etime_value.SetText(string_view_utf8::MakeRAM((const uint8_t *)pw->text_etime.data()));
}
static void update_print_duration(screen_t *screen, time_t rawtime) {
    pw->w_time_value.color_text = COLOR_VALUE_VALID;
    const struct tm *timeinfo = localtime(&rawtime);
    if (timeinfo->tm_yday) {
        snprintf(pw->text_time_dur.data(), MAX_TIMEDUR_STR_SIZE, "%id %2ih", timeinfo->tm_yday, timeinfo->tm_hour);
    } else if (timeinfo->tm_hour) {
        snprintf(pw->text_time_dur.data(), MAX_TIMEDUR_STR_SIZE, "%ih %2im", timeinfo->tm_hour, timeinfo->tm_min);
    } else if (timeinfo->tm_min) {
        snprintf(pw->text_time_dur.data(), MAX_TIMEDUR_STR_SIZE, "%im %2is", timeinfo->tm_min, timeinfo->tm_sec);
    } else {
        snprintf(pw->text_time_dur.data(), MAX_TIMEDUR_STR_SIZE, "%is", timeinfo->tm_sec);
    }
    // this MakeRAM is safe - text_time_dur is allocated in RAM for the lifetime of pw
    pw->w_time_value.SetText(string_view_utf8::MakeRAM((const uint8_t *)pw->text_time_dur.data()));
}

static void screen_printing_reprint(screen_t *screen) {
    print_begin(marlin_vars()->media_SFN_path);
    pw->w_etime_label.SetText(_("Remaining Time"));
    pw->w_labels[static_cast<size_t>(Btn::Stop)].SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)printing_labels[static_cast<size_t>(item_id_t::stop)]));
    pw->w_buttons[static_cast<size_t>(Btn::Stop)].SetIdRes(printing_icons[static_cast<size_t>(item_id_t::stop)]);

#ifndef DEBUG_FSENSOR_IN_HEADER
    p_window_header_set_text(&(pw->header), _("PRINTING"));
#endif
}

//todo use it
/*static void mesh_err_stop_print(screen_t *screen) {
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

static void set_icon_and_label(item_id_t id_to_set, window_icon_t *p_button, window_text_t *lbl) {
    size_t index = static_cast<size_t>(id_to_set);
    if (p_button->GetIdRes() != printing_icons[index])
        p_button->SetIdRes(printing_icons[index]);
    lbl->SetText(_(printing_labels[index]));
}

static void enable_button(window_icon_t *p_button) {
    if (p_button->f_disabled) {
        p_button->f_disabled = 0;
        p_button->f_enabled = 1;
        p_button->Invalidate();
    }
}

static void disable_button(window_icon_t *p_button) {
    if (!p_button->f_disabled) {
        p_button->f_disabled = 1;
        p_button->f_enabled = 0;
        p_button->Invalidate();
    }
}

static void set_pause_icon_and_label(screen_t *screen) {
    window_icon_t *p_button = &pw->w_buttons[static_cast<size_t>(Btn::Pause)];
    window_text_t *pLabel = &pw->w_labels[static_cast<size_t>(Btn::Pause)];

    //todo it is static, because menu tune is not dialog
    //switch (pw->state__readonly__use_change_print_state)
    switch (get_state(screen)) {
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

void set_tune_icon_and_label(screen_t *screen) {
    window_icon_t *p_button = &pw->w_buttons[static_cast<size_t>(Btn::Tune)];
    window_text_t *pLabel = &pw->w_labels[static_cast<size_t>(Btn::Tune)];

    //must be before switch
    set_icon_and_label(item_id_t::settings, p_button, pLabel);

    switch (get_state(screen)) {
    case printing_state_t::PRINTING:
        // case printing_state_t::PAUSED:
        enable_tune_button(screen);
        break;
    case printing_state_t::ABORTING:
        disable_button(p_button);
        break;
    default:
        disable_tune_button(screen);
        break;
    }
}

void set_stop_icon_and_label(screen_t *screen) {
    window_icon_t *p_button = &pw->w_buttons[static_cast<size_t>(Btn::Stop)];
    window_text_t *pLabel = &pw->w_labels[static_cast<size_t>(Btn::Stop)];

    switch (get_state(screen)) {
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

static void change_print_state(screen_t *screen) {
    printing_state_t st = printing_state_t::COUNT;

    switch (marlin_vars()->print_state) {
    case mpsIdle:
        st = printing_state_t::INITIAL;
        break;
    case mpsPrinting:
        st = printing_state_t::PRINTING;
        break;
    case mpsPaused:
        pw->stop_pressed = false;
        st = printing_state_t::PAUSED;
        break;
    case mpsPausing_Begin:
    case mpsPausing_WaitIdle:
    case mpsPausing_ParkHead:
        st = printing_state_t::PAUSING;
        break;
    case mpsResuming_Reheating:
        pw->stop_pressed = false;
        st = printing_state_t::REHEATING;
        break;
    case mpsResuming_Begin:
    case mpsResuming_UnparkHead:
        pw->stop_pressed = false;
        st = printing_state_t::RESUMING;
        break;
    case mpsAborting_Begin:
    case mpsAborting_WaitIdle:
    case mpsAborting_ParkHead:
        pw->stop_pressed = false;
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
    if (pw->stop_pressed) {
        st = printing_state_t::ABORTING;
    }
    if (pw->state__readonly__use_change_print_state != st) {
        pw->state__readonly__use_change_print_state = st;
        set_pause_icon_and_label(screen);
        set_tune_icon_and_label(screen);
        set_stop_icon_and_label(screen);
    }
}

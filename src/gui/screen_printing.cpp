#include "dbg.h"
#include "gui.h"
#include "config.h"
#include "window_header.h"
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
    "Tune",
    "Pause",
    "Pausing...",
    "Stop",
    "Resume",
    "Resuming...",
    "Heating...",
    "Reprint",
    "Home",
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
    std::array<char, 15> label_etime;  // "Remaining Time" or "Print will end"
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
//static void mesh_err_stop_print(screen_t *screen); //todo use it
static void change_print_state(screen_t *screen);
static void update_progress(screen_t *screen, uint8_t percent, uint16_t print_speed);
static void update_remaining_time(screen_t *screen, time_t rawtime);
static void update_end_timestamp(screen_t *screen, time_t now_sec);
static void update_print_duration(screen_t *screen, time_t print_duration);

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
    int16_t id;

    pw->stop_pressed = false;
    marlin_vars_t *vars = marlin_vars();

    strlcpy(pw->text_time_dur.data(), "0m", pw->text_time_dur.size());
    strlcpy(pw->text_filament.data(), "999m", pw->text_filament.size());

    int16_t root = window_create_ptr(WINDOW_CLS_FRAME, -1,
        rect_ui16(0, 0, 0, 0),
        &(pw->root));

    id = window_create_ptr(WINDOW_CLS_HEADER, root, gui_defaults.header_sz, &(pw->header));
    p_window_header_set_icon(&(pw->header), IDR_PNG_status_icon_printing);
#ifndef DEBUG_FSENSOR_IN_HEADER
    p_window_header_set_text(&(pw->header), "PRINTING");
#endif
    id = window_create_ptr(WINDOW_CLS_TEXT, root,
        rect_ui16(10, 33, 220, 29),
        &(pw->w_filename));
    pw->w_filename.font = resource_font(IDR_FNT_BIG);
    window_set_padding(id, padding_ui8(0, 0, 0, 0));
    window_set_alignment(id, ALIGN_LEFT_BOTTOM);
    window_set_text(id, vars->media_LFN ? vars->media_LFN : "");

    id = window_create_ptr(WINDOW_CLS_PROGRESS, root,
        rect_ui16(10, 70, 220, 50),
        &(pw->w_progress));
    pw->w_progress.color_progress = COLOR_ORANGE;
    pw->w_progress.font = resource_font(IDR_FNT_BIG);
    pw->w_progress.height_progress = 14;

    id = window_create_ptr(WINDOW_CLS_TEXT, root,
        rect_ui16(130, 128, 101, 20),
        &(pw->w_etime_label));
    pw->w_etime_label.font = resource_font(IDR_FNT_SMALL);
    window_set_alignment(id, ALIGN_RIGHT_BOTTOM);
    window_set_padding(id, padding_ui8(0, 2, 0, 2));
    strlcpy(pw->label_etime.data(), _("Remaining Time"), 15);
    window_set_text(id, pw->label_etime.data());

    id = window_create_ptr(WINDOW_CLS_TEXT, root,
        rect_ui16(30, 148, 201, 20),
        &(pw->w_etime_value));
    pw->w_etime_value.font = resource_font(IDR_FNT_SMALL);
    window_set_alignment(id, ALIGN_RIGHT_BOTTOM);
    window_set_padding(id, padding_ui8(0, 2, 0, 2));
    window_set_text(id, pw->text_etime.data());

    id = window_create_ptr(WINDOW_CLS_TEXT, root,
        rect_ui16(10, 128, 101, 20),
        &(pw->w_time_label));
    pw->w_time_label.font = resource_font(IDR_FNT_SMALL);
    window_set_alignment(id, ALIGN_RIGHT_BOTTOM);
    window_set_padding(id, padding_ui8(0, 2, 0, 2));
    window_set_text(id, _("Printing time"));

    id = window_create_ptr(WINDOW_CLS_TEXT, root,
        rect_ui16(10, 148, 101, 20),
        &(pw->w_time_value));
    pw->w_time_value.font = resource_font(IDR_FNT_SMALL);
    window_set_alignment(id, ALIGN_RIGHT_BOTTOM);
    window_set_padding(id, padding_ui8(0, 2, 0, 2));
    window_set_text(id, pw->text_time_dur.data());

    id = window_create_ptr(WINDOW_CLS_TEXT, root,
        rect_ui16(10, 75, 230, 95),
        &(pw->w_message));
    pw->w_time_value.font = resource_font(IDR_FNT_SMALL);
    window_set_alignment(id, ALIGN_LEFT_TOP);
    window_set_padding(id, padding_ui8(0, 2, 0, 2));
    window_set_text(id, "No messages");
    window_hide(id);
    pw->message_flag = false;

    // buttons
    const uint16_t icon_y = gui_defaults.footer_sz.y - gui_defaults.padding.bottom - 22 - 64;
    const uint16_t text_y = gui_defaults.footer_sz.y - gui_defaults.padding.bottom - 22;
    for (uint8_t col = 0; col < 3; col++) {
        id = window_create_ptr(
            WINDOW_CLS_ICON, root,
            rect_ui16(8 + (15 + 64) * col, icon_y, 64, 64),
            &(pw->w_buttons[col]));
        window_set_color_back(id, COLOR_GRAY);
        window_set_tag(id, col + 1);
        window_enable(id);

        id = window_create_ptr(
            WINDOW_CLS_TEXT, root,
            rect_ui16(80 * col, text_y, 80, 22),
            &(pw->w_labels[col]));
        pw->w_labels[col].font = resource_font(IDR_FNT_SMALL);
        window_set_padding(id, padding_ui8(0, 0, 0, 0));
        window_set_alignment(id, ALIGN_CENTER);
    }

    invalidate_print_state(screen); //must invalidate, to ensure correct draw of buttons

    pw->last_print_duration = -1;
    pw->last_time_to_end = -1;
    pw->last_sd_percent_done = -1;

    status_footer_init(&(pw->footer), root);
}

void screen_printing_done(screen_t *screen) {
    window_destroy(pw->root.win.id);
}

void screen_printing_draw(screen_t *screen) {
}

static void open_popup_message(screen_t *screen) {
    window_hide(pw->w_etime_label.win.id);
    window_hide(pw->w_etime_value.win.id);
    window_hide(pw->w_progress.win.id);
    window_hide(pw->w_time_label.win.id);
    window_hide(pw->w_time_value.win.id);

    window_set_text(pw->w_message.win.id, msg_stack.msg_data[0]);

    window_show(pw->w_message.win.id);
    pw->message_timer = HAL_GetTick();
    pw->message_flag = true;
}

static void close_popup_message(screen_t *screen) {
    window_show(pw->w_etime_label.win.id);
    window_show(pw->w_etime_value.win.id);
    window_show(pw->w_progress.win.id);
    window_show(pw->w_time_label.win.id);
    window_show(pw->w_time_value.win.id);

    window_set_text(pw->w_message.win.id, "");

    window_hide(pw->w_message.win.id);
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

    change_print_state(screen);

    if (marlin_vars()->print_duration != pw->last_print_duration)
        update_print_duration(screen, marlin_vars()->print_duration);
    if (marlin_vars()->time_to_end != pw->last_time_to_end) {
        time_t sec = sntp_get_system_time();
        if (sec != 0) {
            strlcpy(pw->label_etime.data(), _("Print will end"), 15);
            window_set_text(pw->w_etime_label.win.id, pw->label_etime.data());
            update_end_timestamp(screen, sec);
        } else {
            strlcpy(pw->label_etime.data(), _("Remaining Time"), 15);
            window_set_text(pw->w_etime_label.win.id, pw->label_etime.data());
            update_remaining_time(screen, marlin_vars()->time_to_end);
        }
        pw->last_time_to_end = marlin_vars()->time_to_end;
    }
    if (marlin_vars()->sd_percent_done != pw->last_sd_percent_done)
        update_progress(screen, marlin_vars()->sd_percent_done, marlin_vars()->print_speed);

    if (p_window_header_event_clr(&(pw->header), MARLIN_EVT_MediaRemoved) && get_state(screen) == printing_state_t::PRINTED) {
        screen_close();
    }

    if (event != WINDOW_EVENT_CLICK) {
        return 0;
    }

    int pi = reinterpret_cast<int>(param) - 1;
    // -- pressed button is disabled - dont propagate event further
    if (pw->w_buttons[pi].win.f_disabled) {
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
    p_button->win.f_disabled = 1;
    p_button->win.f_enabled = 0; // can't be focused

    // move to reprint when tune is focused
    if (window_is_focused(p_button->win.id)) {
        window_set_focus(pw->w_buttons[static_cast<size_t>(Btn::Pause)].win.id);
    }
    window_invalidate(p_button->win.id);
}

static void enable_tune_button(screen_t *screen) {
    window_icon_t *p_button = &pw->w_buttons[static_cast<size_t>(Btn::Tune)];

    p_button->win.f_disabled = 0;
    p_button->win.f_enabled = 1; // can be focused
    window_invalidate(p_button->win.id);
}

static void update_progress(screen_t *screen, uint8_t percent, uint16_t print_speed) {
    pw->w_progress.color_text = (percent <= 100) && (print_speed == 100) ? COLOR_VALUE_VALID : COLOR_VALUE_INVALID;
    window_set_value(pw->w_progress.win.id, percent);
}

static void update_remaining_time(screen_t *screen, time_t rawtime) {
    pw->w_etime_value.color_text = rawtime != time_t(-1) ? COLOR_VALUE_VALID : COLOR_VALUE_INVALID;

    if (rawtime != time_t(-1)) {
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
    } else
        strlcpy(pw->text_etime.data(), "N/A", MAX_END_TIMESTAMP_SIZE);

    window_set_text(pw->w_etime_value.win.id, pw->text_etime.data());
}

static void update_end_timestamp(screen_t *screen, time_t now_sec) {

    bool time_invalid = false;
    if (marlin_vars()->time_to_end == TIME_TO_END_INVALID) {
        pw->w_etime_value.color_text = COLOR_VALUE_INVALID;
        time_invalid = true;
    } else {
        pw->w_etime_value.color_text = COLOR_VALUE_VALID;
    }

    static const uint32_t full_day_in_seconds = 86400;
    time_t print_end_sec, tommorow_sec;

    print_end_sec = now_sec + marlin_vars()->time_to_end;
    tommorow_sec = now_sec + full_day_in_seconds;

    struct tm tommorow, print_end, now;
    localtime_r(&now_sec, &now);
    localtime_r(&tommorow_sec, &tommorow);
    localtime_r(&print_end_sec, &print_end);

    if (now.tm_mday == print_end.tm_mday && // if print end is today
        now.tm_mon == print_end.tm_mon && now.tm_year == print_end.tm_year) {
        strftime(pw->text_etime.data(), MAX_END_TIMESTAMP_SIZE, "Today at %H:%M?", &print_end);
    } else if (tommorow.tm_mday == print_end.tm_mday && // if print end is tommorow
        tommorow.tm_mon == print_end.tm_mon && tommorow.tm_year == print_end.tm_year) {
        strftime(pw->text_etime.data(), MAX_END_TIMESTAMP_SIZE, "Tommorow at %H:%M?", &print_end);
    } else {
        strftime(pw->text_etime.data(), MAX_END_TIMESTAMP_SIZE, "%m-%d at %H:%M?", &print_end);
    }

    if (time_invalid == false) {
        uint8_t length = strlen(pw->text_etime.data());
        if (length > 0) {
            pw->text_etime[length - 1] = 0;
        }
    }

    window_set_text(pw->w_etime_value.win.id, pw->text_etime.data());
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
    window_set_text(pw->w_time_value.win.id, pw->text_time_dur.data());
}

static void screen_printing_reprint(screen_t *screen) {
    print_begin(marlin_vars()->media_SFN_path);
    window_set_text(pw->w_etime_label.win.id, PSTR("Remaining Time")); // !!! "screen_printing_init()" is not invoked !!!

    window_set_text(pw->w_labels[static_cast<size_t>(Btn::Stop)].win.id, printing_labels[static_cast<size_t>(item_id_t::stop)]);
    window_set_icon_id(pw->w_buttons[static_cast<size_t>(Btn::Stop)].win.id, printing_icons[static_cast<size_t>(item_id_t::stop)]);

#ifndef DEBUG_FSENSOR_IN_HEADER
    p_window_header_set_text(&(pw->header), "PRINTING");
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

static void set_icon_and_label(item_id_t id_to_set, int16_t btn_id, int16_t lbl_id) {
    size_t index = static_cast<size_t>(id_to_set);
    if (window_get_icon_id(btn_id) != printing_icons[index])
        window_set_icon_id(btn_id, printing_icons[index]);
    //compare pointers to text, compare texts would take too long
    if (window_get_text(lbl_id) != printing_labels[index])
        window_set_text(lbl_id, printing_labels[index]);
}

static void enable_button(window_icon_t *p_button) {
    if (p_button->win.f_disabled) {
        p_button->win.f_disabled = 0;
        window_invalidate(p_button->win.id);
    }
}

static void disable_button(window_icon_t *p_button) {
    if (!p_button->win.f_disabled) {
        p_button->win.f_disabled = 1;
        window_invalidate(p_button->win.id);
    }
}

static void set_pause_icon_and_label(screen_t *screen) {
    window_icon_t *p_button = &pw->w_buttons[static_cast<size_t>(Btn::Pause)];
    int16_t btn_id = p_button->win.id;
    int16_t lbl_id = pw->w_labels[static_cast<size_t>(Btn::Pause)].win.id;

    //todo it is static, because menu tune is not dialog
    //switch (pw->state__readonly__use_change_print_state)
    switch (get_state(screen)) {
    case printing_state_t::COUNT:
    case printing_state_t::INITIAL:
    case printing_state_t::PRINTING:
    case printing_state_t::MBL_FAILED:
        enable_button(p_button);
        set_icon_and_label(item_id_t::pause, btn_id, lbl_id);
        break;
    case printing_state_t::PAUSING:
        disable_button(p_button);
        set_icon_and_label(item_id_t::pausing, btn_id, lbl_id);
        break;
    case printing_state_t::PAUSED:
        enable_button(p_button);
        set_icon_and_label(item_id_t::resume, btn_id, lbl_id);
        break;
    case printing_state_t::RESUMING:
        disable_button(p_button);
        set_icon_and_label(item_id_t::resuming, btn_id, lbl_id);
        break;
    case printing_state_t::REHEATING:
    case printing_state_t::REHEATING_DONE:
        disable_button(p_button);
        set_icon_and_label(item_id_t::reheating, btn_id, lbl_id);
        break;
    case printing_state_t::PRINTED:
        enable_button(p_button);
        set_icon_and_label(item_id_t::reprint, btn_id, lbl_id);
        break;
    case printing_state_t::ABORTING:
        disable_button(p_button);
        break;
    }
}

void set_tune_icon_and_label(screen_t *screen) {
    window_icon_t *p_button = &pw->w_buttons[static_cast<size_t>(Btn::Tune)];
    int16_t btn_id = p_button->win.id;
    int16_t lbl_id = pw->w_labels[static_cast<size_t>(Btn::Tune)].win.id;

    //must be before switch
    set_icon_and_label(item_id_t::settings, btn_id, lbl_id);

    switch (get_state(screen)) {
    case printing_state_t::PRINTING:
    case printing_state_t::PAUSED:
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
    int16_t btn_id = p_button->win.id;
    int16_t lbl_id = pw->w_labels[static_cast<size_t>(Btn::Stop)].win.id;

    switch (get_state(screen)) {
    case printing_state_t::PRINTED:
        enable_button(p_button);
        set_icon_and_label(item_id_t::home, btn_id, lbl_id);
        break;
    case printing_state_t::PAUSING:
    case printing_state_t::RESUMING:
        disable_button(p_button);
        set_icon_and_label(item_id_t::stop, btn_id, lbl_id);
        break;
    case printing_state_t::ABORTING:
        disable_button(p_button);
        break;
    default:
        enable_button(p_button);
        set_icon_and_label(item_id_t::stop, btn_id, lbl_id);
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

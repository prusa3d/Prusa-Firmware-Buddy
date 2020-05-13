#include "dbg.h"
#include "gui.h"
#include "config.h"
#include "window_header.h"
#include "status_footer.h"
#include "marlin_client.h"
#include "filament.h"
#include "marlin_server.h"
#include "print_utils.h"
#include "screens.h"

#include "ffconf.h"

#include "../Marlin/src/libs/duration_t.h"

#include "../Marlin/src/gcode/lcd/M73_PE.h"

#include <ctime>

#ifdef DEBUG_FSENSOR_IN_HEADER
    #include "filament_sensor.h"
#endif

#define COLOR_VALUE_VALID COLOR_WHITE
//#define COLOR_VALUE_INVALID COLOR_YELLOW
#define COLOR_VALUE_INVALID COLOR_WHITE

#define BUTTON_TUNE  0
#define BUTTON_PAUSE 1
#define BUTTON_STOP  2

#define POPUP_MSG_DUR_MS 5000

#pragma pack(push)
#pragma pack(1)

typedef enum {
    P_INITIAL,
    P_PRINTING,
    P_PAUSING,
    P_PAUSED,
    P_RESUMING,
    P_REHEATING,
    P_REHEATING_DONE,
    P_MBL_FAILED,
    P_PRINTED,
    P_COUNT //setting this state == forced update
} printing_state_t;

typedef enum {
    iid_settings,
    iid_pause,
    iid_pausing,
    iid_stop,
    iid_resume,
    iid_resuming,
    iid_reheating,
    iid_reprint,
    iid_home,
    iid_count
} item_id_t;

#pragma pack(pop)

const uint16_t printing_icons[iid_count] = {
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

const char *printing_labels[iid_count] = {
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

#pragma pack(push, 1)

typedef struct
{
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
    uint8_t last_sd_percent_done;

    char text_time[9];
    char text_etime[9];
    char text_filament[5]; // 999m\0 | 1.2m\0

    window_text_t w_message; //Messages from onStatusChanged()
    uint32_t message_timer;
    uint8_t message_flag;
    printing_state_t state__readonly__use_change_print_state;
} screen_printing_data_t;

#pragma pack(pop)

void screen_printing_init(screen_t *screen);
void screen_printing_done(screen_t *screen);
void screen_printing_draw(screen_t *screen);
int screen_printing_event(screen_t *screen, window_t *window, uint8_t event, void *param);

#define pw ((screen_printing_data_t *)screen->pdata)

static void invalidate_print_state(screen_t *screen) {
    pw->state__readonly__use_change_print_state = P_COUNT;
}
static printing_state_t get_state(screen_t *screen) {
    return pw->state__readonly__use_change_print_state;
}

static void screen_printing_reprint(screen_t *screen);
//static void mesh_err_stop_print(screen_t *screen); //todo use it
static void change_print_state(screen_t *screen);
static void update_progress(screen_t *screen, uint8_t percent, uint16_t print_speed);
static void update_remaining_time(screen_t *screen, time_t time_to_end);
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
extern "C" screen_t *const get_scr_printing() { return &screen_printing; }

void screen_printing_init(screen_t *screen) {
    marlin_error_clr(MARLIN_ERR_ProbingFailed);
    int16_t id;

    marlin_vars_t *vars = marlin_vars();

    strcpy(pw->text_time, "0m");
    strcpy(pw->text_filament, "999m");

    int16_t root = window_create_ptr(WINDOW_CLS_FRAME, -1,
        rect_ui16(0, 0, 0, 0),
        &(pw->root));

    id = window_create_ptr(WINDOW_CLS_HEADER, root,
        rect_ui16(0, 0, 240, 31), &(pw->header));
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
    window_set_text(id, vars->media_file_name ? vars->media_file_name : "");

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
    window_set_text(id, "Remaining Time");

    id = window_create_ptr(WINDOW_CLS_TEXT, root,
        rect_ui16(130, 148, 101, 20),
        &(pw->w_etime_value));
    pw->w_etime_value.font = resource_font(IDR_FNT_SMALL);
    window_set_alignment(id, ALIGN_RIGHT_BOTTOM);
    window_set_padding(id, padding_ui8(0, 2, 0, 2));
    window_set_text(id, pw->text_etime);

    id = window_create_ptr(WINDOW_CLS_TEXT, root,
        rect_ui16(10, 128, 101, 20),
        &(pw->w_time_label));
    pw->w_time_label.font = resource_font(IDR_FNT_SMALL);
    window_set_alignment(id, ALIGN_RIGHT_BOTTOM);
    window_set_padding(id, padding_ui8(0, 2, 0, 2));
    window_set_text(id, "Printing Time");

    id = window_create_ptr(WINDOW_CLS_TEXT, root,
        rect_ui16(10, 148, 101, 20),
        &(pw->w_time_value));
    pw->w_time_value.font = resource_font(IDR_FNT_SMALL);
    window_set_alignment(id, ALIGN_RIGHT_BOTTOM);
    window_set_padding(id, padding_ui8(0, 2, 0, 2));
    window_set_text(id, pw->text_time);

    id = window_create_ptr(WINDOW_CLS_TEXT, root,
        rect_ui16(10, 75, 230, 95),
        &(pw->w_message));
    pw->w_time_value.font = resource_font(IDR_FNT_SMALL);
    window_set_alignment(id, ALIGN_LEFT_TOP);
    window_set_padding(id, padding_ui8(0, 2, 0, 2));
    window_set_text(id, "No messages");
    window_hide(id);
    pw->message_flag = 0;

    for (uint8_t col = 0; col < 3; col++) {
        id = window_create_ptr(
            WINDOW_CLS_ICON, root,
            rect_ui16(8 + (15 + 64) * col, 185, 64, 64),
            &(pw->w_buttons[col]));
        window_set_color_back(id, COLOR_GRAY);
        window_set_tag(id, col + 1);
        window_enable(id);

        id = window_create_ptr(
            WINDOW_CLS_TEXT, root,
            rect_ui16(80 * col, 196 + 48 + 8, 80, 22),
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
    pw->message_flag = 1;
}

static void close_popup_message(screen_t *screen) {
    window_show(pw->w_etime_label.win.id);
    window_show(pw->w_etime_value.win.id);
    window_show(pw->w_progress.win.id);
    window_show(pw->w_time_label.win.id);
    window_show(pw->w_time_value.win.id);

    window_set_text(pw->w_message.win.id, "");

    window_hide(pw->w_message.win.id);
    pw->message_flag = 0;
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

    if (pw->message_flag != 0 && HAL_GetTick() - pw->message_timer >= POPUP_MSG_DUR_MS) {
        close_popup_message(screen);
    }

    if (status_footer_event(&(pw->footer), window, event, param)) {
        return 1;
    }

    change_print_state(screen);

    if (marlin_vars()->print_duration != pw->last_print_duration)
        update_print_duration(screen, marlin_vars()->print_duration);
    if (marlin_vars()->time_to_end != pw->last_time_to_end)
        update_remaining_time(screen, marlin_vars()->time_to_end);
    if (marlin_vars()->sd_percent_done != pw->last_sd_percent_done)
        update_progress(screen, marlin_vars()->sd_percent_done, marlin_vars()->print_speed);

    if (event != WINDOW_EVENT_CLICK) {
        return 0;
    }

    switch (((int)param) - 1) {
    case BUTTON_TUNE:
        switch (get_state(screen)) {
        case P_PRINTING:
        case P_PAUSED:
            screen_open(get_scr_menu_tune()->id);
            break;
        default:
            break;
        }
        return 1;
        break;
    case BUTTON_PAUSE: {
        switch (get_state(screen)) {
        case P_PRINTING:
            marlin_print_pause();
            break;
        case P_PAUSED:
            marlin_print_resume();
            break;
        case P_PRINTED:
            screen_printing_reprint(screen);
            break;
        default:
            break;
        }
        break;
    }
    case BUTTON_STOP:
        switch (get_state(screen)) {
        case P_PRINTED:
            screen_close();
            return 1;
        case P_PAUSING:
        case P_RESUMING:
            return 0;
        default: {
            if (gui_msgbox("Are you sure to stop this printing?",
                    MSGBOX_BTN_YESNO | MSGBOX_ICO_WARNING | MSGBOX_DEF_BUTTON1)
                == MSGBOX_RES_YES) {
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
    window_icon_t *p_button = &pw->w_buttons[BUTTON_TUNE];
    p_button->win.f_disabled = 1;
    p_button->win.f_enabled = 0; // cant't be focused

    // move to reprint when tune is focused
    if (window_is_focused(p_button->win.id)) {
        window_set_focus(pw->w_buttons[BUTTON_PAUSE].win.id);
    }
    window_invalidate(p_button->win.id);
}

static void enable_tune_button(screen_t *screen) {
    window_icon_t *p_button = &pw->w_buttons[BUTTON_TUNE];

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
        struct tm *timeinfo = localtime(&rawtime);
        //standard would be:
        //strftime(pw->text_etime, sizeof(pw->text_etime) / sizeof(pw->text_etime[0]), "%jd %Hh", timeinfo);
        if (timeinfo->tm_yday) {
            snprintf(pw->text_etime, sizeof(pw->text_etime) / sizeof(pw->text_etime[0]), "%id %2ih", timeinfo->tm_yday, timeinfo->tm_hour);
        } else if (timeinfo->tm_hour) {
            snprintf(pw->text_etime, sizeof(pw->text_etime) / sizeof(pw->text_etime[0]), "%ih %2im", timeinfo->tm_hour, timeinfo->tm_min);
        } else {
            snprintf(pw->text_etime, sizeof(pw->text_etime) / sizeof(pw->text_etime[0]), "%im", timeinfo->tm_min);
        }
    } else
        strcpy_P(pw->text_etime, PSTR("N/A"));

    window_set_text(pw->w_etime_value.win.id, pw->text_etime);
}

static void update_print_duration(screen_t *screen, time_t rawtime) {
    pw->w_time_value.color_text = COLOR_VALUE_VALID;
    struct tm *timeinfo = localtime(&rawtime);
    if (timeinfo->tm_yday) {
        snprintf(pw->text_time, sizeof(pw->text_time) / sizeof(pw->text_time[0]), "%id %2ih", timeinfo->tm_yday, timeinfo->tm_hour);
    } else if (timeinfo->tm_hour) {
        snprintf(pw->text_time, sizeof(pw->text_time) / sizeof(pw->text_time[0]), "%ih %2im", timeinfo->tm_hour, timeinfo->tm_min);
    } else if (timeinfo->tm_min) {
        snprintf(pw->text_time, sizeof(pw->text_time) / sizeof(pw->text_time[0]), "%im %2is", timeinfo->tm_min, timeinfo->tm_sec);
    } else {
        snprintf(pw->text_time, sizeof(pw->text_time) / sizeof(pw->text_time[0]), "%is", timeinfo->tm_sec);
    }
    window_set_text(pw->w_time_value.win.id, pw->text_time);
}

static void screen_printing_reprint(screen_t *screen) {
    print_begin(marlin_vars()->media_file_path);
    window_set_text(pw->w_etime_label.win.id, PSTR("Remaining Time")); // !!! "screen_printing_init()" is not invoked !!!

    window_set_text(pw->w_labels[BUTTON_STOP].win.id, printing_labels[iid_stop]);
    window_set_icon_id(pw->w_buttons[BUTTON_STOP].win.id, printing_icons[iid_stop]);

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
    if (window_get_icon_id(btn_id) != printing_icons[id_to_set])
        window_set_icon_id(btn_id, printing_icons[id_to_set]);
    //compare pointers to text, compare texts would take too long
    if (window_get_text(lbl_id) != printing_labels[id_to_set])
        window_set_text(lbl_id, printing_labels[id_to_set]);
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
    window_icon_t *p_button = &pw->w_buttons[BUTTON_PAUSE];
    int16_t btn_id = p_button->win.id;
    int16_t lbl_id = pw->w_labels[BUTTON_PAUSE].win.id;

    //todo it is static, because menu tune is not dialog
    //switch (pw->state__readonly__use_change_print_state)
    switch (get_state(screen)) {
    case P_COUNT:
    case P_INITIAL:
    case P_PRINTING:
    case P_MBL_FAILED:
        enable_button(p_button);
        set_icon_and_label(iid_pause, btn_id, lbl_id);
        break;
    case P_PAUSING:
        disable_button(p_button);
        set_icon_and_label(iid_pausing, btn_id, lbl_id);
        break;
    case P_PAUSED:
        enable_button(p_button);
        set_icon_and_label(iid_resume, btn_id, lbl_id);
        break;
    case P_RESUMING:
        disable_button(p_button);
        set_icon_and_label(iid_resuming, btn_id, lbl_id);
        break;
    case P_REHEATING:
    case P_REHEATING_DONE:
        disable_button(p_button);
        set_icon_and_label(iid_reheating, btn_id, lbl_id);
        break;
    case P_PRINTED:
        enable_button(p_button);
        set_icon_and_label(iid_reprint, btn_id, lbl_id);
        break;
    }
}

void set_tune_icon_and_label(screen_t *screen) {
    window_icon_t *p_button = &pw->w_buttons[BUTTON_TUNE];
    int16_t btn_id = p_button->win.id;
    int16_t lbl_id = pw->w_labels[BUTTON_TUNE].win.id;

    //must be before switch
    set_icon_and_label(iid_settings, btn_id, lbl_id);

    switch (get_state(screen)) {
    case P_PRINTING:
    case P_PAUSED:
        enable_tune_button(screen);
        break;
    default:
        disable_tune_button(screen);
        break;
    }
}

void set_stop_icon_and_label(screen_t *screen) {
    window_icon_t *p_button = &pw->w_buttons[BUTTON_STOP];
    int16_t btn_id = p_button->win.id;
    int16_t lbl_id = pw->w_labels[BUTTON_STOP].win.id;

    switch (get_state(screen)) {
    case P_PRINTED:
        enable_button(p_button);
        set_icon_and_label(iid_home, btn_id, lbl_id);
        break;
    case P_PAUSING:
    case P_RESUMING:
        disable_button(p_button);
        set_icon_and_label(iid_stop, btn_id, lbl_id);
        break;
    default:
        enable_button(p_button);
        set_icon_and_label(iid_stop, btn_id, lbl_id);
        break;
    }
}

static void change_print_state(screen_t *screen) {
    printing_state_t st = P_COUNT;

    switch (marlin_vars()->print_state) {
    case mpsIdle:
        st = P_INITIAL;
        break;
    case mpsPrinting:
        st = P_PRINTING;
        break;
    case mpsPaused:
        st = P_PAUSED;
        break;
    case mpsPausing_Begin:
    case mpsPausing_WaitIdle:
    case mpsPausing_ParkHead:
        st = P_PAUSING;
        break;
    case mpsResuming_Reheating:
        st = P_REHEATING;
        break;
    case mpsResuming_Begin:
    case mpsResuming_UnparkHead:
        st = P_RESUMING;
        break;
    case mpsFinishing_WaitIdle:
    case mpsFinishing_ParkHead:
    case mpsAborting_Begin:
    case mpsAborting_WaitIdle:
    case mpsAborting_ParkHead:
        st = P_PRINTING;
        break;
    case mpsAborted:
        st = P_PRINTED;
        break;
    case mpsFinished:
        st = P_PRINTED;
        break;
    }
    if (pw->state__readonly__use_change_print_state != st) {
        pw->state__readonly__use_change_print_state = st;
        set_pause_icon_and_label(screen);
        set_tune_icon_and_label(screen);
        set_stop_icon_and_label(screen);
    }
}

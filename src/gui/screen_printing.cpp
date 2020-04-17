/*
 * screen_prusa.c
 *
 *  Created on: 16. 7. 2019
 *      Author: mcbig
 */

#include "dbg.h"
#include "gui.h"
#include "config.h"
#include "window_header.h"
#include "status_footer.h"
#include "marlin_client.h"
#include "filament.h"
#include "screen_printing.h"
#include "marlin_server.h"
#include "print_utils.h"

#include "ffconf.h"

#include "../Marlin/src/libs/duration_t.h"

#include "../Marlin/src/gcode/lcd/M73_PE.h"

#ifdef DEBUG_FSENSOR_IN_HEADER
    #include "filament_sensor.h"
#endif

extern "C" {
extern screen_t *pscreen_home;
extern screen_t *pscreen_menu_tune;
}

#define COLOR_VALUE_VALID COLOR_WHITE
//#define COLOR_VALUE_INVALID COLOR_YELLOW
#define COLOR_VALUE_INVALID COLOR_WHITE

#define BUTTON_TUNE  0
#define BUTTON_PAUSE 1
#define BUTTON_STOP  2

#define POPUP_MSG_DUR_MS 5000

#define HEATING_DIFFERENCE 1

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
    P_PRINTED
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

#pragma pack(push)
#pragma pack(1)

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
    // window_text_t w_filament_label;
    // window_text_t w_filament_value;

    window_icon_t w_buttons[3];
    window_text_t w_labels[3];

    //printing_state_t state__readonly__use_change_print_state;
    //todo it is static, because menu tune is not dialog

    uint32_t last_timer_repaint;

    status_footer_t footer;

    char text_time[13];    // 999d 23h 30m\0
    char text_etime[9];    // 999X 23Y\0
    char text_filament[5]; // 999m\0 | 1.2m\0

    window_text_t w_message; //Messages from onStatusChanged()
    uint32_t message_timer;
    uint8_t message_flag;

} screen_printing_data_t;

static printing_state_t state__readonly__use_change_print_state;
void reset_print_state(void) {
    marlin_set_print_speed(100);
    state__readonly__use_change_print_state = P_INITIAL;
}

#pragma pack(pop)

class Lock {
    static bool locked;

public:
    Lock() {
        locked = true;
    }
    static bool IsLocked() {
        return locked;
    }
    ~Lock() {
        locked = false;
    }
};
bool Lock::locked = false;

void screen_printing_init(screen_t *screen);
void screen_printing_done(screen_t *screen);
void screen_printing_draw(screen_t *screen);
int screen_printing_event(screen_t *screen, window_t *window, uint8_t event, void *param);
void screen_printing_timer(screen_t *screen, uint32_t seconds);
void screen_printing_update_progress(screen_t *screen);
void screen_printing_pause_print(screen_t *screen);
void screen_printing_resume_print(screen_t *screen);
void screen_printing_reprint(screen_t *screen);
void screen_printing_printed(screen_t *screen);
void screen_mesh_err_stop_print(screen_t *screen);
void change_print_state(screen_t *screen, printing_state_t state);

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
const screen_t *pscreen_printing = &screen_printing;

//TODO: rework this, save memory
char screen_printing_file_name[_MAX_LFN + 1] = { '\0' }; //+1 for '\0' character (avoid warning)
char screen_printing_file_path[_MAX_LFN + 2] = { '\0' }; //+1 for '/' and '\0' characters (avoid warning)

#define pw ((screen_printing_data_t *)screen->pdata)

struct pduration_t : duration_t {
    pduration_t()
        : pduration_t(0) {};

    pduration_t(uint32_t const &seconds)
        : duration_t(seconds) {}

    void to_string(char *buffer) const {
        int d = this->day(),
            h = this->hour() % 24,
            m = this->minute() % 60,
            s = this->second() % 60;

        if (d) {
            sprintf(buffer, "%3id %2ih %2im", d, h, m);
        } else if (h) {
            sprintf(buffer, "     %2ih %2im", h, m);
        } else if (m) {
            sprintf(buffer, "     %2im %2is", m, s);
        } else {
            sprintf(buffer, "         %2is", s);
        }
    }
};

static void screen_printing_update_remaining_time_progress(screen_t *screen);

void screen_printing_init(screen_t *screen) {
    marlin_error_clr(MARLIN_ERR_ProbingFailed);
    int16_t id;

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
    window_set_text(id, screen_printing_file_name);

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

    //todo it is static, because menu tune is not dialog
    //change_print_state(screen, P_INITIAL);
    change_print_state(screen, state__readonly__use_change_print_state);

    status_footer_init(&(pw->footer), root);
    screen_printing_timer(screen, 1000); // first fast value s update
    screen_printing_update_remaining_time_progress(screen);
}

void screen_printing_done(screen_t *screen) {
    window_destroy(pw->root.win.id);
}

void screen_printing_draw(screen_t *screen) {
}

static void abort_print(screen_t *screen) {
    //must set temperatures to zero
    //if not, and nozzle is cold - marlin would break and next print will stack
    marlin_set_target_nozzle(0);
    marlin_set_target_bed(0);
    if (state__readonly__use_change_print_state == P_PAUSED) {
        marlin_print_resume();
    }
    marlin_print_abort();

    while (marlin_vars()->sd_printing) {
        gui_loop();
    }
    marlin_set_target_nozzle(0);
    marlin_set_target_bed(0);
    marlin_park_head();
    while (marlin_vars()->pqueue) {
        gui_loop();
    }
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
    if (Lock::IsLocked())
        return 0;
    Lock l;

    if (event == WINDOW_EVENT_MESSAGE && msg_stack.count > 0) {
        open_popup_message(screen);
        return 0;
    }

    if (pw->message_flag != 0 && HAL_GetTick() - pw->message_timer >= POPUP_MSG_DUR_MS) {
        close_popup_message(screen);
    }

    if (marlin_error(MARLIN_ERR_ProbingFailed)) {
        marlin_error_clr(MARLIN_ERR_ProbingFailed);
        if (state__readonly__use_change_print_state != P_MBL_FAILED) {
            change_print_state(screen, P_MBL_FAILED);
            screen_mesh_err_stop_print(screen);

            if (gui_msgbox("Bed leveling failed. Try again?", MSGBOX_BTN_YESNO) == MSGBOX_RES_YES) {
                screen_printing_reprint(screen);
                change_print_state(screen, P_PRINTING);
            } else {
                marlin_gcode("M104 S0");
                marlin_gcode("M140 S0");
                screen_close();
            }
        }
    }

    if (p_window_header_event_clr(&(pw->header), MARLIN_EVT_MediaRemoved)) { // close screen when media removed
        if (state__readonly__use_change_print_state == P_PRINTED) {
            screen_close();
            return 1;
        }
        screen_printing_pause_print(screen);
    }
    window_header_events(&(pw->header));

    screen_printing_timer(screen, (HAL_GetTick() / 50) * 50);

    if (status_footer_event(&(pw->footer), window, event, param)) {
        return 1;
    }

    if (event != WINDOW_EVENT_CLICK) {
        return 0;
    }

    switch (((int)param) - 1) {
    case BUTTON_TUNE:
        switch (state__readonly__use_change_print_state) {
        case P_PRINTING:
        case P_PAUSED:
            screen_open(pscreen_menu_tune->id);
            break;
        default:
            break;
        }
        return 1;
        break;
    case BUTTON_PAUSE: {
        //todo it is static, because menu tune is not dialog
        //switch(pw->state__readonly__use_change_print_state)
        switch (state__readonly__use_change_print_state) {
        case P_PRINTING:
            screen_printing_pause_print(screen);
            break;
        case P_PAUSED:
            screen_printing_resume_print(screen);
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
        //todo it is static, because menu tune is not dialog
        //if(pw->state__readonly__use_change_print_state == P_PRINTED)
        switch (state__readonly__use_change_print_state) {
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
                abort_print(screen);
                screen_close();
                return 1;
            } else
                return 0;
        }
        }
        break;
    }
    return 0;
}

void screen_printing_timer(screen_t *screen, uint32_t mseconds) {
    if ((mseconds - pw->last_timer_repaint) >= 1000) {
        screen_printing_update_progress(screen);
        pw->last_timer_repaint = mseconds;
    }
}
void screen_printing_disable_tune_button(screen_t *screen) {
    window_icon_t *p_button = &pw->w_buttons[BUTTON_TUNE];
    p_button->win.f_disabled = 1;
    p_button->win.f_enabled = 0; // cant't be focused

    // move to reprint when tune is focused
    if (window_is_focused(p_button->win.id)) {
        window_set_focus(pw->w_buttons[BUTTON_PAUSE].win.id);
    }
    window_invalidate(p_button->win.id);
}

void screen_printing_enable_tune_button(screen_t *screen) {
    window_icon_t *p_button = &pw->w_buttons[BUTTON_TUNE];

    p_button->win.f_disabled = 0;
    p_button->win.f_enabled = 1; // can be focused
    window_invalidate(p_button->win.id);
}

void _state_loop(screen_t *screen) {
    //todo it is static, because menu tune is not dialog
    //switch (pw->state__readonly__use_change_print_state)

    auto p_vars = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_SD_PRINT));
    switch (state__readonly__use_change_print_state) {
    case P_PRINTING:
        if ((!p_vars->sd_printing) && (marlin_command() != MARLIN_CMD_M600) && // prevent false trigering durring M600 TODO: better solution
            !marlin_motion())                                                  // wait for motors idle
            screen_printing_printed(screen);
        else
            screen_printing_update_remaining_time_progress(screen);
        break;
    case P_INITIAL:
    case P_PAUSED:
    case P_RESUMING:
    case P_PRINTED:
    case P_REHEATING_DONE:
        //case P_MBL_FAILED:
        if (p_vars->sd_printing) {
            change_print_state(screen, P_PRINTING);
        }
        break;
    case P_MBL_FAILED:
    default:
        break;
    }
}

static void screen_printing_update_remaining_time_progress(screen_t *screen) {
    uint8_t nPercent;
    if (oProgressData.oPercentDone.mIsActual(marlin_vars()->print_duration)) {
        nPercent = (uint8_t)oProgressData.oPercentDone.mGetValue();
        oProgressData.oTime2End.mFormatSeconds(pw->text_etime, marlin_vars()->print_speed);
        pw->w_etime_value.color_text = ((marlin_vars()->print_speed == 100) ? COLOR_VALUE_VALID : COLOR_VALUE_INVALID);
        pw->w_progress.color_text = COLOR_VALUE_VALID;
        //_dbg(".progress: %d\r",nPercent);
    } else {
        nPercent = marlin_vars()->sd_percent_done;
        strcpy_P(pw->text_etime, PSTR("N/A"));
        pw->w_etime_value.color_text = COLOR_VALUE_VALID;
        pw->w_progress.color_text = COLOR_VALUE_INVALID;
        //_dbg(".progress: %d ???\r",nPercent);
    }
    window_set_value(pw->w_progress.win.id, nPercent);
    window_set_text(pw->w_etime_value.win.id, pw->text_etime);

    //-//		_dbg("progress: %d", ExtUI::getProgress_percent());
    _dbg("progress: %d", nPercent);
    //_dbg("##% FeedRate %d",feedrate_percentage);
}

void screen_printing_update_progress(screen_t *screen) {
    if (marlin_reheating()) {
        //todo it is static, because menu tune is not dialog
        //if (pw->state__readonly__use_change_print_state != P_REHEATING)
        if (state__readonly__use_change_print_state != P_REHEATING)
            change_print_state(screen, P_REHEATING); //state change is not checked inside
    } else {
        //todo it is static, because menu tune is not dialog
        //switch(pw->state__readonly__use_change_print_state)
        switch (state__readonly__use_change_print_state) {
        case P_REHEATING:
            marlin_print_resume();
            change_print_state(screen, P_REHEATING_DONE);
            break;

        case P_PAUSING:
            if (marlin_event_clr(MARLIN_EVT_UserConfirmRequired)) {
                change_print_state(screen, P_PAUSED);
            } //no break, need to do default
        default:
            _state_loop(screen);
        }
    }

    //const pduration_t e_time(ExtUI::getProgress_seconds_elapsed());

    const pduration_t e_time(marlin_vars()->print_duration);
    e_time.to_string(pw->text_time);
    window_set_text(pw->w_time_value.win.id, pw->text_time);
    //_dbg("#.. progress / p :: %d t0: %d ?: %d\r",oProgressData.oPercentDirectControl.mGetValue(),oProgressData.oPercentDirectControl.nTime,oProgressData.oPercentDirectControl.mIsActual(print_job_timer.duration()));
    //_dbg("#.. progress / P :: %d t0: %d ?: %d\r",oProgressData.oPercentDone.mGetValue(),oProgressData.oPercentDone.nTime,oProgressData.oPercentDone.mIsActual(print_job_timer.duration()));
    //_dbg("#.. progress / R :: %d t0: %d ?: %d\r",oProgressData.oTime2End.mGetValue(),oProgressData.oTime2End.nTime,oProgressData.oTime2End.mIsActual(print_job_timer.duration()));
    //_dbg("#.. progress / T :: %d t0: %d ?: %d\r",oProgressData.oTime2Pause.mGetValue(),oProgressData.oTime2Pause.nTime,oProgressData.oTime2Pause.mIsActual(print_job_timer.duration()));

    /*
	sprintf(pw->text_filament, "1.2m");
	window_set_text(pw->w_filament_value.win.id, pw->text_filament);
	*/
}

void screen_printing_pause_print(screen_t *screen) {
    change_print_state(screen, P_PAUSING);
    marlin_event_clr(MARLIN_EVT_UserConfirmRequired);
    marlin_print_pause();
}

void screen_printing_resume_print(screen_t *screen) {
    change_print_state(screen, P_RESUMING);
    marlin_print_resume();
}

void screen_printing_reprint(screen_t *screen) {
    //    marlin_gcode_printf("M23 %s", screen_printing_file_path);
    //    marlin_gcode("M24");
    //    oProgressData.mInit();
    print_begin(screen_printing_file_path);
    window_set_text(pw->w_etime_label.win.id, PSTR("Remaining Time")); // !!! "screen_printing_init()" is not invoked !!!

    window_set_text(pw->w_labels[BUTTON_STOP].win.id, printing_labels[iid_stop]);
    window_set_icon_id(pw->w_buttons[BUTTON_STOP].win.id, printing_icons[iid_stop]);

#ifndef DEBUG_FSENSOR_IN_HEADER
    p_window_header_set_text(&(pw->header), "PRINTING");
#endif
}

void screen_printing_printed(screen_t *screen) {
    marlin_set_print_speed(100);
    change_print_state(screen, P_PRINTED);
    window_set_value(pw->w_progress.win.id, 100);

#ifndef DEBUG_FSENSOR_IN_HEADER
    p_window_header_set_text(&(pw->header), "PRINT DONE"); // beware! this must not hit the ethernet icon, so keep it short
#endif

    pw->w_progress.color_text = COLOR_VALUE_VALID;
    window_set_text(pw->w_etime_label.win.id, PSTR(""));
    window_set_text(pw->w_etime_value.win.id, PSTR(""));

    //screen_printing_disable_tune_button(screen);
}

void screen_mesh_err_stop_print(screen_t *screen) {
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
}

void set_icon_and_label(item_id_t id_to_set, int16_t btn_id, int16_t lbl_id) {
    if (window_get_icon_id(btn_id) != printing_icons[id_to_set])
        window_set_icon_id(btn_id, printing_icons[id_to_set]);
    //compare pointers to text, compare texts would take too long
    if (window_get_text(lbl_id) != printing_labels[id_to_set])
        window_set_text(lbl_id, printing_labels[id_to_set]);
}

void enable_button(window_icon_t *p_button) {
    if (p_button->win.f_disabled) {
        p_button->win.f_disabled = 0;
        window_invalidate(p_button->win.id);
    }
}

void disable_button(window_icon_t *p_button) {
    if (!p_button->win.f_disabled) {
        p_button->win.f_disabled = 1;
        window_invalidate(p_button->win.id);
    }
}

void set_pause_icon_and_label(screen_t *screen) {
    window_icon_t *p_button = &pw->w_buttons[BUTTON_PAUSE];
    int16_t btn_id = p_button->win.id;
    int16_t lbl_id = pw->w_labels[BUTTON_PAUSE].win.id;

    //todo it is static, because menu tune is not dialog
    //switch (pw->state__readonly__use_change_print_state)
    switch (state__readonly__use_change_print_state) {
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

    //todo it is static, because menu tune is not dialog
    //switch (pw->state__readonly__use_change_print_state)
    switch (state__readonly__use_change_print_state) {
    case P_PRINTING:
    case P_PAUSED:
        screen_printing_enable_tune_button(screen);
        break;
    default:
        screen_printing_disable_tune_button(screen);
        break;
    }
}

void set_stop_icon_and_label(screen_t *screen) {
    window_icon_t *p_button = &pw->w_buttons[BUTTON_STOP];
    int16_t btn_id = p_button->win.id;
    int16_t lbl_id = pw->w_labels[BUTTON_STOP].win.id;

    //todo it is static, because menu tune is not dialog
    //switch (pw->state__readonly__use_change_print_state)
    switch (state__readonly__use_change_print_state) {
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

void change_print_state(screen_t *screen, printing_state_t st) {
    _dbg("printstate %d entered", (int)st);
    //todo it is static, because menu tune is not dialog
    //pw->state__readonly__use_change_print_state = st;
    state__readonly__use_change_print_state = st;
    set_pause_icon_and_label(screen);
    set_tune_icon_and_label(screen);
    set_stop_icon_and_label(screen);
}

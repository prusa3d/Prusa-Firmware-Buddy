// wizard_ui.c

#include "wizard_ui.h"
#include "wizard_types.h"
#include "resource.h"
#include "stm32f4xx_hal.h" //HAL_GetTick
#include "marlin_client.h" //HAL_GetTick

uint16_t wizard_get_test_icon_resource(uint8_t state) {
    switch (state) {
    case _TEST_START:
        return IDR_PNG_wizard_icon_na;
    case _TEST_RUN:
        return (HAL_GetTick() & 256) ? IDR_PNG_wizard_icon_ip0 : IDR_PNG_wizard_icon_ip1;
    case _TEST_PASSED:
        return IDR_PNG_wizard_icon_ok;
    case _TEST_FAILED:
        return IDR_PNG_wizard_icon_ng;
    }
    return 0;
}

void wizard_update_test_icon(int16_t win_id, uint8_t state) {
    window_set_icon_id(win_id, wizard_get_test_icon_resource(state));
}

// messagebox with custom buttons (NEXT and DONE), optionaly icon and rectangle
int wizard_msgbox_ex(const char *text, uint16_t flags, uint16_t id_icon, rect_ui16_t rc) {
    const char *custom_btn = 0;
    if ((flags & MSGBOX_MSK_BTN) == MSGBOX_BTN_NEXT)
        custom_btn = "NEXT";
    else if ((flags & MSGBOX_MSK_BTN) == MSGBOX_BTN_DONE)
        custom_btn = "DONE";
    if (custom_btn) {
        flags = (flags & ~MSGBOX_MSK_BTN) | MSGBOX_BTN_CUSTOM1;
        return gui_msgbox_ex(0, text, flags | MSGBOX_ICO_CUSTOM, rc, id_icon, &custom_btn);
    }
    return gui_msgbox_ex(0, text, flags | MSGBOX_ICO_CUSTOM, rc, id_icon, 0);
}

int wizard_msgbox(const char *text, uint16_t flags, uint16_t id_icon) {
    return wizard_msgbox_ex(text, flags, id_icon, gui_defaults.scr_body_sz);
}

int wizard_msgbox1(const char *text, uint16_t flags, uint16_t id_icon) {
    return wizard_msgbox_ex(text, flags, id_icon,
        rect_ui16(0, 76, 240, 320 - 140)); // FIXME looks like manual vertical center align
}

int wizard_msgbox_btns(const char *text, uint16_t flags, uint16_t id_icon, const char **buttons) {
    return gui_msgbox_ex(0, text, flags | MSGBOX_ICO_CUSTOM,
        gui_defaults.scr_body_sz, id_icon, buttons);
}

int wizard_timer(uint32_t *p_timer, uint32_t delay_ms, _TEST_STATE_t *pstate, _WIZ_TIMER_t type) {
    int progress = 0;
    switch (*pstate) {
    case _TEST_START:
        *p_timer = HAL_GetTick();
        *pstate = _TEST_RUN;
        break;
    case _TEST_RUN:
        progress = 100 * (HAL_GetTick() - *p_timer) / delay_ms;
        if (progress >= 100) {
            switch (type) {
            case _WIZ_TIMER:
                progress = 99;
                break;
            case _WIZ_TIMER_AUTOPASS:
                progress = 100;
                *pstate = _TEST_PASSED;
                break;
            case _WIZ_TIMER_AUTOFAIL:
                progress = 100;
                *pstate = _TEST_FAILED;
                break;
            }
        }
        break;
    case _TEST_PASSED:
    case _TEST_FAILED:
        progress = 100;
        break;
    }
    return progress;
}

static void _wizard_init_test() {
    if (!marlin_processing())
        marlin_start_processing();
    marlin_settings_load();
}

static void _disable_PID() {
    //M301 - Set Hotend PID
    //M301 [C<value>] [D<value>] [E<index>] [I<value>] [L<value>] [P<value>]
    marlin_gcode_printf("M301 D0 I0 1000000");
    //M304 - Set Bed PID
    //M304 [D<value>] [I<value>] [P<value>]
    marlin_gcode_printf("M304 D0 I0 1000000");
}

void wizard_init(float t_noz, float t_bed /*, int16_t footer_id*/) {
    /*window_hide(footer_id);*/
    _wizard_init_test();

    //Set Hotend Temperature
    marlin_gcode_printf("M104 S%d", (int)t_noz);

    //Set Bed Temperature
    marlin_gcode_printf("M140 S%d", (int)t_bed);
}
/*
void wizard_init_footer(float t_noz, float t_bed, int16_t footer_id)
{
	window_show(footer_id);
	_wizard_init_test();

	//Set Hotend Temperature
	marlin_gcode_printf("M104 S%d",(int)t_noz);

	//Set Bed Temperature
	marlin_gcode_printf("M140 S%d",(int)t_bed);

}*/

void wizard_init_disable_PID(float t_noz, float t_bed /*, int16_t footer_id*/) {
    /*window_hide(footer_id);*/
    _wizard_init_test();
    _disable_PID();

    //Set Hotend Temperature
    marlin_gcode_printf("M104 S%d", (int)t_noz);

    //Set Bed Temperature
    marlin_gcode_printf("M140 S%d", (int)t_bed);
}
/*
void wizard_init_footer_disable_PID(float t_noz, float t_bed, int16_t footer_id)
{
	window_show(footer_id);
	_wizard_init_test();
	_disable_PID();

	//Set Hotend Temperature
	marlin_gcode_printf("M104 S%d",(int)t_noz);

	//Set Bed Temperature
	marlin_gcode_printf("M140 S%d",(int)t_bed);

}*/

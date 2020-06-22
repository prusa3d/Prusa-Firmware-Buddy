// wizard_ui.h
#pragma once
#include "gui.h"
#include "wizard_types.h"

#define WIZARD_MARGIN_LEFT  6
#define WIZARD_MARGIN_RIGHT 6
#define WIZARD_X_SPACE      (240 - (WIZARD_MARGIN_LEFT + WIZARD_MARGIN_RIGHT))

#define MSGBOX_BTN_NEXT MSGBOX_BTN_MAX + 1
#define MSGBOX_BTN_DONE MSGBOX_BTN_MAX + 2

// types of wizard_timer()
// _WIZ_TIMER_AUTOPASS means - when progress reaches 100%, state automatically set to _TEST_PASSED
// _WIZ_TIMER_AUTOFAIL means - when progress reaches 100%, state automatically set to _TEST_FAILED
typedef enum {
    _WIZ_TIMER,
    _WIZ_TIMER_AUTOPASS,
    _WIZ_TIMER_AUTOFAIL
} _WIZ_TIMER_t;

extern int wizard_timer(uint32_t *p_timer, uint32_t delay_ms, _TEST_STATE_t *pstate,
    _WIZ_TIMER_t type);

extern void wizard_update_test_icon(int16_t win_id, uint8_t state);

extern uint16_t wizard_get_test_icon_resource(uint8_t state);

extern int wizard_msgbox_ex(const char *text, uint16_t flags, uint16_t id_icon, rect_ui16_t rc);

extern int wizard_msgbox(const char *text, uint16_t flags, uint16_t id_icon);

extern int wizard_msgbox1(const char *text, uint16_t flags, uint16_t id_icon);

extern int wizard_msgbox_btns(const char *text, uint16_t flags, uint16_t id_icon,
    const char **buttons);

extern void wizard_init(float t_noz, float t_bed /*, int16_t footer_id*/);

extern void wizard_init_disable_PID(float t_noz, float t_bed /*, int16_t footer_id*/);
/*
extern void wizard_init_footer(float t_noz, float t_bed, int16_t footer_id);

extern void wizard_init_footer_disable_PID(float t_noz, float t_bed, int16_t footer_id);
*/

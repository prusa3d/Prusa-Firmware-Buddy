// firstlay.h
#pragma once

//choose 0 off 1 on
#define DEBUG_TERM 0
//#define DEBUG_TERM 1

#include <inttypes.h>
#include "gui.h"
#include "wizard_types.h"
#include "wizard_load_unload.h"

#pragma pack(push, 1)
//#pragma pack(1) makes enums 8 bit
// which is an ugly and unreadable hack (probably a side effect)
typedef enum {
    _FL_INIT,
    _FL_GCODE_HEAD,
    _FL_GCODE_BODY,
    _FL_GCODE_DONE
} _FL_STATE;

#pragma pack(pop)

#define FIRSTLAY_SCREEN_TERM_X 25
#define FIRSTLAY_SCREEN_TERM_Y 10
typedef struct
{
    _FL_STATE state;
    window_progress_t progress;
#if DEBUG_TERM == 0
    window_text_t text_state;
#else
    window_term_t term;
    term_t terminal;
    uint8_t term_buff[TERM_BUFF_SIZE(FIRSTLAY_SCREEN_TERM_X, FIRSTLAY_SCREEN_TERM_Y)]; //chars and attrs (640 bytes) + change bitmask (40 bytes)
#endif

    float extruder_start_len;
    LD_UNLD_STATE_t load_unload_state;

    window_text_t text_Z_pos;
    window_text_t text_direction_arrow;
    window_numb_t spin_baby_step;

    float Z_offset;
    float Z_offset_request;

    //it is being deleted i do not know why. size isue?
    /*const char**      head_gcode;
	const char**      body_gcode;
	size_t            head_gcode_sz;
	size_t            body_gcode_sz;
	size_t            gcode_sz;*/

    uint32_t timer0;
} firstlay_screen_t;

typedef struct
{
    //_TEST_STATE_t state_heat;
    _TEST_STATE_t state_load;
    _TEST_STATE_t state_print;

} firstlay_data_t;

extern void wizard_firstlay_event_dn(firstlay_screen_t *p_screen);

extern void wizard_firstlay_event_up(firstlay_screen_t *p_screen);

extern void wizard_init_screen_firstlay(int16_t id_body,
    firstlay_screen_t *p_screen, firstlay_data_t *p_data);
/*
extern int wizard_firstlay_heat(int16_t id_body,
		firstlay_screen_t* p_screen, firstlay_data_t* p_data);

extern int wizard_firstlay_load(int16_t id_body,
		firstlay_screen_t* p_screen, firstlay_data_t* p_data);
*/
extern int wizard_firstlay_print(int16_t id_body,
    firstlay_screen_t *p_screen, firstlay_data_t *p_data, float z_offset);

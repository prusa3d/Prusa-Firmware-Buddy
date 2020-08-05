// screen_wizard.h
#pragma once

#include "gui.hpp"
#include "status_footer.h"
#include "wizard_types.h"
//#include "wizard_ui.h"
#include "xyzcalib.h"
#include "selftest.h"
#include "firstlay.h"

union screen_variant_t {
    selftest_fans_axis_screen_t selftest_fans_axis_screen;
    selftest_cool_screen_t selftest_cool_screen;
    selftest_temp_screen_t selftest_temp_screen;
    //selftest_screen_t selftest_screen;
    xyzcalib_screen_t xyzcalib_screen;
    firstlay_screen_t firstlay_screen;
};

//data in screen_variant_t screen_variant does not "survive" between screens
//data in selftest_data_t selftest, xyzcalib_data_t xyzcalib, firstlay_data_t firstlay, does
struct screen_wizard_data_t {
    window_frame_t frame;
    window_frame_t frame_footer;
    //window_header_t header;
    window_text_t header;
    status_footer_t footer;
    window_frame_t frame_body;
    wizard_state_t state;
    uint8_t flags;
    screen_variant_t screen_variant;
    selftest_data_t selftest;
    xyzcalib_data_t xyzcalib;
    firstlay_data_t firstlay;
    /*float Kp_bed;
	float Ki_bed;
	float Kd_bed;
	float Kp_noz;
	float Ki_noz;
	float Kd_noz;*/
};

#define pd ((screen_wizard_data_t *)screen->pdata)

//extern string_view_utf8 wizard_get_caption(screen_t *screen);

extern void wizard_ui_set_progress(int ctl, float val);

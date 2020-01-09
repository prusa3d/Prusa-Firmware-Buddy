// selftest_home.c

#include "selftest_home.h"
#include "config.h"
#include "marlin_client.h"
#include "wizard_ui.h"

int wizard_selftest_home(int16_t id_body, selftest_home_screen_t *p_screen, selftest_home_data_t *p_data) {
    marlin_gcode("G28");//just autohome - do not show any screen
    marlin_wait_motion(100);
    return 100;
}

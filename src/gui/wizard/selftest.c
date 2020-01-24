// selftest.c

#include "selftest.h"

int wizard_selftest_is_ok(int16_t id_body, selftest_data_t *p_data) {
    int ok = 1;
    ok &= (p_data->fans_axis_data.state_fan0 == _TEST_PASSED);
    ok &= (p_data->fans_axis_data.state_fan1 == _TEST_PASSED);
    ok &= (p_data->fans_axis_data.state_x == _TEST_PASSED);
    ok &= (p_data->fans_axis_data.state_y == _TEST_PASSED);
    ok &= (p_data->fans_axis_data.state_z == _TEST_PASSED);
    ok &= (p_data->cool_data.state_cool == _TEST_PASSED);
    ok &= (p_data->temp_data.state_temp_nozzle == _TEST_PASSED);
    ok &= (p_data->temp_data.state_temp_bed == _TEST_PASSED);
    ok &= (p_data->temp_data.state_preheat_bed == _TEST_PASSED);
    ok &= (p_data->temp_data.state_preheat_nozzle == _TEST_PASSED);
    return ok;
}

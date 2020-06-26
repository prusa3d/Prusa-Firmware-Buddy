// selftest.c

#include "selftest.h"
#include "stm32f4xx_hal.h"

uint32_t last_selftest_result;
uint32_t last_selftest_time = 0;

// !!!!!
// !!!!! if it is possible, insert new flags at the beginning (i.e. before the first one)
// !!!!! all changes are best consulted with Content-Department
// !!!!!
static uint32_t get_and_store_selftest_result(int16_t id_body, selftest_data_t *p_data) {
    uint32_t mask = 0;
    mask = (mask << 1) | (p_data->fans_axis_data.state_fan0 != _TEST_PASSED);
    mask = (mask << 1) | (p_data->fans_axis_data.state_fan1 != _TEST_PASSED);
    mask = (mask << 1) | (p_data->fans_axis_data.state_x != _TEST_PASSED);
    mask = (mask << 1) | (p_data->fans_axis_data.state_y != _TEST_PASSED);
    mask = (mask << 1) | (p_data->fans_axis_data.state_z != _TEST_PASSED);
    mask = (mask << 1) | (p_data->cool_data.state_cool != _TEST_PASSED);
    mask = (mask << 1) | (p_data->temp_data.state_temp_nozzle != _TEST_PASSED);
    mask = (mask << 1) | (p_data->temp_data.state_temp_bed != _TEST_PASSED);
    mask = (mask << 1) | (p_data->temp_data.state_preheat_bed != _TEST_PASSED);
    mask = (mask << 1) | (p_data->temp_data.state_preheat_nozzle != _TEST_PASSED);
    last_selftest_result = mask;
    last_selftest_time = HAL_GetTick() / 1000; // ->[s]
    return mask;
}

int wizard_selftest_is_ok(int16_t id_body, selftest_data_t *p_data) {
    return (get_and_store_selftest_result(id_body, p_data) == 0);
}

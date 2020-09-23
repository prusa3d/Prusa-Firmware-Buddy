// marlin_test.cpp
#include "marlin_test.h"
#include "marlin_server.h"
#include "marlin_vars.h"
#include "marlin_events.h"
#include "../Marlin/src/module/planner.h"
#include "../Marlin/src/module/stepper.h"
#include "../Marlin/src/module/endstops.h"
#include "otp.h"
#include "trinamic.h"

CMarlinTest MarlinTest = CMarlinTest();

CMarlinTest::CMarlinTest() {
    sg_sample_on = false;
}

bool CMarlinTest::isInProgress() {
    return (state != mtsIdle) && (state != mtsFinished);
}

bool CMarlinTest::start() {
    state = mtsStart;
    return true;
}

void CMarlinTest::loop() {
    uint8_t movesplanned = planner.movesplanned();
    switch (state) {
    case mtsIdle:
        break;

    case mtsStart: {
        type = mttInit;
        mask = (1 << mttInit) | (1 << mttXAxis) | (1 << mttYAxis);
        marlin_server_set_exclusive_mode(1);
        state = mtsInit;
        char *serial_otp = (char *)OTP_SERIAL_NUMBER_ADDR;
        if (*serial_otp == 0xff) {
            serial_otp = 0;
        }
        char serial[32] = "unknown";
        char fname[32] = "test_unknown.txt";
        if (serial_otp) {
            sprintf(serial, "CZPX%.15s", serial_otp);
            sprintf(fname, "test_CZPX%.15s.txt", serial_otp);
        }
        if (f_open(&fil, fname, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
            fil_ok = true;
            f_printf(&fil, "TEST START\n");
            f_printf(&fil, "printer serial: %s\n\n", serial);
            f_sync(&fil);
        } else
            fil_ok = false;
        break;
    }

    case mtsInit:
        if ((movesplanned == 0) && (queue.length == 0)) {
            // TODO: validate system state (temperature, etc)
            marlin_server_enqueue_gcode("G28");
            marlin_server_set_command(MARLIN_CMD_G28);
            state = mtsInit_Home;
        }
        break;

    case mtsInit_Home:
        if (marlin_server_get_command() != MARLIN_CMD_G28) {
            state = mtsXAxis_Start;
        }
        break;

    case mtsXAxis_Start:
        osDelay(100);
        marlin_server_enqueue_gcode("G28 X");
        marlin_server_set_command(MARLIN_CMD_G28);
        state = mtsXAxis_Home;
        break;
    case mtsXAxis_Home:
        if (marlin_server_get_command() != MARLIN_CMD_G28) {
            state = mtsXAxis_Measure_RL_50mms;
            sg_sample_set(1 << X_AXIS);
            endstops.enable(true);
            XAxis_start(XYAxis_get_fr(state), -1);
        }
        break;
    case mtsXAxis_Measure_RL_50mms:
    case mtsXAxis_Measure_RL_60mms:
    case mtsXAxis_Measure_RL_75mms:
    case mtsXAxis_Measure_RL_100mms:
        if (movesplanned == 0) {
            float fr = XYAxis_get_fr(state);
            XAxis_end(fr, -1);
            state = get_next_state();
            fr = XYAxis_get_fr(state);
            XAxis_start(fr, 1);
        }
        break;
    case mtsXAxis_Measure_LR_50mms:
    case mtsXAxis_Measure_LR_60mms:
    case mtsXAxis_Measure_LR_75mms:
    case mtsXAxis_Measure_LR_100mms:
        if (movesplanned == 0) {
            float fr = XYAxis_get_fr(state);
            XAxis_end(fr, 1);
            state = get_next_state();
            fr = XYAxis_get_fr(state);
            if (fr > 0)
                XAxis_start(fr, -1);
        }
        break;
    case mtsXAxis_Finished:
        endstops.enable(false);
        sg_sample_set(0);
        state = mtsYAxis_Start;
        break;

    case mtsYAxis_Start:
        osDelay(100);
        marlin_server_enqueue_gcode("G28 Y");
        marlin_server_set_command(MARLIN_CMD_G28);
        state = mtsYAxis_Home;
        break;
    case mtsYAxis_Home:
        if (marlin_server_get_command() != MARLIN_CMD_G28) {
            state = mtsYAxis_Measure_BF_50mms;
            sg_sample_set(1 << Y_AXIS);
            endstops.enable(true);
            YAxis_start(XYAxis_get_fr(state), 1);
        }
        break;
    case mtsYAxis_Measure_BF_50mms:
    case mtsYAxis_Measure_BF_60mms:
    case mtsYAxis_Measure_BF_75mms:
    case mtsYAxis_Measure_BF_100mms:
        if (movesplanned == 0) {
            float fr = XYAxis_get_fr(state);
            YAxis_end(fr, 1);
            state = get_next_state();
            fr = XYAxis_get_fr(state);
            YAxis_start(fr, -1);
        }
        break;
    case mtsYAxis_Measure_FB_50mms:
    case mtsYAxis_Measure_FB_60mms:
    case mtsYAxis_Measure_FB_75mms:
    case mtsYAxis_Measure_FB_100mms:
        if (movesplanned == 0) {
            float fr = XYAxis_get_fr(state);
            YAxis_end(fr, -1);
            state = get_next_state();
            fr = XYAxis_get_fr(state);
            if (fr > 0)
                YAxis_start(fr, 1);
        }
        break;
    case mtsYAxis_Finished:
        endstops.enable(false);
        sg_sample_set(0);
        state = mtsFinish;
        break;

    case mtsFinish:
        if (fil_ok)
            f_close(&fil);
        marlin_server_set_exclusive_mode(0);
        state = mtsFinished;
        break;

    case mtsFinished:
        break;
    }
}

marlin_test_state_t CMarlinTest::get_next_state() {
    return (marlin_test_state_t)((int)state + 1);
}

void CMarlinTest::XAxis_start(float fr, int dir) {
    osDelay(100);
    start_time = HAL_GetTick();
    start_pos = stepper.position(X_AXIS);
    f_printf(&fil, "X-axis @%d mm/s %s\n", (int)fr, (dir == -1) ? "(from RIGHT to LEFT)" : "(from LEFT to RIGHT)");
    f_sync(&fil);
    sg_sample_count = 0;
    sg_sample_sum = 0;
    planner.synchronize();
    current_position.x += dir * 200;
    sg_sample_on = true;
    line_to_current_position(fr);
}

void CMarlinTest::XAxis_end(float fr, int dir) {
    sg_sample_on = false;
    osDelay(100);
    int32_t length = dir * (stepper.position(X_AXIS) - start_pos);
    uint32_t sg_avg = sg_sample_sum / sg_sample_count;
    f_printf(&fil, "measured length: %d\n", length);
    f_printf(&fil, "avg stallguard: %d\n\n", sg_avg);
    f_sync(&fil);
}

void CMarlinTest::YAxis_start(float fr, int dir) {
    osDelay(100);
    start_time = HAL_GetTick();
    start_pos = stepper.position(Y_AXIS);
    f_printf(&fil, "Y-axis @%d mm/s %s\n", (int)fr, (dir == -1) ? "(from BACK to FRONT)" : "(from FRONT to BACK)");
    f_sync(&fil);
    sg_sample_count = 0;
    sg_sample_sum = 0;
    planner.synchronize();
    current_position.y += dir * 200;
    sg_sample_on = true;
    line_to_current_position(fr);
}

void CMarlinTest::YAxis_end(float fr, int dir) {
    sg_sample_on = false;
    osDelay(100);
    int32_t length = dir * (stepper.position(Y_AXIS) - start_pos);
    uint32_t sg_avg = sg_sample_sum / sg_sample_count;
    f_printf(&fil, "measured length: %d\n", length);
    f_printf(&fil, "avg stallguard: %d\n\n", sg_avg);
    f_sync(&fil);
}

float CMarlinTest::XYAxis_get_fr(marlin_test_state_t state) {
    static const int XY_speed_table[] = { 50, 60, 75, 100 };
    if ((state >= mtsXAxis_Measure_RL_50mms) && (state <= mtsXAxis_Measure_LR_100mms))
        return XY_speed_table[(state - mtsXAxis_Measure_RL_50mms) / 2];
    if ((state >= mtsYAxis_Measure_BF_50mms) && (state <= mtsYAxis_Measure_FB_100mms))
        return XY_speed_table[(state - mtsYAxis_Measure_BF_50mms) / 2];
    return 0;
}

void CMarlinTest::sg_sample_set(uint8_t axis_mask) {
    if (INIT_TRINAMIC_FROM_MARLIN_ONLY == 0) {
        tmc_set_sg_mask(axis_mask);
        tmc_set_sg_sampe_cb(axis_mask ? sg_sample : nullptr);
    }
}

void CMarlinTest::sg_sample(uint8_t axis, uint16_t sg) {
    if (!MarlinTest.sg_sample_on)
        return;
    int32_t pos = stepper.position(X_AXIS);
    if (MarlinTest.fil_ok)
        f_printf(&MarlinTest.fil, "%u %d %d\n", HAL_GetTick() - MarlinTest.start_time, pos, sg);
    MarlinTest.sg_sample_count++;
    MarlinTest.sg_sample_sum += sg;
    //	_dbg("%u %d %d", HAL_GetTick() - start_time, pos, sg);
}

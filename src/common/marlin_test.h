// marlin_test.h
#pragma once

#include <inttypes.h>
#include "ff.h"

typedef enum {
    mtsIdle = 0,
    mtsStart,
    mtsInit,
    mtsInit_Home,
    mtsXAxis_Start,
    mtsXAxis_Home,
    mtsXAxis_Measure_RL_50mms,
    mtsXAxis_Measure_LR_50mms,
    mtsXAxis_Measure_RL_60mms,
    mtsXAxis_Measure_LR_60mms,
    mtsXAxis_Measure_RL_75mms,
    mtsXAxis_Measure_LR_75mms,
    mtsXAxis_Measure_RL_100mms,
    mtsXAxis_Measure_LR_100mms,
    mtsXAxis_Finished,
    mtsYAxis_Start,
    mtsYAxis_Home,
    mtsYAxis_Measure_BF_50mms,
    mtsYAxis_Measure_FB_50mms,
    mtsYAxis_Measure_BF_60mms,
    mtsYAxis_Measure_FB_60mms,
    mtsYAxis_Measure_BF_75mms,
    mtsYAxis_Measure_FB_75mms,
    mtsYAxis_Measure_BF_100mms,
    mtsYAxis_Measure_FB_100mms,
    mtsYAxis_Finished,
    mtsFinish,
    mtsFinished,
} marlin_test_state_t;

typedef enum {
    mttNone = 0,
    mttInit,
    mttXAxis,
    mttYAxis,
    mttZAxis,
} marlin_test_type_t;


class CMarlinTest
{
public:
	CMarlinTest();
	bool isInProgress();
	bool start();
	void loop();
protected:
	void XAxis_start(float fr, int dir);
	void XAxis_end(float fr, int dir);
	void YAxis_start(float fr, int dir);
	void YAxis_end(float fr, int dir);
protected:
	static void sg_sample(uint8_t axis, uint16_t sg);
protected:
    marlin_test_state_t state;                  // test state (Xaxis_start, Xaxis_home, ...)
    marlin_test_type_t type;                    // current test type (none, init, xaxis...)
    uint64_t mask;                              // mask of tests to be done after test_start

    uint32_t start_time;
    int32_t start_pos;
    FIL fil;
    bool fil_ok;
    bool sg_sample_on = false;
    uint16_t sg_sample_count;
    uint32_t sg_sample_sum;
};

extern CMarlinTest MarlinTest;

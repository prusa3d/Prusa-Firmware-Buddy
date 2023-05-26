/**
 * @file
 */

#pragma once
#include <option/has_accelerometer.h>

#if HAS_ACCELEROMETER()
    #include "SparkFunLIS2DH.h"

extern LIS2DH accelerometer;
#endif

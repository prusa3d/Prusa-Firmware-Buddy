/**
 * @file png_measure.cpp
 */

#include "png_measure.hpp"
#include "timing.h"
#include "log.h"

LOG_COMPONENT_REF(GUI);

PNGMeasure::PNGMeasure() {
    start_us = ticks_us();
}

PNGMeasure::~PNGMeasure() {
    log_debug(GUI, "PNG draw took %u us", ticks_us() - start_us);
}

/**
 * This file serves as place to add defines or includes for configuration store structure
 */
#pragma once
#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
#include "config_features.h"
#include "mem_config_item.hpp"
#include "feature/has_selftest.h"
#include "footer_eeprom.hpp"

#include "../../lib/Marlin/Marlin/src/module/temperature.h"

static constexpr float default_axis_steps_flt[4] = DEFAULT_AXIS_STEPS_PER_UNIT;
static constexpr int crash_sens[2] =
#if ENABLED(CRASH_RECOVERY)
    CRASH_STALL_GUARD;
#else
    { 0, 0 };
#endif // ENABLED(CRASH_RECOVERY)

static constexpr int crash_period[2] =
#if ENABLED(CRASH_RECOVERY)
    CRASH_PERIOD;
#else
    { 0, 0 };
#endif // ENABLED(CRASH_RECOVERY)

static constexpr bool crash_filter_def_val =
#if ENABLED(CRASH_RECOVERY)
    CRASH_FILTER;
#else
    false;
#endif // ENABLED(CRASH_RECOVERY)

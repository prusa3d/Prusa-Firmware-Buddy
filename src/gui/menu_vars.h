// menu_vars.h - shared arrays to be used in menus
#pragma once

#include "i18n.h"

// TODO: Move Z_OFFSET_MIN/MAX somewhere better
#include <common/sheet.hpp>

//-----------------------------------------------------------------------------
// stringize macros
#define QUOTE_ME(x) #x
#define STR(x)      QUOTE_ME(x)

// axis length [mm]
static constexpr int axis_steps_per_unit[] = DEFAULT_AXIS_STEPS_PER_UNIT;

static constexpr float nozzle_to_probe[3] = NOZZLE_TO_PROBE_OFFSET;

static constexpr float z_offset_step = 1.0F / float(axis_steps_per_unit[2]);
static constexpr float z_offset_min = Z_OFFSET_MIN;
static constexpr float z_offset_max = Z_OFFSET_MAX;

#define z_offset_def nozzle_to_probe[2]

#include <array>
#include <cstdint>
using std::size_t;
struct MenuVars {
    constexpr static const size_t AXIS_CNT = 4;
    constexpr static const size_t RANGE_SZ = 3;
    constexpr static const char *const labels[] = { N_("Move X"), N_("Move Y"), N_("Move Z"), N_("Move E") };

    static const std::pair<int, int> crash_sensitivity_range;

    static std::pair<int, int> axis_range(uint8_t axis);

private:
    MenuVars() = delete;
};

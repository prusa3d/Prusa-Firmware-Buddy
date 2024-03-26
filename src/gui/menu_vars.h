// menu_vars.h - shared arrays to be used in menus
#pragma once
#include "SteelSheets.hpp"
#include "i18n.h"
#include "file_list_defs.h"

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
    // TODO This is not a feedrate, it is a print speed. And it does not make any sense.
#if (PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_MK3_5 || PRINTER_IS_PRUSA_XL || PRINTER_IS_PRUSA_iX)
    constexpr static std::array<int, RANGE_SZ> feedrate_range = { 50, 1000, 1 };
#else
    constexpr static std::array<int, RANGE_SZ> feedrate_range = { 10, 255, 1 };
#endif
    static const std::array<int, AXIS_CNT> GetManualFeedrate();
    static const std::array<char, AXIS_CNT> GetAxisLetters();
    static const std::array<int, RANGE_SZ> GetCrashSensitivity();
    static const std::array<int, RANGE_SZ> GetNozzleRange();
    static const std::array<int, RANGE_SZ> GetBedRange();
    static const std::array<int, RANGE_SZ> GetMaximumZRange();
    static const std::array<std::array<int, RANGE_SZ>, AXIS_CNT> GetAxisRanges();

private:
    MenuVars() = delete;
};

#include "selftest_axis.h"
#include "../../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"

using namespace selftest;

static constexpr float XYfr_table[] = { HOMING_FEEDRATE_XY / 60 };
static constexpr size_t xy_fr_table_size = sizeof(XYfr_table) / sizeof(XYfr_table[0]);

//reads data from eeprom, cannot be constexpr
// FIXME: remove fixed lengths once the printer specs are finalized
const AxisConfig_t selftest::Config_XAxis = { .partname = "X-Axis", .length = X_BED_SIZE, .fr_table_fw = XYfr_table, .fr_table_bw = XYfr_table, .length_min = 230, .length_max = 290, .axis = X_AXIS, .steps = xy_fr_table_size * 2, .movement_dir = 1 };
const AxisConfig_t selftest::Config_YAxis = { .partname = "Y-Axis", .length = Y_BED_SIZE, .fr_table_fw = XYfr_table, .fr_table_bw = XYfr_table, .length_min = 190, .length_max = 250, .axis = Y_AXIS, .steps = xy_fr_table_size * 2, .movement_dir = 1 };

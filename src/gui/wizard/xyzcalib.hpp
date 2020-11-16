// xyzcalib.hpp
#pragma once

#include <inttypes.h>
#include "gui.hpp"
#include "wizard_types.hpp"

StateFncData StateFnc_XYZCALIB_INIT(StateFncData last_run);
StateFncData StateFnc_XYZCALIB_HOME(StateFncData last_run);
StateFncData StateFnc_XYZCALIB_Z(StateFncData last_run);
StateFncData StateFnc_XYZCALIB_XY_MSG_CLEAN_NOZZLE(StateFncData last_run);
StateFncData StateFnc_XYZCALIB_XY_MSG_IS_SHEET(StateFncData last_run);
StateFncData StateFnc_XYZCALIB_XY_MSG_REMOVE_SHEET(StateFncData last_run);
StateFncData StateFnc_XYZCALIB_XY_MSG_PLACE_PAPER(StateFncData last_run);
StateFncData StateFnc_XYZCALIB_XY_SEARCH(StateFncData last_run);
StateFncData StateFnc_XYZCALIB_XY_MSG_PLACE_SHEET(StateFncData last_run);
StateFncData StateFnc_XYZCALIB_XY_MEASURE(StateFncData last_run);
StateFncData StateFnc_XYZCALIB_RESULT(StateFncData last_run);

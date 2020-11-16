// xyzcalib.hpp
#pragma once

#include <inttypes.h>
#include "gui.hpp"
#include "wizard_types.hpp"

WizardState_t StateFnc_XYZCALIB_INIT();
WizardState_t StateFnc_XYZCALIB_HOME();
WizardState_t StateFnc_XYZCALIB_Z();
WizardState_t StateFnc_XYZCALIB_XY_MSG_CLEAN_NOZZLE();
WizardState_t StateFnc_XYZCALIB_XY_MSG_IS_SHEET();
WizardState_t StateFnc_XYZCALIB_XY_MSG_REMOVE_SHEET();
WizardState_t StateFnc_XYZCALIB_XY_MSG_PLACE_PAPER();
WizardState_t StateFnc_XYZCALIB_XY_SEARCH();
WizardState_t StateFnc_XYZCALIB_XY_MSG_PLACE_SHEET();
WizardState_t StateFnc_XYZCALIB_XY_MEASURE();
WizardState_t StateFnc_XYZCALIB_RESULT();

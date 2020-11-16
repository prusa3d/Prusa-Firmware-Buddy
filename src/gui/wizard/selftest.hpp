// selftest.hpp
#pragma once
#include "wizard_types.hpp"

StateFncData StateFnc_SELFTEST_FAN(StateFncData last_run);
StateFncData StateFnc_SELFTEST_X(StateFncData last_run);
StateFncData StateFnc_SELFTEST_Y(StateFncData last_run);
StateFncData StateFnc_SELFTEST_Z(StateFncData last_run);
StateFncData StateFnc_SELFTEST_XYZ(StateFncData last_run);
StateFncData StateFnc_SELFTEST_RESULT(StateFncData last_run);
StateFncData StateFnc_SELFTEST_AND_XYZCALIB(StateFncData last_run);
StateFncData StateFnc_SELFTEST_TEMP(StateFncData last_run);

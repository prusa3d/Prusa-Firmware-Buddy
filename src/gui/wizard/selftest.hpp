// selftest.hpp
#pragma once
#include "selftest_cool.hpp"
#include "selftest_temp.hpp"

StateFncData StateFnc_SELFTEST_FAN(StateFncData last_run);
StateFncData StateFnc_SELFTEST_X(StateFncData last_run);
StateFncData StateFnc_SELFTEST_Y(StateFncData last_run);
StateFncData StateFnc_SELFTEST_Z(StateFncData last_run);
StateFncData StateFnc_SELFTEST_XYZ(StateFncData last_run);
StateFncData StateFnc_SELFTEST_PASS(StateFncData last_run);
StateFncData StateFnc_SELFTEST_FAIL(StateFncData last_run);
StateFncData StateFnc_SELFTEST_AND_XYZCALIB(StateFncData last_run);

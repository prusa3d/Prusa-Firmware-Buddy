// selftest.hpp
#pragma once
#include "selftest_cool.hpp"
#include "selftest_temp.hpp"
#include "selftest_fans_axis.hpp"

StateFncData StateFnc_SELFTEST_INIT(StateFncData last_run);
StateFncData StateFnc_SELFTEST_PASS(StateFncData last_run);
StateFncData StateFnc_SELFTEST_FAIL(StateFncData last_run);
StateFncData StateFnc_SELFTEST_AND_XYZCALIB(StateFncData last_run);

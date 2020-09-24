// firstlay.hpp
#pragma once
#include "wizard_types.hpp"

StateFncData StateFnc_FIRSTLAY_INIT(StateFncData last_run);
StateFncData StateFnc_FIRSTLAY_LOAD(StateFncData last_run);
StateFncData StateFnc_FIRSTLAY_MSBX_CALIB(StateFncData last_run);
StateFncData StateFnc_FIRSTLAY_MSBX_START_PRINT(StateFncData last_run);
StateFncData StateFnc_FIRSTLAY_PRINT(StateFncData last_run);
StateFncData StateFnc_FIRSTLAY_MSBX_REPEAT_PRINT(StateFncData last_run);
StateFncData StateFnc_FIRSTLAY_PASS(StateFncData last_run);
StateFncData StateFnc_FIRSTLAY_FAIL(StateFncData last_run);

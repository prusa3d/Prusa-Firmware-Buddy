#pragma once
#include "log.h"

//
// Log Platform Functions
//
// Those functions are to be implemented by the user of this component.
//

log_timestamp_t log_platform_timestamp_get();

log_task_id_t log_platform_task_id_get();

bool log_platform_is_low_on_resources();

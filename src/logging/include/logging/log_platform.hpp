#pragma once

#include <logging/log.hpp>

namespace logging {
//
// Log Platform Functions
//
// Those functions are to be implemented by the user of this component.
//

Timestamp log_platform_timestamp_get();

TaskId log_platform_task_id_get();

} // namespace logging

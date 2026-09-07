///@file
#pragma once

#include <utils/byte_utils.hpp>

namespace rtt_metrics {

/// Call exactly once, from the metrics task, before the first log_metric()/process_rtt_metrics_queue() call.
void init_rtt_metrics();

/// Must be called from a single consumer context only (the rtt_metrics task).
void log_metric(Bytes buffer);

} // namespace rtt_metrics

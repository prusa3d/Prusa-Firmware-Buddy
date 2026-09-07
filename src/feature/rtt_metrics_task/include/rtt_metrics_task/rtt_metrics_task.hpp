///@file
#pragma once

namespace rtt_metrics {

/// Capture the per-axis step counters and enqueue them for later reporting.
/// Meant to be called from the tick ISR, so it only reads and enqueues; the
/// serialization and RTT transfer happen in the rtt_metrics task.
void capture_stepper_positions();

} // namespace rtt_metrics

void rtt_metrics_task();

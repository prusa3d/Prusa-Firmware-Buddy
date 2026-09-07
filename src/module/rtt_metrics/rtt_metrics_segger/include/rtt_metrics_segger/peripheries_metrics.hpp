/// @file
#pragma once

#include <accelerometer/common_structs.hpp>
#include <rtt_metrics_structs/rtt_metrics_structs.hpp>

namespace rtt_metrics {
/// Enqueue a metric sample for later transmission. These are safe to call from
/// any context, including ISRs at any priority: they only push into a lock-free
/// SPSC queue and never touch SEGGER/FreeRTOS. Each function must be called
/// from a single producer context (see peripheries_metrics.cpp). Samples are
/// dropped on queue overflow.
void sample_accelerometer(const accelerometer::RawAcceleration &raw_acceleration);
void sample_loadcell_tared_z(const LoadcellTaredZ &tared_z);
void sample_stepper_positions(const StepperPositions &positions);

/// Must be called from a single consumer context
/// only (the rtt_metrics task), after init_rtt_metrics().
void process_rtt_metrics_queue();
} // namespace rtt_metrics

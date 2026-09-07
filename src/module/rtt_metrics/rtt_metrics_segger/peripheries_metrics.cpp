///@file
#include "include/rtt_metrics_segger/peripheries_metrics.hpp"
#include "include/rtt_metrics_segger/rtt_metrics_segger.hpp"

#include <cstdint>
#include <span>

#include <rtt_metrics_serialization/peripheries_serialization.hpp>
#include <rtt_metrics_structs/rtt_metrics_structs.hpp>
#include <timing.h>
#include <utils/atomic_circular_queue.hpp>
#include <utils/single_isr_producer_guard.hpp>

using namespace rtt_metrics;

namespace {

/// One SPSC queue per metric type. 1 type = 1 context
/// Each queue has headroom for >=30 ms of data.
AtomicCircularQueue<MetricWrapper<accelerometer::RawAcceleration, MetricType::raw_acceleration>, uint8_t, 64> accel_queue;
AtomicCircularQueue<MetricWrapper<LoadcellTaredZ, MetricType::loadcell_tared_z>, uint8_t, 32> loadcell_queue;
AtomicCircularQueue<MetricWrapper<StepperPositions, MetricType::stepper_positions>, uint8_t, 32> stepper_queue;

template <typename Queue>
void drain(Queue &queue) {
    while (!queue.isEmpty()) {
        const auto serialized = serialize_metric(queue.dequeue());
        log_metric(std::as_bytes(std::span { serialized.buffer.data(), serialized.written_size }));
    }
}

} // anonymous namespace

// Best-effort: drop the sample if the consumer can't keep up.
void rtt_metrics::sample_accelerometer(const accelerometer::RawAcceleration &raw_acceleration) {
    static SingleISRProducerGuard guard;
    guard.check();
    static_cast<void>(accel_queue.enqueue({ ticks_us(), raw_acceleration }));
}

void rtt_metrics::sample_loadcell_tared_z(const LoadcellTaredZ &tared_z) {
    static SingleISRProducerGuard guard;
    guard.check();
    static_cast<void>(loadcell_queue.enqueue({ ticks_us(), tared_z }));
}

void rtt_metrics::sample_stepper_positions(const StepperPositions &positions) {
    static SingleISRProducerGuard guard;
    guard.check();
    static_cast<void>(stepper_queue.enqueue({ ticks_us(), positions }));
}

void rtt_metrics::process_rtt_metrics_queue() {
    drain(accel_queue);
    drain(loadcell_queue);
    drain(stepper_queue);
}

#include "spi_task.hpp"

#include "hal_spi.hpp"
#include "timing.hpp"
#include "watchdog.hpp"

#include <freertos/binary_semaphore.hpp>
#include <freertos/critical_section.hpp>
#include <freertos/timing.hpp>
#include <utils/atomic_circular_queue.hpp>
#include <utils/storage/enum_bitset.hpp>

#include <atomic>
#include <utility>
#include <bsod/bsod.h>

namespace spi_task {
AtomicCircularQueue<uint32_t, uint8_t, 128> accel_samples;
AtomicCircularQueue<fifo_coder::LoadcellRecord, uint32_t, 32> loadcell_samples;
namespace {
    enum class Event : uint8_t {
        accel_data_ready = 1 << 0,
        loadcell_data_ready = 1 << 3,
    };

    constexpr Event operator&(std::underlying_type_t<Event> lhs, Event rhs) {
        return static_cast<Event>(lhs & std::to_underlying(rhs));
    }

    std::atomic<std::underlying_type_t<Event>> spi_events = 0;
    freertos::BinarySemaphore spi_task_semaphore {};

    namespace accel {
        uint32_t first_sample_timestamp_us = 0;
        uint32_t last_sample_timestamp_us = 0;
        size_t sample_count = 0;
        bool buffer_overflow = false;

        void get_sample() {
            {
                freertos::CriticalSection cs;
                last_sample_timestamp_us = timing::get_timestamp_us(); // narrowing OK, wraps in 71 minutes
                ++sample_count;
            }
            hal::spi::accel::Sample sample {};
            const bool sample_ok = hal::spi::accel::get_sample(sample);
            const bool sample_overrun = !sample_ok;

            const uint32_t x_10bit_encoded = ((uint32_t)sample.x & 0b1111'1111'1100'0000) >> 6;
            const uint32_t y_10bit_encoded = ((uint32_t)sample.y & 0b1111'1111'1100'0000) << 4;
            const uint32_t z_10bit_encoded = ((uint32_t)sample.z & 0b1111'1111'1100'0000) << 14;
            const uint32_t packed_sample = x_10bit_encoded | y_10bit_encoded | z_10bit_encoded
                | (buffer_overflow ? (1 << 30) : 0)
                | (sample_overrun ? (1 << 31) : 0);

            const bool buffer_ok = accel_samples.enqueue(packed_sample);
            buffer_overflow = !buffer_ok;
        }

        /// Note: first reading is inaccurate (measures from timestamp 0, not from sampling start).
        float measured_sampling_rate() {
            freertos::CriticalSection cs;
            const auto duration = last_sample_timestamp_us - first_sample_timestamp_us;
            const auto samples = sample_count;
            first_sample_timestamp_us = last_sample_timestamp_us;
            sample_count = 0;

            if (duration > 0) {
                return samples * 1000000.f / duration;
            } else {
                return 0;
            }
        }

    } // namespace accel

    namespace loadcell {
        std::atomic<uint32_t> last_sample_ts;
        size_t overflown_count = 0;

        void get_sample() {
            const auto sample = hal::spi::loadcell::get_sample();
            if (sample.has_value()) {
                uint32_t timestamp = loadcell::last_sample_ts.load();
                // Negate raw value — the loadcell has inverted polarity
                if (!loadcell_samples.enqueue({ .timestamp = timestamp, .loadcell_raw_value = -sample.value() })) {
                    ++overflown_count;
                }
            }
        }
    } // namespace loadcell
} // namespace

void run() {
    hal::spi::init_comm();

    FOREVER_WITH_WATCHDOG(100) {
        spi_task_semaphore.acquire();
        const auto events = spi_events.exchange(0);
        debug_assert(events != 0);
        if (static_cast<bool>(events & Event::accel_data_ready)) {
            accel::get_sample();
        }
        if (static_cast<bool>(events & Event::loadcell_data_ready)) {
            loadcell::get_sample();
        }
    }
}

void notify_accel_data_ready() {
    const auto was = spi_events.fetch_or(std::to_underlying(Event::accel_data_ready));
    if (was == 0) {
        spi_task_semaphore.release_from_isr();
    }
}

void notify_loadcell_data_ready() {
    loadcell::last_sample_ts.store(static_cast<uint32_t>(timing::get_timestamp_us()));
    const auto was = spi_events.fetch_or(std::to_underlying(Event::loadcell_data_ready));
    if (was == 0) {
        spi_task_semaphore.release_from_isr();
    }
}

float measured_sampling_rate() {
    return accel::measured_sampling_rate();
}

} // namespace spi_task

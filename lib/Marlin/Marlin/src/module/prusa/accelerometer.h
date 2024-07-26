/**
 * @file
 */
#pragma once
#include "../../inc/MarlinConfigPre.h"
#include <option/has_dwarf.h>
#include <option/has_local_accelerometer.h>
#include <option/has_remote_accelerometer.h>

static_assert(HAS_LOCAL_ACCELEROMETER() || HAS_REMOTE_ACCELEROMETER());

#if HAS_LOCAL_ACCELEROMETER()
    #include "SparkFunLIS2DH.h"
#elif HAS_REMOTE_ACCELEROMETER()
    #include <freertos/mutex.hpp>
    #include <common/circular_buffer.hpp>
    #include <puppies/fifo_coder.hpp>
#endif

// This class must not be instantiated globally, because (for MK3.5) it temporarily takes
// ownership of the tachometer pin and turns it into accelerometer chip select pin.
class PrusaAccelerometer {
private:
#if HAS_LOCAL_ACCELEROMETER()
    #if PRINTER_IS_PRUSA_MK3_5()
    buddy::hw::OutputEnabler output_enabler;
    buddy::hw::OutputPin output_pin;
    #endif
    LIS2DH accelerometer;
#endif

public:
#if HAS_LOCAL_ACCELEROMETER()
    using Acceleration = Fifo::Acceleration;
#else
    struct Acceleration {
        float val[3];
        bool buffer_overflow;
        bool sample_overrun;
    };
#endif

    enum class Error {
        none,
        communication,
        no_active_tool,
        busy,
        corrupted_buddy_overflow, // Data not consistent, sample missed on buddy
#if HAS_DWARF()
        corrupted_dwarf_overflow, // Data not consistent, sample missed on dwarf
        corrupted_transmission_error, // Data not consistent, sample possibly lost in transfer
#endif
        corrupted_sample_overrun, // Data not consistent, sample overrun
    };

    PrusaAccelerometer();
    ~PrusaAccelerometer();

    void clear();
    int get_sample(Acceleration &acceleration);
    float get_sampling_rate() const { return m_sampling_rate; }
    Error get_error() { return m_error; }

#if HAS_REMOTE_ACCELEROMETER()
    static void put_sample(common::puppies::fifo::AccelerometerXyzSample sample);
    static void mark_corrupted(const Error error);
    static void set_rate(float rate);
#endif
private:
    static Error m_error;
    static float m_sampling_rate;
#if HAS_LOCAL_ACCELEROMETER()
    Fifo m_fifo;
#elif HAS_REMOTE_ACCELEROMETER()
    // Mutex is very RAM (80B) consuming for this fast operation, consider switching to critical section
    static freertos::Mutex s_buffer_mutex;
    using Sample_buffer = CircularBuffer<common::puppies::fifo::AccelerometerXyzSample, 128>;
    static Sample_buffer *s_sample_buffer;
    Sample_buffer m_sample_buffer;
#endif
};

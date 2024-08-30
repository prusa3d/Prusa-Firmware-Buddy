/**
 * @file
 */
#pragma once
#include "../../inc/MarlinConfigPre.h"
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

/**
 * This class must not be instantiated globally, because (for MK3.5) it temporarily takes
 * ownership of the tachometer pin and turns it into accelerometer chip select pin.
 */
class PrusaAccelerometer {
public:
#if HAS_LOCAL_ACCELEROMETER()
    using Acceleration = Fifo::Acceleration;
#else
    struct Acceleration {
        float val[3];
    };
#endif

    enum class Error {
        none,
        communication,
#if HAS_REMOTE_ACCELEROMETER()
        no_active_tool,
        busy,
#endif
        overflow_sensor, ///< Data not consistent, sample overrun on accelerometer sensor
#if HAS_REMOTE_ACCELEROMETER()
        overflow_buddy, ///< Data not consistent, sample missed on buddy
        overflow_dwarf, ///< Data not consistent, sample missed on dwarf
        overflow_possible, ///< Data not consistent, sample possibly lost in transfer
#endif
    };

    PrusaAccelerometer();
    ~PrusaAccelerometer();

    /**
     * @brief Clear buffers and Overflow
     */
    void clear();

    /// Obtains one sample from the buffer and puts it to \param acceleration
    /// \returns 0 if the queue is empty and no sample was obtained otherwise a number >0 (number of samples remaining to be read + 1 - the just returned one)
    int get_sample(Acceleration &acceleration);

    float get_sampling_rate() const { return m_sampling_rate; }
    /**
     * @brief Get error
     *
     * Check after PrusaAccelerometer construction.
     * Check after measurement to see if it was valid.
     */
    Error get_error() const { return m_sample_buffer.error.get(); }

#if HAS_REMOTE_ACCELEROMETER()
    static void put_sample(common::puppies::fifo::AccelerometerXyzSample sample);
    static void set_rate(float rate);
    static void set_possible_overflow();
#endif

private:
    class ErrorImpl {
    public:
        ErrorImpl()
            : m_error(Error::none) {}
        void set(Error error) {
            if (Error::none == m_error) {
                m_error = error;
            }
        }
        Error get() const {
            return m_error;
        }
        void clear_overflow() {
            switch (m_error) {
            case Error::none:
            case Error::communication:
#if HAS_REMOTE_ACCELEROMETER()
            case Error::no_active_tool:
            case Error::busy:
#endif
                break;
            case Error::overflow_sensor:
#if HAS_REMOTE_ACCELEROMETER()
            case Error::overflow_buddy:
            case Error::overflow_dwarf:
            case Error::overflow_possible:
#endif
                m_error = Error::none;
                break;
            }
        }

    private:
        Error m_error;
    };

    void set_enabled(bool enable);
#if HAS_LOCAL_ACCELEROMETER()
    struct SampleBuffer {
        Fifo buffer;
        ErrorImpl error;
    };
    SampleBuffer m_sample_buffer;
    #if PRINTER_IS_PRUSA_MK3_5()
    buddy::hw::OutputEnabler output_enabler;
    buddy::hw::OutputPin output_pin;
    #endif
    LIS2DH accelerometer;
#elif HAS_REMOTE_ACCELEROMETER()
    // Mutex is very RAM (80B) consuming for this fast operation, consider switching to critical section
    static freertos::Mutex s_buffer_mutex;
    struct SampleBuffer {
        CircularBuffer<common::puppies::fifo::AccelerometerXyzSample, 128> buffer;
        ErrorImpl error;
    };
    static SampleBuffer *s_sample_buffer;
    SampleBuffer m_sample_buffer;
#endif
    static float m_sampling_rate;
};

/**
 * @file
 */
#pragma once

#include "../../inc/MarlinConfigPre.h"
#include <option/has_local_accelerometer.h>
#include <option/has_remote_accelerometer.h>
#include <Marlin/src/core/types.h>
#include <utils/enum_array.hpp>
#include <numbers>
#include <accelerometer/common_structs.hpp>
#include <bsod/bsod.h>

static_assert(HAS_LOCAL_ACCELEROMETER() || HAS_REMOTE_ACCELEROMETER());

#if HAS_LOCAL_ACCELEROMETER()
    #include <hwio_pindef.h>
#elif HAS_REMOTE_ACCELEROMETER()
    #include <freertos/mutex.hpp>
    #include <common/circular_buffer.hpp>
    #include <fifo_coder/fifo_coder.hpp>
#else
    #error "Why do you #include me?"
#endif

/**
 * This class must not be instantiated globally, because (for MK3.5) it temporarily takes
 * ownership of the tachometer pin and turns it into accelerometer chip select pin.
 *
 * On boards with a dedicated CS pin the underlying poller is long-lived (set up once at
 * boot and sampling continuously); each PrusaAccelerometer instance merely borrows it for
 * one measurement session. Only one instance may exist at a time (enforced at construction).
 */
class PrusaAccelerometer {
public:
    PrusaAccelerometer();
    ~PrusaAccelerometer();

    /**
     * @brief Clear buffers and Overflow
     */
    void clear();

    enum class GetSampleResult {
        ok,
        buffer_empty,
        error,
    };

    /// Obtains one sample from the buffer and puts it to \param raw_acceleration (if the results is ok).
    GetSampleResult get_sample(accelerometer::RawAcceleration &raw_acceleration);

    GetSampleResult get_sample_printer_coords(accelerometer::RawAcceleration &acceleration) {
        accelerometer::RawAcceleration sample;
        const GetSampleResult result = get_sample(sample);
        if (result == GetSampleResult::ok) {
            acceleration = to_printer_coords(sample);
        }
        return result;
    }

    GetSampleResult get_sample_motor_coords(accelerometer::RawAcceleration &acceleration) {
        accelerometer::RawAcceleration sample;
        const GetSampleResult result = get_sample(sample);
        if (result == GetSampleResult::ok) {
            acceleration = to_motor_coords(sample);
        }
        return result;
    }

    template <typename ACCELERATION>
    static ACCELERATION to_motor_coords(ACCELERATION &sample) {
        ACCELERATION out;
#if PRINTER_IS_PRUSA_iX()
        debug_assert(X_AXIS == A_AXIS && Y_AXIS == B_AXIS);
        // Accelerometer is fixed to the head in a way that is parallel to the logical axes and diagonal to the physical ones. Therefore, we need to perform a 45° rotation.
        constexpr float cos45 = std::numbers::sqrt2_v<float> / 2;
        constexpr float sin45 = std::numbers::sqrt2_v<float> / 2;
        out.val[A_AXIS] = static_cast<int16_t>((-sample.val[1]) * cos45 + sample.val[2] * sin45);
        out.val[B_AXIS] = static_cast<int16_t>((-sample.val[1]) * (-sin45) + sample.val[2] * cos45);
        out.val[Z_AXIS] = sample.val[0];
#elif PRINTER_IS_PRUSA_COREONE()
        debug_assert(X_AXIS == A_AXIS && Y_AXIS == B_AXIS);
        // Due to accelerometer being rotated (approx. 45̀̃° = in the same way) as the head, no rotation is necessary, apart from switching axes.
        out.val[A_AXIS] = sample.val[1];
        out.val[B_AXIS] = sample.val[0];
        out.val[Z_AXIS] = sample.val[2];
#elif PRINTER_IS_PRUSA_COREONEL()
        debug_assert(X_AXIS == A_AXIS && Y_AXIS == B_AXIS);
        out.val[A_AXIS] = -sample.val[1];
        out.val[B_AXIS] = -sample.val[0];
        out.val[Z_AXIS] = sample.val[2];
#elif PRINTER_IS_PRUSA_XL()
        constexpr float cos45 = std::numbers::sqrt2_v<float> / 2;
        constexpr float sin45 = std::numbers::sqrt2_v<float> / 2;
        out.val[X_AXIS] = static_cast<int16_t>(sample.val[2] * cos45 + sample.val[1] * sin45);
        out.val[Y_AXIS] = static_cast<int16_t>(sample.val[2] * (-sin45) + sample.val[1] * cos45);
        out.val[Z_AXIS] = sample.val[0];
#elif PRINTER_IS_PRUSA_MK4() || PRINTER_IS_PRUSA_MK3_5()
        // In MK printers the world and motors align
        out = to_printer_coords(sample);
#else
    #error
#endif
        return out;
    }

    template <typename ACCELERATION>
    static ACCELERATION to_printer_coords(ACCELERATION &sample) {
        ACCELERATION out;
#if PRINTER_IS_PRUSA_iX()
        // Accelerometer is fixed to the head in a way that is parallel to the logical axes. Therefore, just need to correctly swap the values.
        out.val[X_AXIS] = -sample.val[1];
        out.val[Y_AXIS] = sample.val[2];
        out.val[Z_AXIS] = sample.val[0];
#elif PRINTER_IS_PRUSA_COREONE()
        // Due to accelerometer being rotated (approx. 45° = in the same way as the motors), no rotation is necessary, apart from switching axes.
        constexpr float cos45 = std::numbers::sqrt2_v<float> / 2;
        constexpr float sin45 = std::numbers::sqrt2_v<float> / 2;
        out.val[X_AXIS] = static_cast<int16_t>(sample.val[1] * cos45 - sample.val[0] * sin45);
        out.val[Y_AXIS] = static_cast<int16_t>(sample.val[1] * sin45 + sample.val[0] * cos45);
        out.val[Z_AXIS] = sample.val[2];
#elif PRINTER_IS_PRUSA_COREONEL()
        // Accelerometer is fixed to the head in a way that is diagonal to the logical axes. Therefore, we need to perform a 45° rotation.
        constexpr float cos45 = std::numbers::sqrt2_v<float> / 2;
        constexpr float sin45 = std::numbers::sqrt2_v<float> / 2;
        out.val[X_AXIS] = static_cast<int16_t>((-sample.val[1]) * cos45 + sample.val[0] * sin45);
        out.val[Y_AXIS] = static_cast<int16_t>((+sample.val[1]) * sin45 + sample.val[0] * cos45);
        out.val[Z_AXIS] = sample.val[2];
#elif PRINTER_IS_PRUSA_XL()
        out.val[X_AXIS] = sample.val[2];
        out.val[Y_AXIS] = -sample.val[1];
        out.val[Z_AXIS] = -sample.val[0];
#elif PRINTER_IS_PRUSA_MK4()
        // Here we have a little conundrum. MK* attaches accelerometer to the head for X axis and then moves it to the bed for Y axis.
        // Though these values are both set here, there is no way we could read them both at the same time.
        out.val[X_AXIS] = sample.val[0];
        out.val[Y_AXIS] = sample.val[1];
        out.val[Z_AXIS] = -sample.val[2];
#elif PRINTER_IS_PRUSA_MK3_5()
        out.val[X_AXIS] = sample.val[1];
        out.val[Y_AXIS] = sample.val[1];
        // TODO find out the real angle
        constexpr float cos45 = std::numbers::sqrt2_v<float> / 2;
        constexpr float sin45 = std::numbers::sqrt2_v<float> / 2;
        out.val[Z_AXIS] = static_cast<int16_t>(sample.val[1] * cos45 + sample.val[2] * sin45);
#else
    #error
#endif
        return out;
    }

    float get_sampling_rate() const;
    /**
     * @brief Get error
     *
     * Check after PrusaAccelerometer construction.
     * Check after measurement to see if it was valid.
     */
    accelerometer::Error get_error() const;

    /// \returns string describing the error or \p nullptr
    const char *error_str() const;

    /// If \p get_error() is not \p None, calls \p report_func(error_str)
    /// \returns \p true if there was an error and \p report_func was called
    template <typename F>
    inline bool report_error(const F &report_func) const {
        if (const auto str = error_str()) {
            report_func(str);
            return true;
        } else {
            return false;
        }
    }

#if HAS_REMOTE_ACCELEROMETER()
    static void put_sample(fifo_coder::AccelerometerXyzSample sample);

    /**
     * @brief Set frequency of calling put_sample().
     * @param rate frequency [Hz]
     */
    static void set_rate(float rate);

    static void set_possible_overflow();
#endif

private:
    class ErrorImpl {
    public:
        ErrorImpl()
            : m_error(accelerometer::Error::none) {}
        void set(accelerometer::Error error) {
            if (accelerometer::Error::none == m_error) {
                m_error = error;
            }
        }
        accelerometer::Error get() const {
            return m_error;
        }
        void clear_overflow() {
            switch (m_error) {
            case accelerometer::Error::no_active_tool:
            case accelerometer::Error::busy:
#if !HAS_REMOTE_ACCELEROMETER()
                bsod_unreachable();
#endif
            case accelerometer::Error::none:
            case accelerometer::Error::communication:
            case accelerometer::Error::_cnt:
                break;

            case accelerometer::Error::overflow_buddy:
            case accelerometer::Error::overflow_dwarf:
            case accelerometer::Error::overflow_possible:
#if !HAS_REMOTE_ACCELEROMETER()
                bsod_unreachable();
#endif
            case accelerometer::Error::overflow_sensor:
                m_error = accelerometer::Error::none;
                break;
            }
        }

    private:
        accelerometer::Error m_error;
    };

    void set_enabled(bool enable);
#if HAS_LOCAL_ACCELEROMETER() && PRINTER_IS_PRUSA_MK3_5()
    buddy::hw::OutputEnabler output_enabler;
    buddy::hw::OutputPin output_pin;
#elif HAS_REMOTE_ACCELEROMETER()
    // Mutex is very RAM (80B) consuming for this fast operation, consider switching to critical section
    static freertos::Mutex s_buffer_mutex;
    struct SampleBuffer {
        CircularBuffer<fifo_coder::AccelerometerXyzSample, 128> buffer;
        ErrorImpl error;
    };
    static SampleBuffer *s_sample_buffer;
    SampleBuffer m_sample_buffer;
    static float m_sampling_rate;
#endif
};

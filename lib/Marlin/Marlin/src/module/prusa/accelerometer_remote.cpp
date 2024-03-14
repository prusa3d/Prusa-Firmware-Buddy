/**
 * @file
 */
#include "accelerometer.h"
#if ENABLED(REMOTE_ACCELEROMETER)
    #include "toolchanger.h"
    #include "accelerometer_utils.h"
    #include <puppies/Dwarf.hpp>
    #include "bsod.h"
    #include "../../core/serial.h"
    #include <mutex>

FreeRTOS_Mutex PrusaAccelerometer::s_buffer_mutex;
PrusaAccelerometer::Sample_buffer *PrusaAccelerometer::s_sample_buffer = nullptr;

/**
 * If this is the first instance of PrusaAccelerometer
 * assign address of our m_sample_buffer to s_sample_buffer
 * and enable active Dwarf accelerometer.
 * Do nothing otherwise.
 */
PrusaAccelerometer::PrusaAccelerometer() {
    m_error = Error::none;
    bool enable_accelerometer = false;
    {
        std::lock_guard lock(s_buffer_mutex);
        if (!s_sample_buffer) {
            s_sample_buffer = &m_sample_buffer;
            enable_accelerometer = true;
        }
        s_sample_buffer->clear();
    }
    if (enable_accelerometer) {
        buddy::puppies::Dwarf *dwarf = prusa_toolchanger.get_marlin_picked_tool();
        if (!dwarf) {
            m_error = Error::no_active_tool;
            return;
        }
        if (!dwarf->set_accelerometer(true)) {
            m_error = Error::communication;
            return;
        }
    } else {
        m_error = Error::busy;
    }
}
/**
 * If there is address of our m_sample_buffer in s_sample_buffer
 * remove it and disable Dwarf accelerometer.
 * Do nothing otherwise.
 */
PrusaAccelerometer::~PrusaAccelerometer() {
    bool disable_accelerometer = false;
    {
        std::lock_guard lock(s_buffer_mutex);
        if (&m_sample_buffer == s_sample_buffer) {
            s_sample_buffer = nullptr;
            disable_accelerometer = true;
        }
    }
    if (disable_accelerometer) {
        switch (m_error) {
        case Error::none:
        case Error::communication:
        case Error::corrupted_buddy_overflow:
        case Error::corrupted_dwarf_overflow:
        case Error::corrupted_sample_overrun:
        case Error::corrupted_transmission_error: {
            buddy::puppies::Dwarf *dwarf = prusa_toolchanger.get_marlin_picked_tool();
            if (!dwarf) {
                return;
            }
            if (!dwarf->set_accelerometer(false)) {
                SERIAL_ERROR_MSG("Failed to disable accelerometer, communication error");
            }
            return;
        }
        case Error::no_active_tool:
            return;
        case Error::busy:
            bsod("Unexpected");
        }
    }
}
void PrusaAccelerometer::clear() {
    // todo wait for for so many samples that it is assured
    // that even if all buffers were full we went through
    // all samples
    Acceleration acceleration;
    while (get_sample(acceleration))
        ;
}
int PrusaAccelerometer::get_sample(Acceleration &acceleration) {
    common::puppies::fifo::AccelerometerXyzSample sample;
    bool ret_val = m_sample_buffer.ConsumeFirst(sample);
    if (ret_val) {
        acceleration = AccelerometerUtils::unpack_sample(sample);
        if (acceleration.buffer_overflow) {
            mark_corrupted(Error::corrupted_dwarf_overflow);
        }
        if (acceleration.sample_overrun) {
            mark_corrupted(Error::corrupted_sample_overrun);
        }
    }
    return ret_val;
}
void PrusaAccelerometer::put_sample(common::puppies::fifo::AccelerometerXyzSample sample) {
    std::lock_guard lock(s_buffer_mutex);
    if (s_sample_buffer) {
        if (!s_sample_buffer->push_back_DontRewrite(sample)) {
            mark_corrupted(Error::corrupted_buddy_overflow);
        }
    }
}
void PrusaAccelerometer::mark_corrupted(const Error error) {
    assert(error == Error::corrupted_dwarf_overflow
        || error == Error::corrupted_buddy_overflow
        || error == Error::corrupted_transmission_error
        || error == Error::corrupted_sample_overrun);
    m_error = error;
}
PrusaAccelerometer::Error PrusaAccelerometer::m_error = Error::none;
#endif // ENABLED(REMOTE_ACCELEROMETER)

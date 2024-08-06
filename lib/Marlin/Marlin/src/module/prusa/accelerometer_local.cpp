#include "accelerometer.h"

#include <option/has_local_accelerometer.h>

static_assert(HAS_LOCAL_ACCELEROMETER());

PrusaAccelerometer::PrusaAccelerometer()
    : m_sample_buffer { accelerometer, {} }
#if PRINTER_IS_PRUSA_MK3_5()
    , output_enabler { buddy::hw::fanPrintTach, buddy::hw::Pin::State::high, buddy::hw::OMode::pushPull, buddy::hw::OSpeed::high }
    , output_pin { output_enabler.pin() }
    , accelerometer { output_pin }
#else
    , accelerometer { buddy::hw::acellCs }
#endif
{
    if (IMU_SUCCESS != accelerometer.begin()) {
        m_sample_buffer.error.set(Error::communication);
    }
    accelerometer.fifoBegin();
}

PrusaAccelerometer::~PrusaAccelerometer() {}

void PrusaAccelerometer::clear() {
    Acceleration acceleration;
    bool overrun;
    while (m_sample_buffer.buffer.get(acceleration, overrun))
        ;
    m_sample_buffer.error.clear_overflow();
}
int PrusaAccelerometer::get_sample(Acceleration &acceleration) {
    bool overrun;
    int retval = m_sample_buffer.buffer.get(acceleration, overrun);
    m_sampling_rate = m_sample_buffer.buffer.get_sampling_rate();
    if (overrun) {
        m_sample_buffer.error.set(Error::overflow_sensor);
    }
    return retval;
}
float PrusaAccelerometer::m_sampling_rate = 0;

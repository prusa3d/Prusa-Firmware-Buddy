#include "accelerometer.h"

#include <option/has_local_accelerometer.h>

static_assert(HAS_LOCAL_ACCELEROMETER());

PrusaAccelerometer::PrusaAccelerometer()
#if PRINTER_IS_PRUSA_MK3_5()
    : output_enabler { buddy::hw::fanPrintTach, buddy::hw::Pin::State::high, buddy::hw::OMode::pushPull, buddy::hw::OSpeed::high }
    , output_pin { output_enabler.pin() }
    , accelerometer { output_pin }
#else
    : accelerometer { buddy::hw::acellCs }
#endif
    , m_fifo(accelerometer) {
    m_error = Error::none;
    if (IMU_SUCCESS != accelerometer.begin()) {
        m_error = Error::communication;
    }
    accelerometer.fifoBegin();
}

PrusaAccelerometer::~PrusaAccelerometer() {}

void PrusaAccelerometer::clear() {
    Acceleration acceleration;
    while (m_fifo.get(acceleration))
        ;
}
int PrusaAccelerometer::get_sample(Acceleration &acceleration) {
    int new_samples = m_fifo.get(acceleration);
    m_sampling_rate = m_fifo.get_sampling_rate();
    return new_samples;
}
PrusaAccelerometer::Error PrusaAccelerometer::m_error = Error::none;
float PrusaAccelerometer::m_sampling_rate = 0;

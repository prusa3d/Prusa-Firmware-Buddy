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
    set_enabled(true);
}

PrusaAccelerometer::~PrusaAccelerometer() = default;

void PrusaAccelerometer::set_enabled(bool enable) {
    m_error = Error::none;
    if (enable) {
        if (IMU_SUCCESS != accelerometer.begin()) {
            m_error = Error::communication;
        } else {
            accelerometer.fifoClear();
            accelerometer.fifoBegin();
        }
    } else {
        accelerometer.end();
    }
}

void PrusaAccelerometer::clear() {
    accelerometer.fifoClear();
}
int PrusaAccelerometer::get_sample(Acceleration &acceleration) {
    return m_fifo.get(acceleration);
}
PrusaAccelerometer::Error PrusaAccelerometer::m_error = Error::none;
float PrusaAccelerometer::m_sampling_rate = 0;

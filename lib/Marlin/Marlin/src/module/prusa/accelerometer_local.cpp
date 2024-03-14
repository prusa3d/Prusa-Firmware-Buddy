/**
 * @file
 */
#include "accelerometer.h"
#if ENABLED(LOCAL_ACCELEROMETER)
    #include "main.hpp"

PrusaAccelerometer::PrusaAccelerometer()
    : m_fifo(accelerometer) {
    m_error = Error::none;
    if (IMU_SUCCESS != accelerometer.begin()) {
        m_error = Error::communication;
    }
    accelerometer.fifoBegin();
}

PrusaAccelerometer::~PrusaAccelerometer() {}

void PrusaAccelerometer::clear() {
    accelerometer.fifoClear();
}
int PrusaAccelerometer::get_sample(Acceleration &acceleration) {
    return m_fifo.get(acceleration);
}
PrusaAccelerometer::Error PrusaAccelerometer::m_error = Error::none;
#endif // ENABLED(LOCAL_ACCELEROMETER)

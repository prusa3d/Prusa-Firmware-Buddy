/**
 * @file
 */
#include "accelerometer.h"
#if ENABLED(LOCAL_ACCELEROMETER)
    #include "main.hpp"

PrusaAccelerometer::PrusaAccelerometer()
    : m_error(Error::none)
    , m_fifo(accelerometer) {
    accelerometer.begin();
    accelerometer.fifoBegin();
}

PrusaAccelerometer::~PrusaAccelerometer() {}

void PrusaAccelerometer::clear() {
    accelerometer.fifoClear();
}
int PrusaAccelerometer::get_sample(Acceleration &acceleration) {
    return m_fifo.get(acceleration);
}
#endif // ENABLED(LOCAL_ACCELEROMETER)

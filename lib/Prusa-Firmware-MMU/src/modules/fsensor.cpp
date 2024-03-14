/// @file fsensor.cpp
#include "fsensor.h"
#include "timebase.h"

namespace modules {
namespace fsensor {

FSensor fsensor;

void FSensor::Step() {
    debounce::Debouncer::Step(mt::timebase.Millis(), reportedFSensorState);
}

void FSensor::ProcessMessage(bool on) {
    reportedFSensorState = on;
}

} // namespace fsensor
} // namespace modules

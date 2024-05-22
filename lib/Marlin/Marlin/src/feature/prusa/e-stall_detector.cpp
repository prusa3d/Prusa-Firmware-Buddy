#include "e-stall_detector.h"
#include <option/has_loadcell.h>

#ifdef UNITTEST
extern unsigned estall_suppressed_trigger_count;
#else
    #include "log.h"

LOG_COMPONENT_REF(MarlinServer);
#endif

bool MotorStallFilter::ProcessSample(int32_t value) {
    //    constexpr std::array<float, 5> gaussianFilter = { 0.0065929F, 0.1939811F, 0.5988519F, 0.1939811F, 0.0065929F };
    //    constexpr std::array<float, 3> edgeFilter = { -1.F, 0.F, 1.F };
    // Hmm, doesn't give the same results like python numpy does
    // import numpy as np
    // gaussianFilter = np.array([0.0065929, 0.1939811, 0.5988519, 0.1939811, 0.0065929])
    // edgeFilter = np.array([-1, 0, 1])
    // combined_filter = np.convolve(gaussianFilter, edgeFilter, mode='same')
    // print(combined_filter)
    // [-0.1939811 -0.592259   0.         0.592259   0.1939811]
    //
    // I don't want to spend any more time on fiddling with the right implementation of 'same'-mode convolution of filters,
    // so let's return the expected filter numbers.
    // If someone knows how to compute it properly, please feel free to improve the code below

    //    constexpr std::array<float, 5> gaussianFilter = { 0.0065929F, 0.1939811F, 0.5988519F, 0.1939811F, 0.0065929F };
    //    constexpr std::array<float, 3> edgeFilter = { -1.F, 0.F, 1.F };
    //    std::array<float, 5> combinedFilter {};
    //    for (size_t i = 0; i < combinedFilter.size(); ++i) {
    //        float sum = 0.0F;
    //        for (size_t j = 0; j < edgeFilter.size(); ++j) {
    //            sum += gaussianFilter[i + j] * edgeFilter[j];
    //        }
    //        combinedFilter[i] = sum;
    //    }

    // Also, for some reason I had to invert the edge detector logic, but this way it works nicely
    constexpr std::array<float, 5> combinedFilter = { 0.1939811F, 0.592259F, 0.F, -0.592259F, -0.1939811F };

    // Shift existing values in the buffer using std::rotate
    std::rotate(buffer.begin(), buffer.begin() + 1, buffer.end());

    // Store the new value in the buffer
    buffer.back() = value;

#ifndef UNITTEST
    float // for regular FW build, filteredValue is a local variable (not a member of the class)
#endif
        filteredValue
        = 0.0F;
    // Apply the filter
    for (size_t i = 0; i < combinedFilter.size(); ++i) {
        filteredValue += combinedFilter[i] * buffer[i];
    }
    return filteredValue > detectionThreshold;
}

void EMotorStallDetector::SetEnabled(bool set) {
    if (enabled == set) {
        return;
    }

    enabled = set;

    if (set) {
        ClearDetected();
    }
}

#if HAS_LOADCELL()
void EMotorStallDetector::ProcessSample(int32_t value) {
    detected |= emf.ProcessSample(value);
}

bool EMotorStallDetector::Evaluate(bool movingE, bool directionE) {
    // only check the E-motor stall when it has to be doing something and it is PUSHING the filament
    // i.e. - prevent triggers on crashes during travel moves and retractions
    if (!movingE || !directionE) {
        // Clear detection flags if the motor is standing still or retracting
        ClearDetected();
        return false;
    }

    if (!detected) {
        return false;
    }

    if (blocked > 0 || reported) {
        return false;
    }

    // Block further reporting until the estall is handled
    reported = true;

    if (!enabled) {
    #ifdef UNITTEST
        estall_suppressed_trigger_count++;
    #else
        log_info(MarlinServer, "E-stall detected but suppressed");
    #endif
    }

    return enabled;
}
#else
// Empty implementation when there is no LoadCell available
// Shall drop all references to unused code along with it.
void EMotorStallDetector::ProcessSample(int32_t value) {}
bool EMotorStallDetector::Evaluate(bool movingE, bool directionE) { return false; }
#endif

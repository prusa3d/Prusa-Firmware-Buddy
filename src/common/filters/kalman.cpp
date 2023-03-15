#include "kalman.hpp"
#include "math.h"

KalmanFilter::KalmanFilter(const double error_estimate, const double error_measure, const double error_weight, predictor_t predictor)
    : error_estimate(error_estimate)
    , error_measure(error_measure)
    , current_estimate(0)
    , last_estimate(0)
    , error_weight(error_weight)
    , predictor(predictor) {}

double KalmanFilter::filter(const double value, const uint32_t now_us) {
    if (predictor) {
        last_estimate = predictor(last_estimate, now_us);
    }
    const double gain = error_estimate / (error_estimate + error_measure);
    current_estimate = last_estimate + gain * (value - last_estimate);
    error_estimate = (1.0 - gain) * error_estimate + fabs(last_estimate - current_estimate) * error_weight;
    last_estimate = current_estimate;
    return current_estimate;
}

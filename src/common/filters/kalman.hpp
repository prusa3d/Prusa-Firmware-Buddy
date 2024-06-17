#pragma once

#include <inplace_function.hpp>
#include <cstdint>

class KalmanFilter {
public:
    using predictor_t = stdext::inplace_function<double(double last_estimate, uint32_t now_us)>;

    KalmanFilter(
        const double error_estimate,
        const double error_measure,
        const double error_weight,
        const predictor_t predictor = nullptr);
    double filter(double value, uint32_t now_us);

private:
    double error_estimate;
    double error_measure;
    double current_estimate;
    double last_estimate;
    const double error_weight;
    const predictor_t predictor;
};

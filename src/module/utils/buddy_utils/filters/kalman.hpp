#pragma once

#include <inplace_function.hpp>
#include <cstdint>
#include <bsod/bsod.h>

class KalmanFilter {
public:
    /**
     * @brief Construct a new Kalman Filter object.
     * @param error_measure_ variance of the measurement [units_as_value^2]
     * @param error_weight_ input from prediction error to error estimate [units_as_value]
     * @param initial_error_estimate initial error estimate [units_as_value^2]
     * @param initial_value initial value of the filter [units_as_value]
     */
    KalmanFilter(double error_measure_, double error_weight_, double initial_error_estimate, double initial_value)
        : error_measure(error_measure_)
        , error_weight(error_weight_)
        , error_estimate(initial_error_estimate)
        , current_estimate(initial_value) {}

    /**
     * @brief Filter a new value.
     * @param value new value to filter
     * @param prediction prediction of the new value [units_as_value]
     * @return new filter output value [units_as_value]
     */
    double filter(double value, double prediction);

    /**
     * @brief Reset the filter to initial state.
     * @param initial_error_estimate initial error estimate [units_as_value^2]
     * @param initial_value initial value of the filter [units_as_value]
     */
    void reset(double initial_error_estimate, double initial_value);

    /**
     * @brief Current filter output.
     */
    [[nodiscard]] double value() const {
        return current_estimate;
    }

    /**
     * @brief Return the current error estimate
     * @return error variance [units_as_value^2]
     */
    [[nodiscard]] double error() const {
        return error_estimate;
    }

protected:
    const double error_measure; ///< Variance of the measurement [units_as_value^2]
    const double error_weight; ///< Input from prediction error to error estimate [units_as_value]
    double error_estimate; ///< Initial error estimate [units_as_value^2]
    double current_estimate; ///< Initial value of the filter [units_as_value]
};

class KalmanFilterCallback : public KalmanFilter {
public:
    using predictor_t = stdext::inplace_function<double(double last_estimate, uint32_t now_us)>;

    /**
     * @brief Construct a new Kalman Filter object with callback predictor.
     * @param error_measure variance of the measurement [units_as_value^2]
     * @param error_weight input from prediction error to error estimate [units_as_value]
     * @param initial_error_estimate initial error estimate [units_as_value^2]
     * @param initial_value initial value of the filter [units_as_value]
     * @param predictor callback to predict the new value
     */
    KalmanFilterCallback(double error_measure, double error_weight, double initial_error_estimate, double initial_value, const predictor_t predictor_)
        : KalmanFilter(error_measure, error_weight, initial_error_estimate, initial_value)
        , predictor(predictor_) {
        debug_assert(predictor);
    }

    /**
     * @brief Filter a new value using the predictor callback.
     * @param value new value to filter
     * @param now_us current time, input to the predictor function [us]
     * @return new filter output value [units_as_value]
     */
    double filter(double value, uint32_t now_us);

private:
    const predictor_t predictor;
};

#include <algorithm>
#include <chrono>
#include <iostream>
#include <utility>
#include <vector>
#include <cmath>
#include <numbers>

#include <signal_processing/pipeline.hpp>
#include <signal_processing/filters.hpp>
#include <signal_processing/generators.hpp>
#include <module/signal2step.hpp>
#include <core/types.h>

// Trapezoidal velocity profile generator
// Accelerates, moves at constant speed, then decelerates
template <typename T>
class TrapezoidalVelocity {
public:
    using sample_type = T;

    TrapezoidalVelocity(T max_velocity, T acceleration, T total_distance, sp::SamplingFreq sampling_freq)
        : max_velocity(max_velocity)
        , acceleration(acceleration)
        , sampling_freq_value(sampling_freq) {

        // Calculate trapezoidal profile parameters
        T accel_time = max_velocity / acceleration;
        T accel_distance = 0.5f * acceleration * accel_time * accel_time;

        // Check if we can reach max velocity
        if (2.0f * accel_distance > total_distance) {
            // Triangular profile - don't reach max velocity
            accel_distance = total_distance / 2.0f;
            accel_time = std::sqrt(2.0f * accel_distance / acceleration);
            max_velocity = acceleration * accel_time;
            constant_time = 0.0f;
        } else {
            // Full trapezoidal profile
            T constant_distance = total_distance - 2.0f * accel_distance;
            constant_time = constant_distance / max_velocity;
        }

        T decel_time = accel_time;

        accel_samples = static_cast<std::size_t>(accel_time * sampling_freq);
        constant_samples = static_cast<std::size_t>(constant_time * sampling_freq);
        decel_samples = static_cast<std::size_t>(decel_time * sampling_freq);
        total_samples = accel_samples + constant_samples + decel_samples;
    }

    T next() {
        std::size_t sample = current_sample++;
        T dt = 1.0f / sampling_freq_value;

        if (sample < accel_samples) {
            // Acceleration phase
            return acceleration * sample * dt;
        } else if (sample < accel_samples + constant_samples) {
            // Constant velocity phase
            return max_velocity;
        } else if (sample < total_samples) {
            // Deceleration phase
            std::size_t decel_sample = sample - accel_samples - constant_samples;
            T remaining_time = (decel_samples - decel_sample) * dt;
            return acceleration * remaining_time;
        }
        return 0.0f;
    }

    sp::pipe::PollResult poll() {
        return current_sample < total_samples ? sp::pipe::PollResult::ready : sp::pipe::PollResult::done;
    }

    sp::SamplingFreq sampling_freq() const {
        return sampling_freq_value;
    }

private:
    T max_velocity;
    T acceleration;
    T constant_time;
    sp::SamplingFreq sampling_freq_value;
    std::size_t accel_samples = 0;
    std::size_t constant_samples = 0;
    std::size_t decel_samples = 0;
    std::size_t total_samples = 0;
    std::size_t current_sample = 0;
};

auto make_trapezoid(float max_velocity, float acceleration, float total_distance, sp::SamplingFreq sampling_freq) {
    return TrapezoidalVelocity<float>(max_velocity, acceleration, total_distance, sampling_freq);
}

auto invert = sp::pipe::transform([](float v) { return -v; });

int main() {
    using namespace std::chrono_literals;

    constexpr float SAMPLING_FREQ = 1000.0f;
    constexpr float MM_PER_STEP = 0.01f;

    // Movement parameters
    constexpr float X_MAX_VEL = 50.0f;
    constexpr float X_ACCEL = 100.0f;
    constexpr float X_DISTANCE = 100.0f;

    constexpr float Y_MAX_VEL = 30.0f;
    constexpr float Y_ACCEL = 80.0f;
    constexpr float Y_DISTANCE = 50.0f;

    constexpr float E_START_FREQ = 2.0f;
    constexpr float E_END_FREQ = 2.0f;
    constexpr float E_AMPLITUDE = 5.0f;

    constexpr float duration_s = 5.f;
    constexpr auto duration = 5s;

    // Create generators for velocity
    auto x_vel = make_trapezoid(X_MAX_VEL, X_ACCEL, X_DISTANCE, SAMPLING_FREQ);
    auto y_vel = sp::pipe::chain(
        make_trapezoid(Y_MAX_VEL, Y_ACCEL, Y_DISTANCE, SAMPLING_FREQ),
        make_trapezoid(Y_MAX_VEL, Y_ACCEL, Y_DISTANCE, SAMPLING_FREQ) | invert);
    auto z_vel = sp::pipe::make_constant<float>(0.0f, SAMPLING_FREQ) | sp::pipe::take(duration);
    auto e_vel = sp::pipe::make_linear_chirp<float>(
                     E_START_FREQ, E_END_FREQ, E_AMPLITUDE,
                     duration, SAMPLING_FREQ)
        | sp::pipe::transform([](float v) { return v + E_AMPLITUDE + 1; });

    // Output metadata
    std::cout << "# SAMPLING_FREQ=" << SAMPLING_FREQ << "\n";
    std::cout << "# MM_PER_STEP=" << MM_PER_STEP << "\n";
    std::cout << "# DURATION=" << duration_s << "\n";
    std::cout << "# X_MAX_VEL=" << X_MAX_VEL << "\n";
    std::cout << "# Y_MAX_VEL=" << Y_MAX_VEL << "\n";
    std::cout << "# E_START_FREQ=" << E_START_FREQ << "\n";
    std::cout << "# E_END_FREQ=" << E_END_FREQ << "\n";

    auto motion = signal2step::fuse_xyze(
        x_vel | sp::pipe::integrate(0.0f),
        y_vel | sp::pipe::integrate(0.0f),
        z_vel | sp::pipe::integrate(0.0f),
        e_vel | sp::pipe::integrate(0.0f));

    std::vector<xyze_float_t> positions;
    while (motion.poll() == sp::pipe::PollResult::ready) {
        auto pos = motion.next();
        positions.push_back(pos);
    }

    struct StepEvent {
        std::size_t sample_index;
        uint32_t timestamp_us;
        AxisEnum axis;
        bool direction;
    };
    std::vector<StepEvent> steps;

    struct Collector {
        std::vector<StepEvent> &steps;

        void step(uint32_t timestamp_us, AxisEnum axis, bool direction) {
            std::size_t sample = static_cast<std::size_t>(timestamp_us / 1000.0f * SAMPLING_FREQ / 1000.0f);
            steps.push_back({ sample, timestamp_us, axis, direction });
        }
        void sample_end(uint32_t) {}
        void wait_for_samples() {}
        bool aborted() const { return false; }
    } collector { steps };

    signal2step::detail::generate_steps(
        sp::pipe::make_source(positions, SAMPLING_FREQ)
            | sp::pipe::transform([](const xyze_float_t &pos) { return signal2step::detail::machine_to_motor(pos); }),
        { MM_PER_STEP, MM_PER_STEP, MM_PER_STEP, MM_PER_STEP },
        collector);

    // Output positions CSV
    std::cout << "\n# POSITIONS\n";
    std::cout << "Sample,Time,X,Y,Z,E,VelX,VelY,VelZ,VelE\n";

    for (std::size_t i = 0; i < positions.size(); ++i) {
        float time = static_cast<float>(i) / SAMPLING_FREQ;

        // Calculate velocity from position differences
        float vx = 0.0f, vy = 0.0f, vz = 0.0f, ve = 0.0f;
        if (i > 0) {
            vx = (positions[i].x - positions[i - 1].x) * SAMPLING_FREQ;
            vy = (positions[i].y - positions[i - 1].y) * SAMPLING_FREQ;
            vz = (positions[i].z - positions[i - 1].z) * SAMPLING_FREQ;
            ve = (positions[i].e - positions[i - 1].e) * SAMPLING_FREQ;
        }

        std::cout << i << ","
                  << time << ","
                  << positions[i].x << ","
                  << positions[i].y << ","
                  << positions[i].z << ","
                  << positions[i].e << ","
                  << vx << ","
                  << vy << ","
                  << vz << ","
                  << ve << "\n";
    }

    // Output steps CSV
    std::cout << "\n# STEPS\n";
    std::cout << "Sample,Time,Axis,Direction\n";
    for (const auto &step : steps) {
        float time = static_cast<float>(step.timestamp_us) / 1000000.0f;
        const char *axis_name;
        switch (step.axis) {
        case X_AXIS:
            axis_name = "X";
            break;
        case Y_AXIS:
            axis_name = "Y";
            break;
        case Z_AXIS:
            axis_name = "Z";
            break;
        case E_AXIS:
            axis_name = "E";
            break;
        default:
            axis_name = "?";
            break;
        }

        std::cout << step.sample_index << ","
                  << time << ","
                  << axis_name << ","
                  << (step.direction ? "1" : "-1") << "\n";
    }

    return 0;
}

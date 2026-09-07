#pragma once

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <numbers>
#include <random>
#include <signal_processing/math.hpp>
#include <signal_processing/pipeline.hpp>
#include <bsod/bsod.h>

namespace sp {

// Linear chirp: frequency increases linearly with time
struct LinearPhase {
    template <typename T>
    static T calculate(T t, T duration, T start_freq, T end_freq) {
        return T { 2 } * std::numbers::pi_v<T> * (start_freq * t + (end_freq - start_freq) * t * t / (T { 2 } * duration));
    }
};

// Quadratic chirp: frequency increases quadratically with time
struct QuadraticPhase {
    template <typename T>
    static T calculate(T t, T duration, T start_freq, T end_freq) {
        return T { 2 } * std::numbers::pi_v<T> * (start_freq * t + (end_freq - start_freq) * t * t * t / (T { 3 } * duration * duration));
    }
};

// A source that generates a chirp (sine wave with varying frequency)
// from start_freq to end_freq over the specified duration.
// The frequency progression is determined by the PhasePolicy.
template <typename T, typename PhasePolicy = LinearPhase>
class Chirp {
public:
    using sample_type = T;

    Chirp(T start_freq, T end_freq, T amplitude,
        sp::Duration duration, sp::SamplingFreq sampling_freq)
        : start_freq(start_freq)
        , end_freq(end_freq)
        , amplitude(amplitude)
        , rate(static_cast<T>(sampling_freq))
        , sampling_freq_value(sampling_freq) {
        debug_assert(sampling_freq > sp::SamplingFreq { 0 });
        const float duration_seconds = std::chrono::duration<float>(duration).count();
        debug_assert(duration_seconds > 0.f);
        total_samples = static_cast<std::size_t>(duration_seconds * static_cast<T>(sampling_freq));
        debug_assert(total_samples > 0);
        duration_in_seconds = static_cast<T>(total_samples) / rate;
    }

    T next() {
        debug_assert(poll() == sp::pipe::PollResult::ready);
        const T t = static_cast<T>(current_sample) / rate;
        const T phase = PhasePolicy::calculate(t, duration_in_seconds, start_freq, end_freq);

        ++current_sample;
        return amplitude * sp::sin(phase);
    }

    sp::pipe::PollResult poll() {
        return current_sample < total_samples ? sp::pipe::PollResult::ready : sp::pipe::PollResult::done;
    }

    sp::SamplingFreq sampling_freq() const {
        return sampling_freq_value;
    }

private:
    T start_freq;
    T end_freq;
    T amplitude;
    T rate;
    T duration_in_seconds;
    sp::SamplingFreq sampling_freq_value;
    std::size_t total_samples = 0;
    std::size_t current_sample = 0;
};

// An infinite source that generates Gaussian white noise with specified RMS.
template <typename T>
class GaussianNoise {
public:
    using sample_type = T;

    // Constructor with random seed
    explicit GaussianNoise(T rms, sp::SamplingFreq sampling_freq)
        : sampling_freq_value(sampling_freq) {
        debug_assert(rms > T { 0 });
        std::random_device rd;
        state = std::make_unique<State>(T { 0 }, rms);
        state->rng.seed(rd());
    }

    // Constructor with deterministic seed (for reproducible results)
    GaussianNoise(T rms, std::uint32_t seed, sp::SamplingFreq sampling_freq)
        : sampling_freq_value(sampling_freq) {
        debug_assert(rms > T { 0 });
        state = std::make_unique<State>(T { 0 }, rms);
        state->rng.seed(seed);
    }

    T next() {
        return state->dist(state->rng);
    }

    sp::pipe::PollResult poll() {
        return sp::pipe::PollResult::ready;
    }

    sp::SamplingFreq sampling_freq() const {
        return sampling_freq_value;
    }

private:
    // We store the state on heap as the mt19937 generator is quite large
    // (approx 2 kB). Hence, storing it on embedded stack isn't a good idea.
    struct State {
        State(T mean, T rms)
            : dist(mean, rms) {}

        std::mt19937 rng;
        std::normal_distribution<T> dist;
    };

    std::unique_ptr<State> state;
    sp::SamplingFreq sampling_freq_value;
};

// An infinite source that generates approximate Gaussian white noise with
// specified RMS using a fast PRNG and CLT approximation. RNG: xorshift32
// (period 2^32-1), output converted to uniform [0,1). Gaussian approximation:
// sum of 12 uniforms minus 6 (CLT), scaled by RMS. Can be considered as much
// faster alternative to GaussianNoise.
template <typename T>
class FastGaussianNoise {
public:
    using sample_type = T;

    explicit FastGaussianNoise(T rms, sp::SamplingFreq sampling_freq)
        : rms(rms)
        , sampling_freq_value(sampling_freq) {
        debug_assert(rms > T { 0 });
        std::random_device rd;
        seed(static_cast<std::uint32_t>(rd()));
    }

    FastGaussianNoise(T rms, std::uint32_t seed_value, sp::SamplingFreq sampling_freq)
        : rms(rms)
        , sampling_freq_value(sampling_freq) {
        debug_assert(rms > T { 0 });
        seed(seed_value);
    }

    T next() {
        return rms * std_normal_approx();
    }

    sp::pipe::PollResult poll() {
        return sp::pipe::PollResult::ready;
    }

    sp::SamplingFreq sampling_freq() const {
        return sampling_freq_value;
    }

private:
    void seed(std::uint32_t seed_value) {
        state = seed_value ? seed_value : 0x6d2b79f5u;
    }

    std::uint32_t next_u32() {
        // xorshift32
        std::uint32_t x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        state = x;
        return x;
    }

    T uniform01() {
        // Convert to [0,1). 2^-32 = 2.3283064365386963e-10
        return static_cast<T>(next_u32()) * static_cast<T>(2.3283064365386963e-10);
    }

    T std_normal_approx() {
        // Sum of 12 uniforms minus 6 gives ~N(0,1)
        T sum = T { 0 };
        for (int i = 0; i < 12; ++i) {
            sum += uniform01();
        }
        return sum - T { 6 };
    }

    std::uint32_t state = 0;
    T rms;
    sp::SamplingFreq sampling_freq_value;
};

// An infinite source that generates a linear ramping signal with constant slope.
// Generates: position(t) = initial_value + acceleration * t
// Precomputes the step per sample to avoid costly per-sample computation.
template <typename T>
class Ramp {
public:
    using sample_type = T;

    Ramp(T initial_value, T acceleration, sp::SamplingFreq sampling_freq)
        : current_position(initial_value)
        , sampling_freq_value(sampling_freq) {
        debug_assert(sampling_freq > sp::SamplingFreq { 0 });

        // Precompute step per sample to avoid per-sample computation
        // position[n] = initial_value + acceleration * n * dt
        // Incremental: position[n+1] = position[n] + acceleration * dt
        const T dt = T { 1 } / static_cast<T>(sampling_freq);
        step = acceleration * dt;
    }

    T next() {
        T result = current_position;
        current_position += step;
        return result;
    }

    sp::pipe::PollResult poll() {
        return sp::pipe::PollResult::ready;
    }

    sp::SamplingFreq sampling_freq() const {
        return sampling_freq_value;
    }

private:
    T current_position;
    T step; // Constant increment per sample (acceleration * dt)
    sp::SamplingFreq sampling_freq_value;
};

} // namespace sp

namespace sp::pipe {

// Factory function for creating a linear chirp source
template <typename T>
sp::Chirp<T, sp::LinearPhase> make_linear_chirp(T start_freq, T end_freq, T amplitude,
    sp::Duration duration, sp::SamplingFreq sampling_freq) {
    return sp::Chirp<T, sp::LinearPhase>(start_freq, end_freq, amplitude, duration, sampling_freq);
}

// Factory function for creating a quadratic chirp source
template <typename T>
sp::Chirp<T, sp::QuadraticPhase> make_quadratic_chirp(T start_freq, T end_freq, T amplitude,
    sp::Duration duration, sp::SamplingFreq sampling_freq) {
    return sp::Chirp<T, sp::QuadraticPhase>(start_freq, end_freq, amplitude, duration, sampling_freq);
}

// Factory function for creating Gaussian white noise with random seed
template <typename T>
sp::GaussianNoise<T> make_gaussian_noise(T rms, sp::SamplingFreq sampling_freq) {
    return sp::GaussianNoise<T>(rms, sampling_freq);
}

// Factory function for creating Gaussian white noise with deterministic seed
template <typename T>
sp::GaussianNoise<T> make_gaussian_noise(T rms, std::uint32_t seed,
    sp::SamplingFreq sampling_freq) {
    return sp::GaussianNoise<T>(rms, seed, sampling_freq);
}

// Factory function for creating fast approximate Gaussian white noise
template <typename T>
sp::FastGaussianNoise<T> make_fast_gaussian_noise(T rms, sp::SamplingFreq sampling_freq) {
    return sp::FastGaussianNoise<T>(rms, sampling_freq);
}

// Factory function for creating fast approximate Gaussian white noise with deterministic seed
template <typename T>
sp::FastGaussianNoise<T> make_fast_gaussian_noise(T rms, std::uint32_t seed,
    sp::SamplingFreq sampling_freq) {
    return sp::FastGaussianNoise<T>(rms, seed, sampling_freq);
}

// Factory function for creating a ramp signal with constant acceleration
template <typename T>
sp::Ramp<T> make_ramp(T initial_value, T acceleration, sp::SamplingFreq sampling_freq) {
    return sp::Ramp<T>(initial_value, acceleration, sampling_freq);
}

} // namespace sp::pipe

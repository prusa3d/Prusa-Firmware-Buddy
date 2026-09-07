#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <concepts>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <numeric>
#include <signal_processing/math.hpp>
#include <signal_processing/pipeline.hpp>
#include <bsod/bsod.h>

// This file defines various standard filters. Each filter is implemented as a
// class with a process() method that takes a single sample and returns the
// processed sample. There is also a factory function for each filter that
// creates a pipeline node for that filter that allows for easy composition in a
// pipeline.

namespace sp {

template <class F>
concept Filter = requires(F f, typename F::sample_type s) {
    typename F::sample_type;
    { f.process(s) } -> std::same_as<typename F::sample_type>;
    { f.reset() };
};

template <typename T, std::size_t N>
class FIR {
public:
    static_assert(N > 0, "FIR filter length must be greater than 0");

    using sample_type = T;
    static constexpr std::size_t length = N;

    explicit FIR(const std::array<T, N> &coefficients)
        : coeffs(coefficients) {}

    // Discrete convolution split into two loops to avoid modulo in the
    // inner loop. The first loop covers the non-wrapping part of the delay
    // line, the second loop covers the wrapped part.
    T process(T sample) {
        delay_line[index] = sample;
        const std::size_t write_pos = index;
        index = (index + 1) % N;

        // Correlate coefficients with delay line walking backwards from
        // write_pos (the newest sample, multiplied by coeffs[0]), wrapping
        // around. For symmetric filters this is equivalent to convolution.
        T result = T { 0 };
        std::size_t coeff_idx = 0;
        std::size_t delay_idx = write_pos + 1;

        // Indices [write_pos ... 0]
        const std::size_t first_segment = write_pos + 1;
        for (std::size_t i = 0; i < first_segment; ++i) {
            --delay_idx;
            result += coeffs[coeff_idx++] * delay_line[delay_idx];
        }

        // Wrapped indices [N-1 ... write_pos+1]
        delay_idx = N;
        const std::size_t second_segment = N - first_segment;
        for (std::size_t i = 0; i < second_segment; ++i) {
            --delay_idx;
            result += coeffs[coeff_idx++] * delay_line[delay_idx];
        }
        return result;
    }

    void reset() {
        delay_line.fill(T { 0 });
        index = 0;
    }

private:
    std::array<T, N> coeffs;
    std::array<T, N> delay_line {};
    std::size_t index = 0;
};

// Coefficients of H(z) = (b0 + b1 z^-1 + b2 z^-2) / (1 + a1 z^-1 + a2 z^-2)
template <typename T>
struct BiquadCoeffs {
    T b0 = T { 1 };
    T b1 = T { 0 };
    T b2 = T { 0 };
    T a1 = T { 0 };
    T a2 = T { 0 };

    static constexpr BiquadCoeffs first_order(T b0, T b1, T a1) {
        return BiquadCoeffs { b0, b1, T { 0 }, a1, T { 0 } };
    }
};

template <typename T>
class Biquad {
public:
    using sample_type = T;
    using coeffs_type = BiquadCoeffs<T>;

    Biquad() = default;

    explicit Biquad(const BiquadCoeffs<T> &coeffs)
        : coeffs(coeffs) {}

    Biquad(T b0, T b1, T b2, T a1, T a2)
        : coeffs { b0, b1, b2, a1, a2 } {}

    T process(T sample) {
        // Transposed Direct Form II — better numerical accuracy than direct forms
        // with float precision, and only 2 state variables.
        // https://en.wikipedia.org/wiki/Digital_biquad_filter#Transposed_direct_forms_2
        T result = coeffs.b0 * sample + state1;
        state1 = coeffs.b1 * sample - coeffs.a1 * result + state2;
        state2 = coeffs.b2 * sample - coeffs.a2 * result;
        return result;
    }

    void reset() {
        state1 = T { 0 };
        state2 = T { 0 };
    }

    // Updates coefficients without resetting internal state.
    void set_coeffs(const BiquadCoeffs<T> &new_coeffs) {
        coeffs = new_coeffs;
    }

private:
    BiquadCoeffs<T> coeffs {};
    T state1 = T { 0 };
    T state2 = T { 0 };
};

template <typename T, std::size_t N>
class BiquadCascade {
public:
    static_assert(N > 0, "Biquad cascade must have at least one stage");
    using sample_type = T;
    using coeffs_array_type = std::array<BiquadCoeffs<T>, N>;
    static constexpr std::size_t num_stages = N;

    BiquadCascade() = default;

    explicit BiquadCascade(const std::array<BiquadCoeffs<T>, N> &coeffs) {
        for (std::size_t i = 0; i < N; ++i) {
            stages[i].set_coeffs(coeffs[i]);
        }
    }

    T process(T input) {
        T signal = input;
        for (auto &stage : stages) {
            signal = stage.process(signal);
        }
        return signal;
    }

    void reset() {
        for (auto &stage : stages) {
            stage.reset();
        }
    }

    Biquad<T> &stage(std::size_t index) {
        return stages[index];
    }

    const Biquad<T> &stage(std::size_t index) const {
        return stages[index];
    }

    // Updates coefficients without resetting internal state.
    void set_coeffs(const std::array<BiquadCoeffs<T>, N> &coeffs) {
        for (std::size_t i = 0; i < N; ++i) {
            stages[i].set_coeffs(coeffs[i]);
        }
    }

private:
    std::array<Biquad<T>, N> stages {};
};

// Median filter with compile-time window size N.
// Uses std::nth_element (O(N) average) for the general case and a
// hardcoded sorting network for N==5.
template <typename T, std::size_t N>
class MedianFilter {
public:
    static_assert(N > 0 && N % 2 == 1, "Median filter window must be odd and > 0");
    using sample_type = T;

    MedianFilter() = default;

    T filter(T input) {
        buffer_[write_index_] = input;
        write_index_ = (write_index_ + 1) % N;

        if (count_ < N) {
            ++count_;
            return input;
        }

        std::array<T, N> tmp = buffer_;

        if constexpr (N == 1) {
            return tmp[0];
        } else if constexpr (N == 3) {
            // Sorting network for 3 elements (3 comparisons)
            auto s = [](T &a, T &b) { if (a > b) { T t = a; a = b; b = t; } };
            s(tmp[0], tmp[1]);
            s(tmp[1], tmp[2]);
            s(tmp[0], tmp[1]);
            return tmp[1];
        } else if constexpr (N == 5) {
            // Sorting network for 5 elements (9 comparisons, optimal)
            auto s = [](T &a, T &b) { if (a > b) { T t = a; a = b; b = t; } };
            s(tmp[0], tmp[1]);
            s(tmp[3], tmp[4]);
            s(tmp[2], tmp[4]);
            s(tmp[2], tmp[3]);
            s(tmp[0], tmp[3]);
            s(tmp[0], tmp[2]);
            s(tmp[1], tmp[4]);
            s(tmp[1], tmp[3]);
            s(tmp[1], tmp[2]);
            return tmp[2];
        } else {
            constexpr std::size_t mid = N / 2;
            std::nth_element(tmp.begin(), tmp.begin() + mid, tmp.end());
            return tmp[mid];
        }
    }

    void reset() {
        buffer_.fill(T { 0 });
        write_index_ = 0;
        count_ = 0;
    }

private:
    std::array<T, N> buffer_ {};
    std::size_t write_index_ = 0;
    std::size_t count_ = 0;
};

template <typename T, std::size_t N>
constexpr std::array<T, N> moving_average_coeffs() {
    static_assert(N > 0, "Moving average window size must be non-zero");
    std::array<T, N> coeffs {};
    T value = T { 1 } / static_cast<T>(N);
    for (std::size_t i = 0; i < N; ++i) {
        coeffs[i] = value;
    }
    return coeffs;
}

template <typename T, std::size_t N>
std::array<T, N> hamming_lowpass_coeffs(T cutoff_hz, T sample_rate) {
    static_assert(N > 0, "Filter length must be greater than 0");
    static_assert(N % 2 == 1, "N should be odd for symmetric FIR filter");
    debug_assert(sample_rate > T { 0 });
    debug_assert(cutoff_hz > T { 0 });
    debug_assert(cutoff_hz < sample_rate / T { 2 });

    std::array<T, N> coeffs {};

    // Normalized cutoff frequency (radians)
    const T omega_c = T { 2 } * std::numbers::pi_v<T> * cutoff_hz / sample_rate;
    const int M = static_cast<int>(N - 1) / 2;

    // Generate windowed sinc coefficients
    for (int n = -M; n <= M; ++n) {
        const std::size_t idx = static_cast<std::size_t>(n + M);

        // Ideal lowpass (sinc function)
        T h;
        if (n == 0) {
            h = omega_c / std::numbers::pi_v<T>;
        } else {
            h = sp::sin(omega_c * static_cast<T>(n)) / (std::numbers::pi_v<T> * static_cast<T>(n));
        }

        // Hamming window: w[n] = 0.54 - 0.46*cos(2πn/(N-1))
        const T window_arg = T { 2 } * std::numbers::pi_v<T> * static_cast<T>(n + M) / static_cast<T>(N - 1);
        const T w = static_cast<T>(0.54) - static_cast<T>(0.46) * sp::cos(window_arg);

        coeffs[idx] = h * w;
    }

    // Normalize to unity gain at DC (sum of coefficients = 1)
    const T sum = std::accumulate(coeffs.begin(), coeffs.end(), T { 0 });
    for (auto &c : coeffs) {
        c /= sum;
    }

    return coeffs;
}

template <typename T, std::size_t N>
std::array<T, N> gaussian_lowpass_coeffs(T sigma) {
    static_assert(N > 0, "Filter length must be greater than 0");
    static_assert(N % 2 == 1, "N should be odd for symmetric FIR filter");
    debug_assert(sigma > T { 0 });

    std::array<T, N> coeffs {};
    const int M = static_cast<int>(N - 1) / 2;
    const T two_sigma_sq = T { 2 } * sigma * sigma;

    // Generate Gaussian coefficients
    for (int n = -M; n <= M; ++n) {
        const std::size_t idx = static_cast<std::size_t>(n + M);
        const T n_f = static_cast<T>(n);
        coeffs[idx] = std::exp(-(n_f * n_f) / two_sigma_sq);
    }

    // Normalize to unity gain at DC
    const T sum = std::accumulate(coeffs.begin(), coeffs.end(), T { 0 });
    for (auto &c : coeffs) {
        c /= sum;
    }

    return coeffs;
}

template <typename T, std::size_t N>
std::array<T, N> gaussian_lowpass_coeffs_fc(T cutoff_hz, T sample_rate) {
    debug_assert(sample_rate > T { 0 });
    debug_assert(cutoff_hz > T { 0 });
    debug_assert(cutoff_hz < sample_rate / T { 2 });
    // Approximate relationship between sigma and -3dB cutoff frequency
    // For Gaussian: fc_3dB ≈ 0.133 * fs / sigma
    // Therefore: sigma ≈ 0.133 * fs / fc
    const T sigma = static_cast<T>(0.133) * sample_rate / cutoff_hz;
    return gaussian_lowpass_coeffs<T, N>(sigma);
}

// Bilinear transform prewarping: returns normalized prewarped frequency c.
template <typename T>
T bilinear_prewarp(T cutoff_hz, T sample_rate) {
    debug_assert(sample_rate > T { 0 });
    debug_assert(cutoff_hz > T { 0 });
    debug_assert(cutoff_hz < sample_rate / T { 2 });
    const T omega_c = T { 2 } * std::numbers::pi_v<T> * cutoff_hz;
    const T omega_d = T { 2 } * sample_rate * sp::tan(omega_c / (T { 2 } * sample_rate));
    return omega_d / (T { 2 } * sample_rate);
}

template <typename T>
BiquadCoeffs<T> butterworth_lowpass_biquad_1st(T cutoff_hz, T sample_rate) {
    const T c = bilinear_prewarp(cutoff_hz, sample_rate);

    const T a0 = T { 1 } + c;
    const T b0 = c / a0;
    const T b1 = c / a0;
    const T a1 = (c - T { 1 }) / a0;

    return BiquadCoeffs<T>::first_order(b0, b1, a1);
}

template <typename T>
BiquadCoeffs<T> butterworth_lowpass_biquad_2nd(T cutoff_hz, T sample_rate) {
    const T c = bilinear_prewarp(cutoff_hz, sample_rate);
    const T c2 = c * c;

    // Butterworth 2nd order has Q = 1/sqrt(2) = 0.7071...
    constexpr T sqrt2 = std::numbers::sqrt2_v<T>;

    const T a0 = T { 1 } + sqrt2 * c + c2;
    const T b0 = c2 / a0;
    const T b1 = T { 2 } * c2 / a0;
    const T b2 = c2 / a0;
    const T a1 = T { 2 } * (c2 - T { 1 }) / a0;
    const T a2 = (T { 1 } - sqrt2 * c + c2) / a0;

    return BiquadCoeffs<T> { b0, b1, b2, a1, a2 };
}

template <typename T>
std::array<BiquadCoeffs<T>, 2> butterworth_lowpass_biquads_4th(T cutoff_hz, T sample_rate) {
    const T c = bilinear_prewarp(cutoff_hz, sample_rate);
    const T c2 = c * c;

    // 4th-order Butterworth poles (2 pairs of complex conjugates)
    constexpr T q1 = T { 1 } / (T { 2 } * std::sin(T { 3 } * std::numbers::pi_v<T> / T { 8 }));
    constexpr T q2 = T { 1 } / (T { 2 } * std::sin(std::numbers::pi_v<T> / T { 8 }));

    const auto create_butterworth_biquads = [&](T pole) -> BiquadCoeffs<T> {
        const T k = c / pole;
        const T a0 = T { 1 } + k + c2;
        return BiquadCoeffs<T> {
            .b0 = c2 / a0,
            .b1 = T { 2 } * c2 / a0,
            .b2 = c2 / a0,
            .a1 = T { 2 } * (c2 - T { 1 }) / a0,
            .a2 = (T { 1 } - k + c2) / a0
        };
    };

    return { create_butterworth_biquads(q1), create_butterworth_biquads(q2) };
}

template <typename T>
class LeakyMean {
public:
    using sample_type = T;

    explicit LeakyMean(T leak)
        : leak(leak) {
        debug_assert(leak > T { 0 });
        debug_assert(leak <= T { 1 });
    }

    T process(T sample) {
        if (!initialized) {
            mean = sample;
            initialized = true;
        } else {
            mean += leak * (sample - mean);
        }
        return sample - mean;
    }

    // After reset, the next sample re-seeds the mean estimate.
    void reset() {
        initialized = false;
        mean = T { 0 };
    }

    void set_mean(T new_mean) {
        mean = new_mean;
        initialized = true;
    }

private:
    T leak;
    T mean = T { 0 };
    bool initialized = false;
};

template <typename T>
class CubicSoftClip {
public:
    using sample_type = T;

    CubicSoftClip(T min_value, T max_value)
        : min_value(min_value)
        , max_value(max_value) {
        debug_assert(min_value < max_value);
        recompute_scale();
    }

    T process(T sample) {
        const T normalized = std::clamp((sample - center) / half_range, T { -1 }, T { 1 });
        const T soft = (static_cast<T>(3) * normalized - normalized * normalized * normalized) / T { 2 };
        return std::clamp(center + soft * half_range, min_value, max_value);
    }

    void reset() {}

private:
    void recompute_scale() {
        half_range = (max_value - min_value) / T { 2 };
        center = min_value + half_range;
    }

    T min_value;
    T max_value;
    T center = T { 0 };
    T half_range = T { 0 };
};

template <typename T>
class AGC {
public:
    using sample_type = T;

    AGC(T target_rms, T attack_coeff, T release_coeff)
        : target_rms(target_rms)
        , attack_coeff(attack_coeff)
        , release_coeff(release_coeff)
        , rms_squared(target_rms * target_rms) {
        debug_assert(target_rms > T { 0 });
        debug_assert(attack_coeff > T { 0 });
        debug_assert(attack_coeff <= T { 1 });
        debug_assert(release_coeff > T { 0 });
        debug_assert(release_coeff <= T { 1 });
    }

    T process(T sample) {
        // Update RMS estimate using exponential moving average
        const T sample_squared = sample * sample;

        // Choose attack or release coefficient based on signal level change
        const T coeff = (sample_squared > rms_squared) ? attack_coeff : release_coeff;
        rms_squared += coeff * (sample_squared - rms_squared);

        // Calculate and apply gain
        const T current_rms = sp::sqrt(rms_squared);
        const T gain = (current_rms > T { 0 }) ? (target_rms / current_rms) : T { 1 };

        return sample * gain;
    }

    void reset() {
        rms_squared = target_rms * target_rms;
    }

    T get_current_rms() const {
        return sp::sqrt(rms_squared);
    }

    T get_current_gain() const {
        const T current_rms = sp::sqrt(rms_squared);
        return (current_rms > T { 0 }) ? (target_rms / current_rms) : T { 1 };
    }

private:
    T target_rms;
    T attack_coeff;
    T release_coeff;
    T rms_squared;
};

} // namespace sp

namespace sp::pipe {

template <Source S, sp::Filter F>
    requires std::same_as<typename S::sample_type, typename F::sample_type>
class FilterNode : public NodeBase<S> {
public:
    using sample_type = typename F::sample_type;

    template <typename... Args>
    FilterNode(S source, Args &&...args)
        : NodeBase<S>(std::move(source))
        , filter(std::forward<Args>(args)...) {}

    sample_type next() {
        return filter.process(this->source.next());
    }

    F &get_filter() { return filter; }
    const F &get_filter() const { return filter; }

private:
    F filter;
};

template <typename T, std::size_t N>
auto fir(const std::array<T, N> &coefficients) {
    return [coefficients]<Source S>(S &&s) {
        return FilterNode<std::remove_cvref_t<S>, sp::FIR<T, N>> {
            std::forward<S>(s), coefficients
        };
    };
}

template <typename T>
auto biquad(const sp::BiquadCoeffs<T> &coeffs) {
    return [coeffs]<Source S>(S &&s) {
        return FilterNode<std::remove_cvref_t<S>, sp::Biquad<T>> {
            std::forward<S>(s), coeffs
        };
    };
}

template <typename T>
auto biquad(T b0, T b1, T b2, T a1, T a2) {
    return [=]<Source S>(S &&s) {
        return FilterNode<std::remove_cvref_t<S>, sp::Biquad<T>> {
            std::forward<S>(s), sp::BiquadCoeffs<T> { b0, b1, b2, a1, a2 }
        };
    };
}

template <typename T, std::size_t N>
auto biquad_cascade(const std::array<sp::BiquadCoeffs<T>, N> &coeffs) {
    return [coeffs]<Source S>(S &&s) {
        return FilterNode<std::remove_cvref_t<S>, sp::BiquadCascade<T, N>> {
            std::forward<S>(s), coeffs
        };
    };
}

template <typename T>
auto leaky_mean(T leak) {
    return [leak]<Source S>(S &&s) {
        return FilterNode<std::remove_cvref_t<S>, sp::LeakyMean<T>> {
            std::forward<S>(s), leak
        };
    };
}

template <typename T>
auto cubic_soft_clip(T min_value, T max_value) {
    return [min_value, max_value]<Source S>(S &&s) {
        return FilterNode<std::remove_cvref_t<S>, sp::CubicSoftClip<T>> {
            std::forward<S>(s), min_value, max_value
        };
    };
}

template <typename T>
auto agc(T target_rms, T attack_coeff, T release_coeff) {
    return [target_rms, attack_coeff, release_coeff]<Source S>(S &&s) {
        return FilterNode<std::remove_cvref_t<S>, sp::AGC<T>> {
            std::forward<S>(s), target_rms, attack_coeff, release_coeff
        };
    };
}

template <typename T>
auto agc(T target_rms, sp::Duration attack_time, sp::Duration release_time) {
    const T attack_seconds = std::chrono::duration<float>(attack_time).count();
    const T release_seconds = std::chrono::duration<float>(release_time).count();
    debug_assert(attack_seconds > T { 0 });
    debug_assert(release_seconds > T { 0 });

    return [=]<Source S>(S &&s) {
        const sp::SamplingFreq sampling_freq = s.sampling_freq();
        debug_assert(sampling_freq > sp::SamplingFreq { 0 });
        debug_assert(attack_seconds > T { 0 });
        debug_assert(release_seconds > T { 0 });

        // Convert time constant in seconds to samples, then to coefficient
        const T attack_time_samples = attack_seconds * static_cast<T>(sampling_freq);
        const T release_time_samples = release_seconds * static_cast<T>(sampling_freq);

        // Convert time constant in samples to EMA coefficient
        // tau (time constant) ≈ 1 / coeff
        // Therefore: coeff ≈ 1 / tau
        const T attack_coeff = T { 1 } / attack_time_samples;
        const T release_coeff = T { 1 } / release_time_samples;

        return FilterNode<std::remove_cvref_t<S>, sp::AGC<T>> {
            std::forward<S>(s), target_rms, attack_coeff, release_coeff
        };
    };
}

template <typename T, std::size_t N>
auto moving_average() {
    static constexpr auto coeffs = sp::moving_average_coeffs<T, N>();
    return fir(coeffs);
}

template <typename T, std::size_t N>
auto hamming_lowpass(T cutoff_hz) {
    return [cutoff_hz]<Source S>(S &&s) {
        const sp::SamplingFreq sampling_freq = s.sampling_freq();
        debug_assert(sampling_freq > sp::SamplingFreq { 0 });
        return FilterNode<std::remove_cvref_t<S>, sp::FIR<T, N>> {
            std::forward<S>(s),
            sp::hamming_lowpass_coeffs<T, N>(cutoff_hz, static_cast<T>(sampling_freq))
        };
    };
}

template <typename T, std::size_t N>
auto gaussian_lowpass(T sigma) {
    return fir(sp::gaussian_lowpass_coeffs<T, N>(sigma));
}

template <typename T, std::size_t N>
auto gaussian_lowpass_fc(T cutoff_hz) {
    return [cutoff_hz]<Source S>(S &&s) {
        const sp::SamplingFreq sampling_freq = s.sampling_freq();
        debug_assert(sampling_freq > sp::SamplingFreq { 0 });
        return FilterNode<std::remove_cvref_t<S>, sp::FIR<T, N>> {
            std::forward<S>(s),
            sp::gaussian_lowpass_coeffs_fc<T, N>(cutoff_hz, static_cast<T>(sampling_freq))
        };
    };
}

template <typename T>
auto butterworth_lowpass_1st(T cutoff_hz) {
    return [cutoff_hz]<Source S>(S &&s) {
        const sp::SamplingFreq sampling_freq = s.sampling_freq();
        debug_assert(sampling_freq > sp::SamplingFreq { 0 });
        return FilterNode<std::remove_cvref_t<S>, sp::Biquad<T>> {
            std::forward<S>(s),
            sp::butterworth_lowpass_biquad_1st<T>(cutoff_hz, static_cast<T>(sampling_freq))
        };
    };
}

template <typename T>
auto butterworth_lowpass_2nd(T cutoff_hz) {
    return [cutoff_hz]<Source S>(S &&s) {
        const sp::SamplingFreq sampling_freq = s.sampling_freq();
        debug_assert(sampling_freq > sp::SamplingFreq { 0 });
        return FilterNode<std::remove_cvref_t<S>, sp::Biquad<T>> {
            std::forward<S>(s),
            sp::butterworth_lowpass_biquad_2nd<T>(cutoff_hz, static_cast<T>(sampling_freq))
        };
    };
}

template <typename T>
auto butterworth_lowpass_4th(T cutoff_hz) {
    return [cutoff_hz]<Source S>(S &&s) {
        const sp::SamplingFreq sampling_freq = s.sampling_freq();
        debug_assert(sampling_freq > sp::SamplingFreq { 0 });
        return FilterNode<std::remove_cvref_t<S>, sp::BiquadCascade<T, 2>> {
            std::forward<S>(s),
            sp::butterworth_lowpass_biquads_4th<T>(cutoff_hz, static_cast<T>(sampling_freq))
        };
    };
}

} // namespace sp::pipe

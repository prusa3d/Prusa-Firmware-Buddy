#pragma once

#include <numbers>
#include <vector>
#include <span>
#include <cmath>
#include <type_traits>
#include <arm_math.h>
#include <signal_processing/math.hpp>
#include <bsod/bsod.h>

namespace sp {

// Concept for an object capable of applying a window to a data span. It is an
// object to allow for precomputation of window coefficients.
template <typename W, typename T>
concept Windowing = std::constructible_from<W, size_t> && requires(const W &w, std::span<T> s) {
    { w.apply(s) } -> std::same_as<void>;
};

template <typename W>
concept WindowPolicy = Windowing<W, float>;

template <typename T>
class NoWindow {
public:
    explicit NoWindow(size_t /* window_size */) {}
    void apply(std::span<T> /* data */) const {
        // No-op: rectangular window
    }
};

template <typename T>
class OnTheFlyHannWindow {
public:
    explicit OnTheFlyHannWindow(size_t window_size)
        : window_size(window_size) {
        debug_assert(window_size > 1 && "Window size must be greater than 1");
    }

    void apply(std::span<T> data) const {
        debug_assert(data.size() == window_size && "Data size must match window size");
        const T scale = (T(2) * std::numbers::pi_v<T>) / T(window_size - 1);
        for (size_t n = 0; n < window_size; ++n) {
            T w = T(0.5) * (T(1.0) - sp::cos(scale * T(n)));
            data[n] *= w;
        }
    }

private:
    size_t window_size;
};

template <typename T>
class HannWindow {
public:
    static_assert(std::is_same_v<T, float>, "HannWindow currently supports float (float32) only via CMSIS-DSP");

    explicit HannWindow(size_t window_size)
        : coefficients(window_size) {
        arm_hanning_f32(coefficients.data(), static_cast<uint32_t>(window_size));
    }

    void apply(std::span<T> data) const {
        debug_assert(data.size() == coefficients.size() && "Data size must match window size");
        arm_mult_f32(data.data(), coefficients.data(), data.data(), static_cast<uint32_t>(data.size()));
    }

private:
    std::vector<float> coefficients;
};

} // namespace sp

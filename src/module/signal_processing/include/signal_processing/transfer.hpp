#pragma once

#include <array>
#include <vector>
#include <span>
#include <cstring>
#include <algorithm>
#include <utility>
#include <numbers>

#include "math.hpp"
#include "windowing.hpp"
#include "fft.hpp"
#include <bsod/bsod.h>

namespace sp {

// Common storage type for frequency-domain data. Both Complex and MagPhase
// are layout-compatible with this type (verified by static_assert in math.hpp).
using FreqBin = std::array<float, 2>;

static_assert(sizeof(FreqBin) == sizeof(Complex));
static_assert(sizeof(FreqBin) == sizeof(MagPhase));
static_assert(alignof(FreqBin) == alignof(Complex));
static_assert(alignof(FreqBin) == alignof(MagPhase));

class UnwrappedTransferFunction;

// Represents a sampled/discretized transfer function H(f) in the frequency
// domain. Stores complex transfer function values and provides frequency
// access.
class TransferFunction {
public:
    TransferFunction(size_t num_bins, float sampling_freq)
        : bins(num_bins)
        , sampling_freq(sampling_freq) {}

    TransferFunction(std::vector<FreqBin> &&bins, float sampling_freq)
        : bins(std::move(bins))
        , sampling_freq(sampling_freq) {}

    Complex &operator[](size_t bin) {
        debug_assert(bin < bins.size());
        return reinterpret_cast<Complex &>(bins[bin]);
    }

    const Complex &operator[](size_t bin) const {
        debug_assert(bin < bins.size());
        return reinterpret_cast<const Complex &>(bins[bin]);
    }

    float get_frequency(size_t bin) const {
        debug_assert(bin < bins.size());
        return (sampling_freq * static_cast<float>(bin)) / static_cast<float>((bins.size() - 1) * 2);
    }

    size_t size() const { return bins.size(); }
    float sampling_frequency() const { return sampling_freq; }

    std::span<Complex> data() { return { reinterpret_cast<Complex *>(bins.data()), bins.size() }; }
    std::span<const Complex> data() const { return { reinterpret_cast<const Complex *>(bins.data()), bins.size() }; }

    static TransferFunction from(const UnwrappedTransferFunction &utf);
    static TransferFunction from(UnwrappedTransferFunction &&utf);

private:
    friend class UnwrappedTransferFunction;
    // We use std::vector instead of sfl::segmented_vector for compatibility
    // with the estimators.
    std::vector<FreqBin> bins;
    float sampling_freq;
};

// Represents a transfer function as magnitude and unwrapped phase. Such
// representation is often more convenient for fitting than the complex form.
// Shares the same underlying storage as TransferFunction (vector<FreqBin>),
// enabling zero-allocation conversion via UnwrappedTransferFunction::from().
class UnwrappedTransferFunction {
public:
    UnwrappedTransferFunction(size_t num_bins, float sampling_freq)
        : bins(num_bins)
        , sampling_freq(sampling_freq) {}

    UnwrappedTransferFunction(std::vector<FreqBin> &&bins, float sampling_freq)
        : bins(std::move(bins))
        , sampling_freq(sampling_freq) {}

    MagPhase &operator[](size_t bin) {
        debug_assert(bin < bins.size());
        return reinterpret_cast<MagPhase &>(bins[bin]);
    }

    const MagPhase &operator[](size_t bin) const {
        debug_assert(bin < bins.size());
        return reinterpret_cast<const MagPhase &>(bins[bin]);
    }

    float get_frequency(size_t bin) const {
        debug_assert(bin < bins.size());
        return (sampling_freq * static_cast<float>(bin)) / static_cast<float>((bins.size() - 1) * 2);
    }

    size_t size() const { return bins.size(); }
    float sampling_frequency() const { return sampling_freq; }

    float magnitude(size_t bin) const { return (*this)[bin].magnitude; }
    float phase(size_t bin) const { return (*this)[bin].phase; }

    std::span<MagPhase> data() { return { reinterpret_cast<MagPhase *>(bins.data()), bins.size() }; }
    std::span<const MagPhase> data() const { return { reinterpret_cast<const MagPhase *>(bins.data()), bins.size() }; }

    // Construct from a TransferFunction (complex → magnitude + unwrapped phase).
    // The const-ref overload allocates a new buffer; the rvalue overload
    // converts in-place with zero allocation.
    static UnwrappedTransferFunction from(const TransferFunction &tf);
    static UnwrappedTransferFunction from(TransferFunction &&tf);

private:
    friend class TransferFunction;

    static void unwrap_phase(std::vector<FreqBin> &bins) {
        if (bins.size() < 2) {
            return;
        }
        constexpr float pi = std::numbers::pi_v<float>;
        constexpr float two_pi = 2.0f * pi;
        for (size_t i = 1; i < bins.size(); ++i) {
            float diff = bins[i][1] - bins[i - 1][1];
            while (diff > pi) {
                bins[i][1] -= two_pi;
                diff -= two_pi;
            }
            while (diff < -pi) {
                bins[i][1] += two_pi;
                diff += two_pi;
            }
        }
    }

    static void complex_to_mag_phase(std::vector<FreqBin> &bins) {
        for (auto &b : bins) {
            const auto &c = reinterpret_cast<const Complex &>(b);
            const float mag = c.magnitude();
            const float phase = c.angle();
            b = { mag, phase };
        }
        unwrap_phase(bins);
    }

    static void mag_phase_to_complex(std::vector<FreqBin> &bins) {
        for (auto &b : bins) {
            const auto &mp = reinterpret_cast<const MagPhase &>(b);
            const auto c = Complex::from_polar(mp);
            b = { c.re, c.im };
        }
    }

    std::vector<FreqBin> bins;
    float sampling_freq;
};

// Allocating: copies the buffer, then converts
inline UnwrappedTransferFunction UnwrappedTransferFunction::from(const TransferFunction &tf) {
    auto bins = tf.bins;
    complex_to_mag_phase(bins);
    return UnwrappedTransferFunction(std::move(bins), tf.sampling_freq);
}

// Stealing: converts in-place, zero allocation
inline UnwrappedTransferFunction UnwrappedTransferFunction::from(TransferFunction &&tf) {
    complex_to_mag_phase(tf.bins);
    return UnwrappedTransferFunction(std::move(tf.bins), tf.sampling_freq);
}

// Allocating: copies the buffer, then converts
inline TransferFunction TransferFunction::from(const UnwrappedTransferFunction &utf) {
    auto bins = utf.bins;
    UnwrappedTransferFunction::mag_phase_to_complex(bins);
    return TransferFunction(std::move(bins), utf.sampling_freq);
}

// Stealing: converts in-place, zero allocation
inline TransferFunction TransferFunction::from(UnwrappedTransferFunction &&utf) {
    UnwrappedTransferFunction::mag_phase_to_complex(utf.bins);
    return TransferFunction(std::move(utf.bins), utf.sampling_freq);
}

// H1 Transfer Function Estimator using Welch's method.
//
// Usage:
// H1TransferEstimator<> estimator(1024, 0.5f);
// for (auto [x, y] : data) estimator.push(x, y);
// TransferFunction H = estimator.finalize(1000.0f);
template <WindowPolicy Window = HannWindow<float>>
class H1TransferEstimator {
public:
    H1TransferEstimator(size_t window_size, float overlap)
        : window_size(window_size)
        , hop_size(window_size - static_cast<size_t>(overlap * window_size))
        , num_bins(window_size / 2 + 1)
        , window_policy(window_size)
        , fft(window_size)
        , write_pos(0)
        , total_samples(0)
        , samples_since_window(0) {
        debug_assert((window_size & (window_size - 1)) == 0 && "Window size must be power of 2");
        debug_assert(window_size >= 8 && "Window size must be at least 8");
        debug_assert(overlap >= 0.0f && overlap < 1.0f && "Overlap must be in [0, 1)");
        debug_assert(hop_size > 0 && "Overlap too large: hop size must be positive");

        // Preallocate all buffers
        x_buffer.resize(window_size);
        y_buffer.resize(window_size);
        fft_a.resize(window_size);
        fft_b.resize(window_size);
        fft_scratch.resize(window_size);

        Gxy.resize(num_bins, { 0.0f, 0.0f });
        Gxx.resize(num_bins, 0.0f);
    }

    // Push a new sample pair (input, output)
    // x: input sample
    // y: output sample
    // Processes a window when enough new samples for a hop have arrived.
    void push(float x, float y) {
        debug_assert(!Gxx.empty() && "Cannot push after finalize()");

        x_buffer[write_pos] = x;
        y_buffer[write_pos] = y;
        write_pos = (write_pos + 1) % window_size;

        if (total_samples < window_size) {
            ++total_samples;
        }
        ++samples_since_window;

        if (total_samples == window_size && samples_since_window >= hop_size) {
            process_window();
            samples_since_window = 0;
        }
    }

    // Finalize processing. Call when no more samples will be pushed.
    // Processes any remaining partial window if enough samples are available.
    TransferFunction finalize(float sampling_freq) {
        debug_assert(!Gxx.empty() && "Cannot finalize more than once");

        // Process a final partial window if enough new samples are available.
        if (samples_since_window >= hop_size && total_samples > 0) {
            process_window();
        }

        // H1(f) = Gxy(f) / Gxx(f)
        for (size_t k = 0; k < num_bins; ++k) {
            auto &h = reinterpret_cast<Complex &>(Gxy[k]);
            if (Gxx[k] > 1e-12f) {
                h = h / Gxx[k];
            } else {
                h = Complex { 0.0f, 0.0f };
            }
        }

        // Free all internal buffers (move-assign from empty to release memory)
        x_buffer = {};
        y_buffer = {};
        fft_a = {};
        fft_b = {};
        fft_scratch = {};
        Gxx = {};

        return TransferFunction(std::move(Gxy), sampling_freq);
    }

private:
    // Process a full window: apply window, compute FFT, accumulate spectra
    void process_window() {
        // Compute X_freq: build windowed x into fft_a, FFT into fft_b, swap
        // so that fft_a holds X_freq and fft_b is free for reuse.
        build_window(fft_a, x_buffer);
        window_policy.apply(std::span<float> { fft_a.data(), window_size });
        fft(std::span<float> { fft_a }, std::span<float> { fft_b });
        std::swap(fft_a, fft_b);

        // Compute Y_freq: build windowed y into fft_b, FFT needs a distinct
        // output buffer so we use fft_scratch. Swap so fft_b holds Y_freq.
        build_window(fft_b, y_buffer);
        window_policy.apply(std::span<float> { fft_b.data(), window_size });
        fft(std::span<float> { fft_b }, std::span<float> { fft_scratch });
        std::swap(fft_b, fft_scratch);

        // At this point: fft_a = X_freq, fft_b = Y_freq (CMSIS-DSP format)
        accumulate_spectra(fft_a, fft_b);
    }

    // Accumulate cross-spectrum Gxy and auto-spectrum Gxx from FFT results
    // in CMSIS-DSP format: [DC, Nyquist, Re1, Im1, Re2, Im2, ...]
    void accumulate_spectra(const std::vector<float> &x_freq, const std::vector<float> &y_freq) {
        // DC (index 0) and Nyquist (index 1) are real-valued
        const float x_dc = x_freq[0];
        const float y_dc = y_freq[0];
        auto &gxy_dc = reinterpret_cast<Complex &>(Gxy[0]);
        gxy_dc.re += y_dc * x_dc;
        Gxx[0] += x_dc * x_dc;

        const float x_nyq = x_freq[1];
        const float y_nyq = y_freq[1];
        auto &gxy_nyq = reinterpret_cast<Complex &>(Gxy[num_bins - 1]);
        gxy_nyq.re += y_nyq * x_nyq;
        Gxx[num_bins - 1] += x_nyq * x_nyq;

        // Complex bins (k = 1..num_bins-2)
        for (size_t k = 1; k + 1 < num_bins; ++k) {
            const size_t idx = 2 * k;
            const float xr = x_freq[idx];
            const float xi = x_freq[idx + 1];
            const float yr = y_freq[idx];
            const float yi = y_freq[idx + 1];

            auto &gxy = reinterpret_cast<Complex &>(Gxy[k]);

            // Gxy += Y * conj(X)
            gxy.re += yr * xr + yi * xi;
            gxy.im += yi * xr - yr * xi;

            // Gxx += |X|^2
            Gxx[k] += xr * xr + xi * xi;
        }
    }

    void build_window(std::vector<float> &dst, const std::vector<float> &src) const {
        if (total_samples < window_size) {
            std::fill(dst.begin(), dst.end(), 0.0f);
            const size_t offset = window_size - total_samples;
            std::memcpy(dst.data() + offset, src.data(), total_samples * sizeof(float));
            return;
        }

        const size_t tail = window_size - write_pos;
        std::memcpy(dst.data(), src.data() + write_pos, tail * sizeof(float));
        std::memcpy(dst.data() + tail, src.data(), write_pos * sizeof(float));
    }

    // Configuration
    size_t window_size;
    size_t hop_size;
    size_t num_bins;

    // Windowing
    Window window_policy;

    // FFT instance
    RfftFastF32 fft;

    // Note about the buffers. They **NEED** to be std::vectors, they can't be
    // e.g., segmented_vectors. We use CMSIS-DSP FFT functions which require
    // contiguous buffers, and segmented_vectors don't guarantee that.

    // Time-domain buffers
    std::vector<float> x_buffer;
    std::vector<float> y_buffer;

    // Frequency-domain scratch buffers (swapped during process_window so
    // that after both FFTs: fft_a = X_freq, fft_b = Y_freq)
    std::vector<float> fft_a;
    std::vector<float> fft_b;
    std::vector<float> fft_scratch;

    // Accumulated spectra
    std::vector<FreqBin> Gxy; // Cross-power spectrum
    std::vector<float> Gxx; // Input auto-power spectrum

    // State
    size_t write_pos;
    size_t total_samples;
    size_t samples_since_window;
};

} // namespace sp

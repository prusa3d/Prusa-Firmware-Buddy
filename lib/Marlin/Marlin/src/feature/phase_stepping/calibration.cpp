#include "calibration.hpp"
#include "axes.hpp"
#include "phase_stepping.hpp"
#include "calibration_config.hpp"
#include "i18n.h"

#include <bsod/bsod.h>
#include <logging/log.hpp>
#include <module/planner.h>
#include <module/motion.h>
#include <mapi/motion.hpp>
#include <sfl/segmented_vector.hpp>
#include <gcode/gcode.h>

#include <algorithm>
#include <vector>
#include <cmath>
#include <numbers>
#include <expected>
#include <inplace_vector.hpp>

using namespace phase_stepping;
using namespace phase_stepping::opts;
using namespace accelerometer;

LOG_COMPONENT_REF(PhaseStepping);

// Temporary debugging to Marlin serial for convenience
// #define SERIAL_DEBUG
#define ABORT_CHECK()                   \
    if (should_abort && should_abort()) \
        return std::unexpected(CalibrateAxisError::aborted);

static constexpr std::size_t RETRY_COUNT = 2;
static constexpr float MAX_ACC_SAMPLING_RATE = 1600;
static constexpr float SAMPLE_BUFFER_MARGIN = 1.05f;
static constexpr float VIBRATION_SETTLE_TIME = 0.2f;

static float convert(AccelerometerSample sample) {
    // Optimization:
    // We don't need to convert raw samples to physical acceleration.
    // Algorithm is actually invariant to scaling of the samples,
    // as long as the samples are floats and have enough precision.
    // This is already utilized in pseudo_project() which doesn't
    // normalize the vector's magnitude. As for the precision concerns,
    // accelerometer::raw_to_accel() adds multiplicative factor of
    // 5.985687e-04 so we need about 4 orders of magnitude leaway.
    // We are also squaring the samples and then summing a bunch of them,
    // so let's say 12 orders of magnitude, meaning we should be fine.
    return static_cast<float>(sample.value);
}

// Analogy of std::span - a non-owning view on part of a an infinite 1D
// signal. Unlike std::span, it allows accessing elements out of the bounds.
// This is useful for performing windowed sweeps. If there is no sample in the
// underlying signal when performing out-of-bounds access, zero is returned.
// Non-owning view on part of an infinite 1D signal.
// Allows accessing elements out of bounds, returning zero for out-of-bounds access.
// Works with any container that supports indexing via operator[].
template <typename Container>
class SignalViewT {
    const Container &_signal;
    std::ptrdiff_t _start, _end;

public:
    using value_type = typename Container::value_type;
    using size_type = std::ptrdiff_t;
    using difference_type = std::ptrdiff_t;
    using reference = const typename Container::value_type &;
    using pointer = const typename Container::value_type *;

    SignalViewT(const Container &signal, std::ptrdiff_t start, std::ptrdiff_t end)
        : _signal(signal)
        , _start(start)
        , _end(end) {}

    class const_iterator {
        const SignalViewT *_view;
        int _index;

    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = typename Container::value_type;
        using difference_type = std::ptrdiff_t;
        using pointer = const typename Container::value_type *;
        using reference = const typename Container::value_type &;

        const_iterator()
            : _view(nullptr)
            , _index(0) {}
        const_iterator(const SignalViewT &view, int index)
            : _view(&view)
            , _index(index) {}

        value_type operator*() const { return (*_view)[_index]; }
        value_type operator[](difference_type n) const { return (*_view)[_index + n]; }

        const_iterator &operator++() {
            ++_index;
            return *this;
        }
        const_iterator operator++(int) {
            const_iterator temp = *this;
            ++(*this);
            return temp;
        }

        const_iterator &operator--() {
            --_index;
            return *this;
        }
        const_iterator operator--(int) {
            const_iterator temp = *this;
            --(*this);
            return temp;
        }

        const_iterator &operator+=(difference_type n) {
            _index += n;
            return *this;
        }
        const_iterator &operator-=(difference_type n) {
            _index -= n;
            return *this;
        }

        const_iterator operator+(difference_type n) const { return const_iterator((*_view), _index + n); }
        const_iterator operator-(difference_type n) const { return const_iterator((*_view), _index - n); }

        difference_type operator-(const const_iterator &other) const { return _index - other._index; }

        bool operator==(const const_iterator &other) const { return _index == other._index; }
        bool operator!=(const const_iterator &other) const { return !(*this == other); }
        bool operator<(const const_iterator &other) const { return _index < other._index; }
        bool operator<=(const const_iterator &other) const { return _index <= other._index; }
        bool operator>(const const_iterator &other) const { return _index > other._index; }
        bool operator>=(const const_iterator &other) const { return _index >= other._index; }
    };

    typename Container::value_type operator[](std::ptrdiff_t i) const {
        std::ptrdiff_t idx = _start + i;
        if (idx < 0 || std::size_t(idx) >= _signal.size()) {
            return typename Container::value_type(); // Return default-constructed value for out of bounds
        }
        return _signal[idx];
    }

    std::ptrdiff_t size() const {
        return _end - _start;
    }

    const_iterator begin() const { return const_iterator(*this, 0); }
    const_iterator end() const { return const_iterator(*this, size()); }

    SignalViewT<Container> subsignal(std::ptrdiff_t start, std::ptrdiff_t end) const {
        return SignalViewT(_signal, _start + start, _start + end);
    }

    SignalViewT<Container> offset(std::ptrdiff_t offset) const {
        return SignalViewT(_signal, _start + offset, _end + offset);
    }

    SignalViewT<Container> expand(std::ptrdiff_t window) const {
        return SignalViewT(_signal, _start - window, _end + window);
    }

    std::tuple<int, int> bounds() const {
        return { _start, _end };
    }
};

using EnergyContainer = sfl::segmented_vector<float, 256>;
using MagnitudeContainer = sfl::segmented_vector<float, 256>;
using SignalContainer = sfl::segmented_vector<AccelerometerSample, 512>;
using SignalView = SignalViewT<SignalContainer>;

struct DftSweepResult {
    MagnitudeContainer samples; // Magnitudes of the sweep result
    float start_x, end_x; // Start and end of the sweep control variable

    float x_for_index(int idx) const {
        return start_x + (end_x - start_x) * idx / (samples.size() - 1);
    }
};

// Maximum number of peaks to keep per harmonic in find_peaks. Keeping fewer
// peaks reduces memory use and the O(N*M) anchor search in find_harmonic_peaks.
static constexpr size_t max_peaks_per_harmonic = 4;

struct SignalPeak {
    size_t idx;
    float prominence;
};

struct HarmonicPeak {
    int harmonic;
    float position;

    bool operator==(const HarmonicPeak &other) const {
        const auto position_equal = [](float a, float b) {
            return std::fabs(a - b) < 1000;
        };

        return harmonic == other.harmonic && position_equal(position, other.position);
    }
};

template <typename T>
struct HarmonicT {
    int harmonic = 0;
    T value = {};
};

struct MotionCharacteristics {
    float revs;
    float duration;
};

struct CalibrationResult {
    SpectralItem params;
    float score;
};

struct CalibrationSweep {
    int harmonic; // Harmonic correction on which the sweep is performed

    float setup_distance; // Distance in mm before the sweep
    float sweep_distance; // Distance in mm to perform the sweep

    int pha_start, pha_diff; // Phase start and end diff in fixed point
    int mag_start, mag_diff; // Magnitude start and end diff in fixed point

    static CalibrationSweep build_for_motor(AxisEnum axis, int harmonic,
        float setup_revs, float sweep_revs,
        float pha_start, float pha_end,
        float mag_start, float mag_end) {
        return {
            harmonic,
            rev_to_mm(axis, setup_revs),
            rev_to_mm(axis, sweep_revs),
            pha_to_fixed(pha_start),
            pha_to_fixed(pha_end - pha_start),
            mag_to_fixed(mag_start),
            mag_to_fixed(mag_end - mag_start)
        };
    }
};

enum class SweepDirection {
    Up,
    Down
};

struct MagCalibResult {
    float mag_value;
    float score;
};

using AbortFun = stdext::inplace_function<bool(void)>;

template <typename Func>
auto with_retries(std::size_t n, Func &&func) {
    debug_assert(n >= 1);

    using ExpectedType = std::invoke_result_t<Func>;
    static_assert(
        requires(ExpectedType e) { e.has_value(); },
        "Function must return std::expected");

    // We choose this construction to have a single return point to ease the
    // compiler to perform named return value optimization.
    for (std::size_t attempt = 0; attempt < n; ++attempt) {
        if (auto result = func(); result.has_value() || attempt == n - 1) {
            return result;
        }
        log_error(PhaseStepping, "Operation failed, retrying...");
    }
    __builtin_unreachable();
}

// Given change in physical space, return change in logical axis.
// - for cartesian this is identity,
// - for CORE XY it converts (A, B) to (X, Y)
static std::tuple<float, float> physical_to_logical(float x, float y) {
#ifdef COREXY
    return {
        (x + y) / 2,
        (x - y) / 2
    };
#else
    return { x, y };
#endif
}

// @brief Wait for a state of a given axis specified by a predicate.
// @return true on success, false on timeout
template <typename Pred>
static bool wait_for_movement_state(phase_stepping::AxisState &axis_state,
    int timeout_ms, Pred pred) {
    auto start_time = ticks_ms();

    while (!(pred(axis_state))) {
        auto cur_time = ticks_ms();
        if (ticks_diff(cur_time, start_time) > timeout_ms) {
            return false;
        } else {
            idle(true);
        }
    }
    return true;
}

// Given a signal, compute signal energy for each sample. The energy is deduced
// from the surrounding symmetrical window of size window_size.
static EnergyContainer signal_local_energy(SignalView signal, int window_size) {
    EnergyContainer result;
    result.reserve(signal.size());

    int half_window = window_size / 2;

    auto first_window = signal.subsignal(-half_window, half_window);
    result.push_back(std::accumulate(first_window.begin(), first_window.end(), 0.f,
        [](float acc, AccelerometerSample raw_x) { float x = convert(raw_x); return acc + x * x; }));

    for (int i = 0; i < signal.size(); i++) {
        float throw_away_elem = convert(signal[i - half_window - 1]);
        float add_elem = convert(signal[i + half_window]);
        result.push_back(result.back() - throw_away_elem * throw_away_elem + add_elem * add_elem);
    }
    return result;
}

// Given a an annotation of signal, precisely locate markers in the signal.
// Returns tuple of indices for start and end marker.
static std::tuple<int, int> locate_signal_markers(const SamplesAnnotation &annot,
    const SignalContainer &signal, float search_window = 0.1f) {
    static const float ENERGY_WIN_S = 0.005f;
    int energy_win = static_cast<int>(annot.sampling_freq * ENERGY_WIN_S);

    int markers_idx_offset = static_cast<int>((annot.end_marker - annot.start_marker) * annot.sampling_freq);
    int first_maker_start_idx = static_cast<int>((annot.start_marker - search_window / 2) * annot.sampling_freq);
    int first_maker_end_idx = static_cast<int>((annot.start_marker + search_window / 2) * annot.sampling_freq);

    auto start_marker_search_window = SignalView(signal,
        first_maker_start_idx, first_maker_end_idx);
    auto end_marker_search_window = start_marker_search_window.offset(markers_idx_offset);

    auto end_energy = signal_local_energy(end_marker_search_window, energy_win);
    auto combined_energy = signal_local_energy(start_marker_search_window, energy_win);
    for (std::size_t i = 0; i < combined_energy.size(); i++) {
        combined_energy[i] += end_energy[i];
    }

    float energy_mean = std::accumulate(combined_energy.begin(), combined_energy.end(), 0.f) / combined_energy.size();
    auto peak_it = std::find_if(combined_energy.begin(), combined_energy.end(),
        [energy_mean](float x) { return x > energy_mean; });

    int win_offset = std::distance(combined_energy.begin(), peak_it);
    return { first_maker_start_idx + win_offset, first_maker_start_idx + win_offset + markers_idx_offset };
}

// Given a raw captured samples, locate signal markers and return a view of the
// signal specified by the annotation
static SignalView locate_signal(const SamplesAnnotation &annot, const SignalContainer &signal) {
    debug_assert(annot.sampling_freq != 0);
    auto [start_marker_idx, _] = locate_signal_markers(annot, signal);

    int signal_start = static_cast<int>((annot.signal_start - annot.start_marker) * annot.sampling_freq);
    int signal_end = static_cast<int>((annot.signal_end - annot.start_marker) * annot.sampling_freq);

    return SignalView(signal, start_marker_idx + signal_start, start_marker_idx + signal_end);
}

// A rolling-over window for computing windowed DFT of a signal. Hides the
// complexity of index manipulation. You just push new correlation samples one
// after another and get the magnitude of the DFT.
class SlidingDftWindow {
private:
    std::vector<std::tuple<float, float>> buffer;
    std::vector<float> hann_window;
    int current_pos = 0;
    float sin_sum = 0;
    float cos_sum = 0;

public:
    SlidingDftWindow(int size)
        : buffer(size) {}

    void push_sample(std::tuple<float, float> sample) {
        auto [sin_sub, cos_sub] = buffer[current_pos];
        sin_sum -= sin_sub;
        cos_sum -= cos_sub;

        buffer[current_pos] = sample;

        auto [sin_add, cos_add] = sample;
        sin_sum += sin_add;
        cos_sum += cos_add;

        current_pos = (current_pos + 1) % buffer.size();
    }

    float get_magnitude() const {
        return sqrt(sin_sum * sin_sum + cos_sum * cos_sum) * 2 / buffer.size();
    }

    float get_windowed_power() {
        if (hann_window.empty()) {
            std::size_t size = buffer.size();
            hann_window.reserve(size);
            for (std::size_t i = 0; i < size; i++) {
                float x = 2 * std::numbers::pi_v<float> * i / (size - 1);
                hann_window.push_back(0.5f * (1 - std::cos(x)));
            }
        }

        float sin_sum = 0;
        float cos_sum = 0;

        for (std::size_t i = 0; i < buffer.size(); i++) {
            auto [sin_val, cos_val] = buffer[(i + current_pos) % buffer.size()];
            sin_sum += sin_val * hann_window[i];
            cos_sum += cos_val * hann_window[i];
        }
        return sin_sum * sin_sum + cos_sum * cos_sum;
    }
};

static std::tuple<int, int, int> compute_calibration_tweak(
    const CalibrationSweep &params, float relative_position) {
    relative_position = std::fabs(relative_position);

    float progress = (relative_position - params.setup_distance) / params.sweep_distance;
    progress = std::clamp(progress, 0.f, 1.f);

    return {
        params.harmonic,
        params.pha_start + progress * params.pha_diff,
        params.mag_start + progress * params.mag_diff
    };
}

namespace phase_stepping::internal {
static CalibrationSweep calibration_sweep;
std::atomic<int> calibration_axis_request = -1;
std::atomic<int> calibration_axis_active = -1;

void calibration_new_move(const AxisState &axis_state) {
    float calibration_distance = calibration_sweep.setup_distance + calibration_sweep.sweep_distance;
    auto current_target = axis_state.current_target;
    float move_distance = current_target.target_pos - current_target.initial_pos;
    calibration_axis_active = std::fabs(move_distance) >= std::fabs(calibration_distance) ? axis_state.axis_index : -1;
}

std::tuple<int, int, int> compute_calibration_tweak(float relative_position) {
    debug_assert(calibration_axis_request >= 0);
    debug_assert(calibration_axis_active >= 0);
    return compute_calibration_tweak(calibration_sweep, relative_position);
}

} // namespace phase_stepping::internal

// Compute a windowed sweep for a single bin of DFT of a constant velocity
// movement. The bin is determined by the harmonic of motor and its speed. The
// window size and step_sizes are given in motor_periods. Returns a
// DFTSweepResults, where x is time.
static DftSweepResult motor_harmonic_dft_sweep(
    SignalView signal, float sampling_freq, float motor_speed,
    int motor_steps, int harmonic, int window_size, int step_size) {
    const float motor_period_duration = 1.f / (motor_speed * motor_steps / 4);

    const float analysis_freq = motor_speed * harmonic * motor_steps / 4;
    const int window_half_size = static_cast<int>(window_size * motor_period_duration / 2 * sampling_freq);
    const float sampling_period = 1 / sampling_freq;

    auto sample_correlation = [&](int idx) {
        float arg = 2 * std::numbers::pi_v<float> * idx * analysis_freq * sampling_period;
        float s = convert(signal[idx]);

        const int int_arg = static_cast<int>(opts::SIN_PERIOD * std::fmod(arg, 2 * std::numbers::pi_v<float>) / (2 * std::numbers::pi_v<float>));
        return std::make_tuple(sin_lut(int_arg) * s, cos_lut(int_arg) * s);
    };

    int step_size_idx = std::max(1, static_cast<int>(step_size * motor_period_duration * sampling_freq));
    int total_steps = signal.size() / step_size_idx;

    DftSweepResult result {
        .samples = {},
        .start_x = 0,
        .end_x = total_steps * step_size_idx / sampling_freq
    };
    result.samples.reserve(total_steps + 1);

    SlidingDftWindow window(window_half_size * 2 + 1);
    // Fill the window for the initial sample:
    int next_sample_idx = -window_half_size;
    for (int i = 0; i != window_half_size * 2; i++, next_sample_idx++) {
        window.push_sample(sample_correlation(next_sample_idx));
    }

    for (int i = 0; i < signal.size(); i++, next_sample_idx++) {
        window.push_sample(sample_correlation(next_sample_idx));
        if (i % step_size_idx == 0) {
            result.samples.push_back(window.get_magnitude());
        }
    }
    return result;
}

// Compute a windowed sweep for a single bin of DFT of a signal where motor
// accelerates to a top speed and then decelerates. The analysis frequency is
// given by the motor speed and harmonic. The window and step sizes are given in
// seconds. Returns a 2-element array of DFTSweepResults for acceleration and
// deceleration phase. The x of the sweep is the motor speed. The both results are
// sorted form start to top speed and they have the same length.
static std::array<DftSweepResult, 2> motor_speed_dft_sweep(SignalView signal,
    float sampling_freq, float start_speed, float top_speed, int motor_steps,
    int harmonic, float window_size, float step_size) {
    debug_assert(signal.size() != 0);
    // The operation is performed by correlating the signal with an accelerated
    // sin/cos waveform followed by windowed sweep to perform convolution.
    const float sweep_duration = signal.size() / sampling_freq;
    const float ramp_duration = sweep_duration / 2;
    const int ramp_samples = signal.size() / 2;

    const float start_freq = harmonic * start_speed * motor_steps / 4;
    const float top_freq = harmonic * top_speed * motor_steps / 4;
    const float freq_accel = (top_freq - start_freq) / ramp_duration;
    const float sampling_period = 1 / sampling_freq;

    const int window_half_size = static_cast<int>(window_size * sampling_freq / 2);

    auto sample_correlation = [&](int idx) {
        const float t = idx * sampling_period;
        float freq = 0;
        float arg = 0;

        // Switch arg computation based on the phase of the movement
        if (t < 0) {
            freq = start_freq;
            arg = 2 * std::numbers::pi_v<float> * start_freq * t;
        } else if (t < ramp_duration) {
            freq = start_freq + freq_accel * t;
            arg = 2 * std::numbers::pi_v<float> * start_freq * t + std::numbers::pi_v<float> * freq_accel * t * t;
        } else {
            const float t_rel = t - ramp_duration;
            freq = top_freq - freq_accel * t_rel;
            arg = 2 * std::numbers::pi_v<float> * top_freq * t_rel - std::numbers::pi_v<float> * freq_accel * t_rel * t_rel;
        }

        if (freq < sampling_freq / 2) {
            float s = convert(signal[idx]);
            const int int_arg = static_cast<int>(opts::SIN_PERIOD * std::fmod(arg, 2 * std::numbers::pi_v<float>) / (2 * std::numbers::pi_v<float>));
            return std::make_tuple(sin_lut(int_arg) * s, cos_lut(int_arg) * s);
        } else {
            return std::make_tuple(0.f, 0.f);
        }
    };

    std::array<DftSweepResult, 2> result;
    const int step_size_idx = std::max(static_cast<int>(step_size * sampling_freq), 1);
    const int result_samples = signal.size() / 2 / step_size_idx;
    const float top_analyzed_speed = start_speed + (top_speed - start_speed) * result_samples * step_size_idx / ramp_samples;
    for (SweepDirection ramp : { SweepDirection::Up, SweepDirection::Down }) {
        auto &res = ramp == SweepDirection::Up ? result[0] : result[1];
        res.samples.reserve(result_samples + 1);
        res.start_x = start_speed;
        res.end_x = top_analyzed_speed;

        int initial_idx = ramp == SweepDirection::Up ? 0 : signal.size() - 1;
        int direction = ramp == SweepDirection::Up ? 1 : -1;

        SlidingDftWindow window(window_half_size * 2 + 1);
        // Fill the window for the initial sample:
        int next_sample_idx = initial_idx - direction * window_half_size;
        for (int i = 0; i != window_half_size * 2; i++) {
            window.push_sample(sample_correlation(next_sample_idx));
            next_sample_idx += direction;
        }

        // And sweep it across the signal:
        for (int i = 0; i != ramp_samples; i++) {
            window.push_sample(sample_correlation(next_sample_idx));
            next_sample_idx += direction;
            if (i % step_size_idx == 0) {
                float speed = start_speed + (top_speed - start_speed) * static_cast<float>(i) / ramp_samples;
                res.samples.push_back(window.get_windowed_power() / speed);
            }
        }
        debug_assert(!res.samples.empty());
    }
    return result;
}

// Given a pair of random-access iterators to a container containing float, find
// local peaks. Returns at most max_peaks_per_harmonic peaks sorted by
// prominence (descending). Uses a two-pass approach to avoid O(N) auxiliary
// arrays for left_min/right_min.
template <typename It>
stdext::inplace_vector<SignalPeak, max_peaks_per_harmonic> find_peaks(It begin, It end, float min_prominence) {
    static_assert(std::is_same_v<typename std::iterator_traits<It>::value_type, float>);
    static_assert(std::is_same_v<typename std::iterator_traits<It>::iterator_category, std::random_access_iterator_tag>);

    const size_t signal_size = std::distance(begin, end);
    auto signal_val = [begin](size_t i) { return *(begin + i); };

    if (signal_size < 3) {
        return {};
    }

    // Forward pass: find local maxima, record their left_min values, and track
    // signal_max. Theoretical max ~signal_size/2 candidates; with
    // speed_sweep_bins=400 that's at most 200 — less than the Python's 400 per
    // left_min/right_min array.
    struct PeakCandidate {
        size_t idx;
        float left_min;
    };
    sfl::segmented_vector<PeakCandidate, 64> candidates;
    float signal_max = signal_val(0);
    float running_left_min = signal_val(0);

    for (size_t i = 1; i < signal_size; i++) {
        float val = signal_val(i);
        signal_max = std::max(signal_max, val);
        running_left_min = std::min(running_left_min, val);
        if (i < signal_size - 1 && val > signal_val(i - 1) && val > signal_val(i + 1)) {
            candidates.push_back({ i, running_left_min });
        }
    }

    // Backward pass: walk the signal backward to compute running right_min.
    // When we reach a candidate's index, compute its prominence and insert
    // into the bounded result sorted by prominence (descending).
    stdext::inplace_vector<SignalPeak, max_peaks_per_harmonic> peaks;
    float running_right_min = signal_val(signal_size - 1);
    size_t cand_cursor = candidates.size();

    for (size_t i = signal_size - 2; i >= 1 && cand_cursor > 0; i--) {
        running_right_min = std::min(running_right_min, signal_val(i));
        if (candidates[cand_cursor - 1].idx != i) {
            // Not a peak position, just updating running_right_min
            continue;
        }

        cand_cursor--;
        float abs_prominence = signal_val(i) - std::min(candidates[cand_cursor].left_min, running_right_min);
        float prominence = abs_prominence / signal_max;

        if (prominence < min_prominence) {
            continue;
        }

        // Insert into bounded container, maintaining descending prominence order
        auto ins_pos = std::upper_bound(peaks.begin(), peaks.end(), prominence,
            [](float p, const SignalPeak &peak) { return p > peak.prominence; });
        if (peaks.size() < max_peaks_per_harmonic) {
            peaks.insert(ins_pos, SignalPeak { i, prominence });
        } else if (ins_pos != peaks.begin()) {
            // Better than the worst peak; drop the worst and insert
            peaks.pop_back();
            peaks.insert(ins_pos, SignalPeak { i, prominence });
        }
        // else: worse than all kept peaks, skip
    }

    if (peaks.empty()) {
        // No prominent peak found; return the highest value as fallback
        size_t max_idx = std::distance(begin, std::max_element(begin, end));
        return { SignalPeak { max_idx, 1.f } };
    }

    return peaks;
}

// Detect a peak in each signal such that they are all harmonics of a common
// fundamental frequency. Returns a single fit.
template <typename PosConverter>
stdext::inplace_vector<HarmonicPeak, opts::CORRECTION_HARMONICS> find_harmonic_peaks(
    std::span<const HarmonicT<MagnitudeContainer>> sweeps, PosConverter idx_to_pos) {
    debug_assert(!sweeps.empty());
    for (const auto &sweep : sweeps) {
        (void)sweep; // Silence unused variable warning in release builds
        debug_assert(!sweep.value.empty());
    }

    // Find peaks in each signal and convert indices to speed positions.
    // This follows the Python detect_harmonic_peaks approach: work entirely
    // in speed space to correctly handle non-zero start_speed.
    struct PosPeak {
        float position;
    };

    using PosPeaks = stdext::inplace_vector<PosPeak, max_peaks_per_harmonic>;
    stdext::inplace_vector<
        HarmonicT<PosPeaks>,
        opts::CORRECTION_HARMONICS>
        peaks_by_harmonic;

    for (const auto &sweep : sweeps) {
        auto raw_peaks = find_peaks(sweep.value.begin(), sweep.value.end(), 0.2f);
        PosPeaks positioned;
        for (const auto &peak : raw_peaks) {
            positioned.push_back({ idx_to_pos(peak.idx) });
        }
        peaks_by_harmonic.push_back({ sweep.harmonic, std::move(positioned) });
    }

    // Try every detected peak as an anchor for harmonic matching.
    // For anchor at speed anchor_pos in harmonic h_anchor, the candidate
    // fundamental speed is anchor_pos * h_anchor. For each other harmonic h,
    // select the peak closest to fundamental / h (matching pos * h ≈ nominal).
    float best_badness = std::numeric_limits<float>::infinity();
    stdext::inplace_vector<HarmonicPeak, opts::CORRECTION_HARMONICS> best_selected;

    // Tip for reader: Yes, there are 4 nested for loops, which is quite a lot.
    //
    // However, the general idea is to have:
    //
    // for anchor in all_peaks():
    // 	 for peak in all_peaks():
    // 	    compare how good the match of peak against anchor is.
    //
    // But to iterate through all the peaks through all the harmonics, we need
    // 2 for loops to unpack them. Which gives us 2 outer and 2 inner for loops.
    for (std::size_t ai = 0; ai < peaks_by_harmonic.size(); ai++) {
        const int h_anchor = peaks_by_harmonic[ai].harmonic;
        const auto &anchor_peaks = peaks_by_harmonic[ai].value;

        for (const auto &anchor : anchor_peaks) {
            const float nominal_fundamental = anchor.position * h_anchor;

            // Select the closest peak for each harmonic
            float c_sum = 0.f;
            stdext::inplace_vector<HarmonicPeak, opts::CORRECTION_HARMONICS> selected;

            for (std::size_t hi = 0; hi < peaks_by_harmonic.size(); hi++) {
                const int h = peaks_by_harmonic[hi].harmonic;
                const auto &h_peaks = peaks_by_harmonic[hi].value;

                float closest_dist = std::numeric_limits<float>::infinity();
                float closest_pos = 0.f;
                for (const auto &peak : h_peaks) {
                    float dist = std::abs(peak.position * h - nominal_fundamental);
                    if (dist < closest_dist) {
                        closest_dist = dist;
                        closest_pos = peak.position;
                    }
                }
                selected.push_back({ h, closest_pos });
                c_sum += h * closest_pos;
            }

            // Fit: C_est = mean(h * pos), estimated position = C_est / h
            const float c_est = c_sum / static_cast<float>(peaks_by_harmonic.size());

            // Badness: sum of squared position errors
            float badness = 0.f;
            for (const auto &peak : selected) {
                float diff = peak.position - c_est / peak.harmonic;
                badness += diff * diff;
            }

            if (badness < best_badness) {
                best_badness = badness;
                best_selected = selected;
            }
        }
    }

    return best_selected;
}

// Find best peaks in the signal that are evenly spaced with the target_spacing.
// Return position of the first peak in the signal. If there are no fitting
// peaks, NaN is returned.
template <typename It, typename PosConverter>
float find_evenly_spaced_peaks(It begin, It end,
    PosConverter idx_to_pos, float target_spacing) {
    static_assert(std::is_same_v<typename std::iterator_traits<It>::value_type, float>);
    static_assert(std::is_same_v<typename std::iterator_traits<It>::iterator_category, std::random_access_iterator_tag>);

    const int signal_size = std::distance(begin, end);
    if (signal_size <= 1) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    // Convert target_spacing from position space to index space
    float pos_per_sample = (idx_to_pos(signal_size - 1) - idx_to_pos(0)) / (signal_size - 1);
    if (pos_per_sample <= 0) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    int chunk_size = static_cast<int>(std::round(target_spacing / pos_per_sample));
    if (chunk_size <= 0 || chunk_size >= signal_size) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    // Create summed signal by adding chunks together
    std::vector<float> summed_signal(chunk_size, 0.0f);
    int num_chunks = 0;

    for (int start_idx = 0; start_idx + chunk_size <= signal_size; start_idx += chunk_size) {
        for (int i = 0; i < chunk_size; ++i) {
            summed_signal[i] += *(begin + start_idx + i);
        }
        num_chunks++;
    }

    if (num_chunks == 0) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    // Find the maximum in the summed signal
    auto max_it = std::max_element(summed_signal.begin(), summed_signal.end());
    int max_idx = std::distance(summed_signal.begin(), max_it);

    // Convert back to position space
    return idx_to_pos(max_idx);
}

// Compute a centered moving average over ± half_window samples. The result is
// defined for all samples, even those at the beginning and end of the signal.
// The window just shrinks for those samples.
template <typename It, typename Yield>
void moving_average(It begin, It end, int half_window, Yield yield) {
    using T = typename std::iterator_traits<It>::value_type;
    static_assert(std::is_arithmetic_v<T>);

    const int total_size = std::distance(begin, end);
    if (total_size == 0) {
        return;
    }

    T sum = {};
    int sum_size = 0;

    It win_start = begin;
    It win_end = begin;
    // First accumulate half_window elements
    for (int i = 0; i < half_window && win_end != end; ++i) {
        sum += *win_end++;
        sum_size++;
    }
    // Compute elements with growing window
    for (int i = 0; i < half_window && win_end != end; ++i) {
        sum += *win_end++;
        sum_size++;
        yield(sum / sum_size);
    }

    // Compute elements with full window and then shrinking window
    for (int i = half_window; i < total_size; ++i) {
        if (win_end != end) {
            sum += *win_end++;
            sum_size++;
        }

        yield(sum / sum_size);

        sum -= *win_start++;
        sum_size--;
    }
}

// The same as above, but returns the result as a vector instead of yielding it
// via a callback.
template <typename Container>
Container moving_average(const Container &in, int half_window) {
    Container result;
    result.reserve(in.size());
    moving_average(in.begin(), in.end(), half_window, [&](typename Container::value_type x) {
        result.push_back(x);
    });
    return result;
}

// Provide a characteriation of motion for a speed sweep to guess the number of
// accelerometer samples in advance and to plan a starting position.
static MotionCharacteristics characterize_speed_sweep(const AxisCalibrationConfig &calib_config) {
    float ramp_distance = calib_config.max_movement_revs / 2;
    auto [start_speed, end_speed] = calib_config.speed_range;
    float ramp_duration = 2 * ramp_distance / (start_speed + end_speed);

    return {
        .revs = 2 * ramp_distance,
        .duration = 2 * ramp_duration + 6 * VIBRATION_SETTLE_TIME
    };
}

// Provide a characteriation of motion for a parameter sweep to guess the number
// of accelerometer samples in advance and to plan a starting position.
static MotionCharacteristics characterize_param_sweep(float speed, float duration) {
    float revs = (duration + 2 * VIBRATION_SETTLE_TIME) * speed;

    return {
        .revs = revs,
        .duration = duration + 8 * VIBRATION_SETTLE_TIME
    };
}

static void move_to_measurement_start(AxisEnum axis, float revs) {
    if (!axis_states[axis].inverted) {
        revs = -revs;
    }

    const float a_revs = axis == AxisEnum::A_AXIS ? revs : 0;
    const float b_revs = axis == AxisEnum::B_AXIS ? revs : 0;
    const auto [d_rot_x, d_rot_y] = physical_to_logical(a_revs, b_revs);
    const float dx = rev_to_mm(AxisEnum::X_AXIS, d_rot_x);
    const float dy = rev_to_mm(AxisEnum::Y_AXIS, d_rot_y);

    const float target_x = X_BED_SIZE / 2.f - dx / 2.f;
    const float target_y = Y_BED_SIZE / 2.f - dy / 2.f;

    do_blocking_move_to_xy(target_x, target_y);
    Planner::synchronize();
}

static void move_to_measurement_start(AxisEnum axis, MotionCharacteristics motion) {
    move_to_measurement_start(axis, motion.revs);
}

static float plan_no_movement_block(AxisEnum physical_axis, int direction, float duration) {
    debug_assert(direction == 1 || direction == -1);
    debug_assert(duration >= 0);

    // We fake no movement synchronous with planner via arbitrarily small speed.
    // We accelerate to this speed and then decelerate to zero.
    constexpr float NO_MOVEMENT_SPEED = 0.1f; // mm/s
    float accel = 2 * NO_MOVEMENT_SPEED / duration;
    float dist = direction * 2 * (0.5f * accel * duration * duration / 4); // Up and down

    auto [d_x, d_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? dist : 0,
        physical_axis == AxisEnum::Y_AXIS ? dist : 0);
    auto [speed_x, speed_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? NO_MOVEMENT_SPEED : 0,
        physical_axis == AxisEnum::Y_AXIS ? NO_MOVEMENT_SPEED : 0);
    auto [accel_x, accel_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? accel : 0,
        physical_axis == AxisEnum::Y_AXIS ? accel : 0);

    float block_speed = sqrt(speed_x * speed_x + speed_y * speed_y);
    float block_accel = sqrt(accel_x * accel_x + accel_y * accel_y);

    auto target = planner.get_machine_position_mm();
    target.x += d_x;
    target.y += d_y;

    Planner::buffer_raw_line(target, block_accel, block_speed, 0, 0, PhysicalToolIndex::currently_selected());
    set_current_position(to_native_pos(target));

    return duration;
}

static float plan_constant_movement_block(AxisEnum physical_axis, int direction, float speed, float revs) {
    debug_assert(physical_axis == AxisEnum::X_AXIS || physical_axis == AxisEnum::Y_AXIS);
    debug_assert(direction == 1 || direction == -1);
    debug_assert(speed >= 0);
    debug_assert(revs > 0);

    static const float DUMMY_ACCEL = 1;

    float duration = revs / speed;

    auto [x_revs, y_revs] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? direction * revs : 0,
        physical_axis == AxisEnum::Y_AXIS ? direction * revs : 0);
    auto [speed_x, speed_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? speed : 0,
        physical_axis == AxisEnum::Y_AXIS ? speed : 0);

    float speed_x_mm = rev_to_mm(AxisEnum::X_AXIS, speed_x);
    float speed_y_mm = rev_to_mm(AxisEnum::Y_AXIS, speed_y);
    float block_speed = sqrt(speed_x_mm * speed_x_mm + speed_y_mm * speed_y_mm);

    auto target = planner.get_machine_position_mm();
    target.x += rev_to_mm(AxisEnum::X_AXIS, x_revs);
    target.y += rev_to_mm(AxisEnum::Y_AXIS, y_revs);

    Planner::buffer_raw_line(target, DUMMY_ACCEL, block_speed, block_speed, block_speed, PhysicalToolIndex::currently_selected());
    set_current_position(to_native_pos(target));

    return duration;
}

static float plan_accel_block(AxisEnum physical_axis, int direction, float start_speed, float end_speed, float accel = 100) {
    debug_assert(physical_axis == AxisEnum::X_AXIS || physical_axis == AxisEnum::Y_AXIS);
    debug_assert(direction == 1 || direction == -1);
    debug_assert(start_speed >= 0 && end_speed >= 0);
    debug_assert(accel > 0);

    if (start_speed == end_speed) {
        return 0;
    }

    float duration = std::fabs((end_speed - start_speed)) / accel;
    float revs = direction * 0.5f * accel * duration * duration;

    auto [revs_x, revs_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? revs : 0,
        physical_axis == AxisEnum::Y_AXIS ? revs : 0);
    auto [start_speed_x, start_speed_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? start_speed : 0,
        physical_axis == AxisEnum::Y_AXIS ? start_speed : 0);
    auto [end_speed_x, end_speed_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? end_speed : 0,
        physical_axis == AxisEnum::Y_AXIS ? end_speed : 0);
    auto [accel_x, accel_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? accel : 0,
        physical_axis == AxisEnum::Y_AXIS ? accel : 0);

    float start_speed_x_mm = rev_to_mm(AxisEnum::X_AXIS, start_speed_x);
    float start_speed_y_mm = rev_to_mm(AxisEnum::Y_AXIS, start_speed_y);
    float block_start_speed = sqrt(start_speed_x_mm * start_speed_x_mm + start_speed_y_mm * start_speed_y_mm);

    float end_speed_x_mm = rev_to_mm(AxisEnum::X_AXIS, end_speed_x);
    float end_speed_y_mm = rev_to_mm(AxisEnum::Y_AXIS, end_speed_y);
    float block_end_speed = sqrt(end_speed_x_mm * end_speed_x_mm + end_speed_y_mm * end_speed_y_mm);

    float block_nominal_speed = std::max(block_start_speed, block_end_speed);

    float accel_x_mm = rev_to_mm(AxisEnum::X_AXIS, accel_x);
    float accel_y_mm = rev_to_mm(AxisEnum::Y_AXIS, accel_y);
    float block_accel = sqrt(accel_x_mm * accel_x_mm + accel_y_mm * accel_y_mm);

    auto target = planner.get_machine_position_mm();
    target.x += rev_to_mm(AxisEnum::X_AXIS, revs_x);
    target.y += rev_to_mm(AxisEnum::Y_AXIS, revs_y);

    Planner::buffer_raw_line(target, block_accel, block_nominal_speed, block_start_speed, block_end_speed, PhysicalToolIndex::currently_selected());
    set_current_position(to_native_pos(target));

    return duration;
}

static float plan_accel_over_dist_block(AxisEnum physical_axis, int direction,
    float start_speed, float end_speed, float revs) {
    debug_assert(direction == 1 || direction == -1);
    debug_assert(start_speed >= 0 && end_speed >= 0);
    debug_assert(start_speed != end_speed);
    debug_assert(revs > 0);

    // Plan a block with limited acceleration that moves from start speed to end
    // speed over revs revolutions. The assumption is that the printer already
    // accelerated to the start speed and there will be extra movement planned
    // after this block to decelerate.

    float accel_rev = std::fabs(end_speed * end_speed - start_speed * start_speed) / (2 * revs);

    auto [revs_x, revs_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? direction * revs : 0,
        physical_axis == AxisEnum::Y_AXIS ? direction * revs : 0);
    auto [accel_x, accel_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? accel_rev : 0,
        physical_axis == AxisEnum::Y_AXIS ? accel_rev : 0);
    auto [start_speed_x, start_speed_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? start_speed : 0,
        physical_axis == AxisEnum::Y_AXIS ? start_speed : 0);
    auto [end_speed_x, end_speed_y] = physical_to_logical(
        physical_axis == AxisEnum::X_AXIS ? end_speed : 0,
        physical_axis == AxisEnum::Y_AXIS ? end_speed : 0);

    float start_speed_x_mm = rev_to_mm(AxisEnum::X_AXIS, start_speed_x);
    float start_speed_y_mm = rev_to_mm(AxisEnum::Y_AXIS, start_speed_y);
    float block_start_speed = sqrt(start_speed_x_mm * start_speed_x_mm + start_speed_y_mm * start_speed_y_mm);

    float end_speed_x_mm = rev_to_mm(AxisEnum::X_AXIS, end_speed_x);
    float end_speed_y_mm = rev_to_mm(AxisEnum::Y_AXIS, end_speed_y);
    float block_end_speed = sqrt(end_speed_x_mm * end_speed_x_mm + end_speed_y_mm * end_speed_y_mm);

    float block_nominal_speed = std::max(block_start_speed, block_end_speed);

    float accel_x_mm = rev_to_mm(AxisEnum::X_AXIS, accel_x);
    float accel_y_mm = rev_to_mm(AxisEnum::Y_AXIS, accel_y);
    float block_accel = sqrt(accel_x_mm * accel_x_mm + accel_y_mm * accel_y_mm);

    auto target = planner.get_machine_position_mm();
    target.x += rev_to_mm(AxisEnum::X_AXIS, revs_x);
    target.y += rev_to_mm(AxisEnum::Y_AXIS, revs_y);

    Planner::buffer_raw_line(target, block_accel, block_nominal_speed, block_start_speed, block_end_speed, PhysicalToolIndex::currently_selected());
    set_current_position(to_native_pos(target));

    return 2 * revs / (start_speed + end_speed);
}

static float plan_marker_move(AxisEnum physical_axis, int direction) {
    static const float MARKER_SPEED = 2; // rev/s
    static const float MAKER_ACCEL = 1000; // rev/s^2

    float duration = 0;

    duration += plan_accel_block(physical_axis, direction, 0, MARKER_SPEED, MAKER_ACCEL);
    duration += plan_accel_block(physical_axis, direction, MARKER_SPEED, 0, MAKER_ACCEL);
    duration += plan_accel_block(physical_axis, -direction, 0, MARKER_SPEED, MAKER_ACCEL);
    duration += plan_accel_block(physical_axis, -direction, MARKER_SPEED, 0, MAKER_ACCEL);

    return duration;
}

// Captures samples of movement of a given axis. The movement should be already
// planned and executed. The function yields samples of acceleration in the
// direction of the axis. The function returns a tuple of:
// - sampling frequency,
// - movement success flag,
// - accelerometer error.
static std::tuple<float, bool, Error> capture_movement_samples(AxisEnum axis, int32_t timeout_ms,
    const YieldSample &yield_sample) {

    if (!wait_for_movement_state(phase_stepping::axis_states[axis], 300, [](phase_stepping::AxisState &s) {
            return phase_stepping::processing();
        })) {
        log_error(PhaseStepping, "Movement didn't start within timeout");
        return { 0, false, Error::none };
    }

    auto start_ts = ticks_ms();

    PrusaAccelerometer accelerometer;
    if (Error error = accelerometer.get_error(); error != Error::none) {
        log_error(PhaseStepping, "Cannot initialize accelerometer %u", static_cast<unsigned>(error));
        return { 0, true, error };
    }
    accelerometer.clear();

    while (Planner::busy() && ticks_diff(ticks_ms(), start_ts) < timeout_ms) {
        RawAcceleration sample;
        using GetSampleResult = PrusaAccelerometer::GetSampleResult;

        switch (accelerometer.get_sample_motor_coords(sample)) {
        case GetSampleResult::ok:
            yield_sample({ sample.val[axis] });
            break;

        case GetSampleResult::buffer_empty:
            idle(true);
            break;

        case GetSampleResult::error: {
            const Error error = accelerometer.get_error();
            log_error(PhaseStepping, "Accelerometer reading failed %u", static_cast<unsigned>(error));
            return { 0, true, error };
        }
        }
    }

    float sampling_freq = accelerometer.get_sampling_rate();
    debug_assert(sampling_freq != 0);
    const Error error = accelerometer.get_error();

    if (ticks_diff(ticks_ms(), start_ts) >= timeout_ms) {
        log_error(PhaseStepping, "Timeout while capturing samples");
        return { sampling_freq, false, error };
    }

    return { sampling_freq, true, error };
}

SamplesAnnotation phase_stepping::capture_param_sweep_samples(AxisEnum axis, float speed, float revs, int harmonic,
    const SweepParams &sweep_params, const YieldSample &yield_sample) {
    debug_assert(speed > 0);

    Planner::synchronize();

    phase_stepping::AxisState &axis_state = phase_stepping::axis_states[axis];

    int direction = revs > 0 ? 1 : -1;
    if (!axis_state.inverted) {
        direction = -direction;
    }
    revs = std::fabs(revs);

    internal::calibration_sweep = CalibrationSweep::build_for_motor(
        axis, harmonic, VIBRATION_SETTLE_TIME * speed, revs,
        sweep_params.pha_start, sweep_params.pha_end, sweep_params.mag_start, sweep_params.mag_end);

    // ensure the previous request has been consumed before resetting
    debug_assert(internal::calibration_axis_request < 0);
    internal::calibration_axis_request = axis_state.axis_index;

    float start_marker = 0;
    start_marker = plan_no_movement_block(axis, direction, VIBRATION_SETTLE_TIME);

    float signal_start = start_marker;
    signal_start += plan_marker_move(axis, direction);

    signal_start += plan_accel_block(axis, direction, 0, speed);

    float signal_end = signal_start;
    signal_end += plan_constant_movement_block(axis, direction, speed, revs + 2 * VIBRATION_SETTLE_TIME * speed);

    float end_marker = signal_end;
    end_marker += plan_accel_block(axis, direction, speed, 0);
    end_marker += plan_no_movement_block(axis, direction, VIBRATION_SETTLE_TIME);

    plan_marker_move(axis, direction);
    plan_no_movement_block(axis, direction, VIBRATION_SETTLE_TIME);

    // Adjust the signal start and end to account for extra time to settle
    // vibrations
    signal_start += VIBRATION_SETTLE_TIME;
    signal_end -= VIBRATION_SETTLE_TIME;

    int32_t timeout_ms = static_cast<int32_t>(1.5f * 1000 * end_marker);
    auto [sampling_freq, movement_ok, acc_error] = capture_movement_samples(axis, timeout_ms, yield_sample);

    return {
        .sampling_freq = sampling_freq,
        .movement_ok = movement_ok,
        .accel_error = acc_error,
        .start_marker = start_marker,
        .end_marker = end_marker,
        .signal_start = signal_start,
        .signal_end = signal_end
    };
}

SamplesAnnotation phase_stepping::capture_speed_sweep_samples(AxisEnum axis,
    float start_speed, float end_speed, float revs,
    const YieldSample &yield_sample) {

    debug_assert(start_speed >= 0 && end_speed >= 0);
    debug_assert(start_speed != 0 || start_speed != end_speed);

    Planner::synchronize();

    int direction = revs > 0 ? 1 : -1;
    if (!axis_states[axis].inverted) {
        direction = -direction;
    }
    revs = std::fabs(revs);
    revs /= 2;

    float start_marker = 0;
    start_marker += plan_no_movement_block(axis, direction, VIBRATION_SETTLE_TIME);

    float signal_start = start_marker;
    signal_start += plan_marker_move(axis, direction);
    signal_start += plan_no_movement_block(axis, direction, VIBRATION_SETTLE_TIME);
    signal_start += plan_accel_block(axis, direction, 0, start_speed);

    float signal_end = signal_start;
    signal_end += plan_accel_over_dist_block(axis, direction, start_speed, end_speed, std::fabs(revs));
    signal_end += plan_accel_over_dist_block(axis, direction, end_speed, start_speed, std::fabs(revs));

    float end_marker = signal_end;

    end_marker += plan_accel_block(axis, direction, start_speed, 0);
    end_marker += plan_no_movement_block(axis, direction, VIBRATION_SETTLE_TIME);
    plan_marker_move(axis, direction);
    plan_no_movement_block(axis, direction, VIBRATION_SETTLE_TIME);

    int32_t timeout_ms = static_cast<int32_t>(1.5f * 1000 * end_marker);
    auto [sampling_freq, movement_ok, acc_error] = capture_movement_samples(axis, timeout_ms, yield_sample);

    return {
        .sampling_freq = sampling_freq,
        .movement_ok = movement_ok,
        .accel_error = acc_error,
        .start_marker = start_marker,
        .end_marker = end_marker,
        .signal_start = signal_start,
        .signal_end = signal_end
    };
}

#ifdef SERIAL_DEBUG
// #error dead code found by automatic analyses (see BFW-5461)
static void serial_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

static void serial_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[256];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    SerialUSB.cdc_write_sync(reinterpret_cast<uint8_t *>(buf), strlen(buf));
}
#endif

static void debug_dump_raw_measurement(const char *name, const SignalContainer &signal,
    const SamplesAnnotation &annotation, const SignalView &signal_view) {
#ifdef SERIAL_DEBUG
    // #error dead code found by automatic analyses (see BFW-5461)
    serial_printf("# raw_signal {");
    serial_printf("\"name\": \"%s\", ", name);

    serial_printf("\"annotation\": ");
    serial_printf("{");
    serial_printf("\"sampling_freq\": %f, ", annotation.sampling_freq);
    serial_printf("\"movement_ok\": %d, ", annotation.movement_ok);
    serial_printf("\"accel_error\": %u, ", static_cast<unsigned>(annotation.accel_error));
    serial_printf("\"start_marker\": %f, ", annotation.start_marker);
    serial_printf("\"end_marker\": %f, ", annotation.end_marker);
    serial_printf("\"signal_start\": %f, ", annotation.signal_start);
    serial_printf("\"signal_end\": %f", annotation.signal_end);
    serial_printf("},");

    auto [start, end] = signal_view.bounds();
    serial_printf("\"signal_bounds\": [%d, %d], ", start, end);
    serial_printf("\"signal\": [");
    for (size_t i = 0; i < signal.size(); i++) {
        serial_printf("%f", convert(signal[i]));
        if (i + 1 < signal.size()) {
            serial_printf(", ");
        }
    }
    serial_printf("]}\n");
#endif
}

static void debug_dump_dft_sweep_result(const char *name, int harmonic, int dir, const DftSweepResult &result) {
#ifdef SERIAL_DEBUG
    // #error dead code found by automatic analyses (see BFW-5461)
    serial_printf("# dft_speed_sweep_result {");
    serial_printf("\"name\": \"%s%d\", ", name, dir);
    serial_printf("\"harmonic\": %d, ", harmonic);
    serial_printf("\"start_x\": %f, ", result.start_x);
    serial_printf("\"end_x\": %f, ", result.end_x);
    serial_printf("\"signal\": [");
    for (size_t i = 0; i < result.samples.size(); i++) {
        serial_printf("%f", result.samples[i]);
        if (i + 1 < result.samples.size()) {
            serial_printf(", ");
        }
    }
    serial_printf("]}\n");
#endif
}

static void debug_dump_harmonic_peaks(const char *name, const stdext::inplace_vector<HarmonicPeak, opts::CORRECTION_HARMONICS> &peaks) {
#ifdef SERIAL_DEBUG
    // #error dead code found by automatic analyses (see BFW-5461)
    serial_printf("# harmonic_peaks {");
    serial_printf("\"name\": \"%s\", ", name);
    serial_printf("\"peaks\": [");
    for (size_t i = 0; i < peaks.size(); i++) {
        serial_printf("{");
        serial_printf("\"harmonic\": %d, ", peaks[i].harmonic);
        serial_printf("\"measured_position\": %f,", peaks[i].position);
        serial_printf("\"estimated_position\": %f", peaks[i].position);
        serial_printf("}");
        if (i + 1 < peaks.size()) {
            serial_printf(", ");
        }
    }
    serial_printf("]}\n");
#endif
}

static void debug_dump_magnitude_search(int harmonic, float magnitude, const MagnitudeContainer &response) {
#ifdef SERIAL_DEBUG
    // #error dead code found by automatic analyses (see BFW-5461)
    serial_printf("# magnitude_search {");
    serial_printf("\"harmonic\": %d, ", harmonic);
    serial_printf("\"magnitude\": %f, ", magnitude);
    serial_printf("\"response\": [");
    for (size_t i = 0; i < response.size(); i++) {
        serial_printf("%f", response[i]);
        if (i + 1 < response.size()) {
            serial_printf(", ");
        }
    }
    serial_printf("]}\n");
#endif
}

static void debug_dump_param_search(int harmonic, const SweepParams &params, int move_dir, const MagnitudeContainer &response) {
#ifdef SERIAL_DEBUG
    // #error dead code found by automatic analyses (see BFW-5461)
    serial_printf("# param_search {");
    serial_printf("\"harmonic\": %d, ", harmonic);
    serial_printf("\"move_dir\": %d, ", move_dir);
    serial_printf("\"pha_start\": %f, ", params.pha_start);
    serial_printf("\"pha_end\": %f, ", params.pha_end);
    serial_printf("\"mag_start\": %f, ", params.mag_start);
    serial_printf("\"mag_end\": %f, ", params.mag_end);
    serial_printf("\"response\": [");
    for (size_t i = 0; i < response.size(); i++) {
        serial_printf("%f", response[i]);
        if (i + 1 < response.size()) {
            serial_printf(", ");
        }
    }
    serial_printf("]}\n");
#endif
}

// Analyze the motor and measure the best calibration speeds for each harmonic
// Returns a vector of harmonic frequencies and their corresponding speeds or an
// error message if the calibration failed.
static std::expected<stdext::inplace_vector<HarmonicT<float>, opts::CORRECTION_HARMONICS>, CalibrateAxisError>
measure_calibration_speeds(AxisEnum axis, const AxisCalibrationConfig &calibration_config, AbortFun should_abort) {
    if (calibration_config.speed_override.has_value()) {
        // We have an override. Just provide that and skip all the rest.
        stdext::inplace_vector<HarmonicT<float>, opts::CORRECTION_HARMONICS> result;
        for (size_t harmonic = 1; harmonic <= opts::CORRECTION_HARMONICS; harmonic++) {
            if (calibration_config.enabled_harmonics[harmonic - 1]) {
                result.emplace_back(harmonic, *calibration_config.speed_override / harmonic);
            }
        }
        return result;
    }

    auto move_characteristics = characterize_speed_sweep(calibration_config);
    auto [start_speed, end_speed] = calibration_config.speed_range;

    move_to_measurement_start(axis, move_characteristics);

    // First, capture the speed sweep there and back
    SignalContainer forward_samples;
    forward_samples.reserve(static_cast<size_t>(MAX_ACC_SAMPLING_RATE * SAMPLE_BUFFER_MARGIN * move_characteristics.duration));
    auto forward_annotation = capture_speed_sweep_samples(axis,
        start_speed, end_speed, calibration_config.max_movement_revs,
        [&](const auto &sample) {
            forward_samples.push_back(sample);
        });

    if (!forward_annotation.movement_ok || forward_annotation.accel_error != accelerometer::Error::none) {
        log_error(PhaseStepping, "Speed sweep movement failed: acc_error %u, movement_ok %d",
            static_cast<unsigned>(forward_annotation.accel_error), forward_annotation.movement_ok);
        return std::unexpected(CalibrateAxisError::speed_sweep_movement_failed);
    }
    ABORT_CHECK();
    auto forward_signal = locate_signal(forward_annotation, forward_samples);
    debug_dump_raw_measurement("forward", forward_samples, forward_annotation, forward_signal);
    debug_assert(forward_signal.size() != 0);
    ABORT_CHECK();

    SignalContainer backward_samples;
    backward_samples.reserve(static_cast<size_t>(MAX_ACC_SAMPLING_RATE * SAMPLE_BUFFER_MARGIN * move_characteristics.duration));
    auto backward_annotation = capture_speed_sweep_samples(axis,
        start_speed, end_speed, -calibration_config.max_movement_revs,
        [&](const auto &sample) {
            backward_samples.push_back(sample);
        });

    if (!backward_annotation.movement_ok || backward_annotation.accel_error != accelerometer::Error::none) {
        log_error(PhaseStepping, "Speed sweep movement failed: acc_error %u, movement_ok %d",
            static_cast<unsigned>(backward_annotation.accel_error), backward_annotation.movement_ok);
        return std::unexpected(CalibrateAxisError::speed_sweep_movement_failed);
    }
    ABORT_CHECK();
    auto backward_signal = locate_signal(backward_annotation, backward_samples);
    debug_dump_raw_measurement("backward", backward_samples, backward_annotation, backward_signal);
    debug_assert(backward_signal.size() != 0);
    ABORT_CHECK();

    // Then construct a combined signal for each harmonic from all measurements
    std::vector<HarmonicT<MagnitudeContainer>> harmonic_signals;
    for (int harmonic = 1; harmonic <= opts::CORRECTION_HARMONICS; harmonic++) {
        if (!calibration_config.enabled_harmonics[harmonic - 1]) {
            continue;
        }

        MagnitudeContainer combined;
        const float window_size = calibration_config.analysis_window_size_seconds;
        const int ramp_samples = forward_signal.size() / 2;
        const float time_step = ramp_samples / forward_annotation.sampling_freq / calibration_config.speed_sweep_bins;
        for (SweepDirection dir : { SweepDirection::Up, SweepDirection::Down }) {
            const auto &signal = dir == SweepDirection::Up ? forward_signal : backward_signal;
            const auto &annotation = dir == SweepDirection::Up ? forward_annotation : backward_annotation;

            auto [up_analysis, down_analysis] = motor_speed_dft_sweep(
                signal, annotation.sampling_freq, start_speed, end_speed,
                get_motor_steps(axis), harmonic, window_size, time_step);
            debug_assert(!up_analysis.samples.empty());

            ABORT_CHECK();

            debug_dump_dft_sweep_result("up", harmonic, dir == SweepDirection::Down, up_analysis);
            debug_dump_dft_sweep_result("down", harmonic, dir == SweepDirection::Down, down_analysis);

            if (dir == SweepDirection::Up) {
                combined.resize(up_analysis.samples.size());
            }

            int samples = std::min(combined.size(), up_analysis.samples.size());
            for (int i = 0; i < samples; i++) {
                combined[i] += up_analysis.samples[i] + down_analysis.samples[i];
            }
        }

        ABORT_CHECK();

        debug_assert(!combined.empty());
        harmonic_signals.emplace_back(harmonic, std::move(combined));
    }

    // Locate the peak positions
    auto idx_to_pos = [&](int idx) {
        return start_speed + idx * (end_speed - start_speed) / (harmonic_signals[0].value.size() - 1);
    };
    auto detected_peaks = find_harmonic_peaks(harmonic_signals, idx_to_pos);
    debug_dump_harmonic_peaks("peaks", detected_peaks);

    ABORT_CHECK();

    for (std::size_t i = 0; i < detected_peaks.size(); i++) {
        log_debug(PhaseStepping, "    peak %d at %f",
            detected_peaks[i].harmonic, detected_peaks[i].position);
    }

    stdext::inplace_vector<HarmonicT<float>, opts::CORRECTION_HARMONICS> result;
    for (const auto &peak : detected_peaks) {
        result.emplace_back(peak.harmonic, peak.position);
    }
    return result;
}

// Estimate the approximate magnitude of a harmonic at a given speed. The
// function returns the magnitude of the harmonic or an error message if the
// estimation failed.
static std::expected<float, CalibrateAxisError> find_approx_mag(AxisEnum axis,
    const AxisCalibrationConfig &calib_config, int harmonic, float nominal_calib_speed,
    AbortFun should_abort) {
    // Magnitude is estimated by performing a phase sweep with increasing
    // magnitudes in a geometric manner until a regression is detected.
    WithCorrectionDisabled disabler(axis, harmonic);

    const float speed = nominal_calib_speed * calib_config.peak_speed_shift;
    const float duration = calib_config.fine_movement_duration / 2;

    MotionCharacteristics move_characteristics = characterize_param_sweep(speed, duration);

    move_to_measurement_start(axis, move_characteristics);

    static const float PHA_START = -1.f;
    static const float PHA_END = 4 * std::numbers::pi_v<float> + 1.f;

    float last_minimum = std::numeric_limits<float>::infinity();
    float min_magnitude = std::numeric_limits<float>::infinity();
    float magnitude = calib_config.min_magnitude;
    int gone_worse_count = 0;
    int dir = 1;

    SignalContainer samples;
    samples.reserve(static_cast<size_t>(MAX_ACC_SAMPLING_RATE * SAMPLE_BUFFER_MARGIN * move_characteristics.duration));

    while (magnitude < calib_config.max_magnitude) {
        ABORT_CHECK();

        log_info(PhaseStepping, "Estimating magnitude %f (%d, %f)", magnitude,
            mag_to_fixed(magnitude), mag_to_float(mag_to_fixed(magnitude)));

        float revs = duration * speed * dir;
        samples.clear();

        SweepParams params {
            .pha_start = PHA_START,
            .pha_end = PHA_END,
            .mag_start = magnitude,
            .mag_end = magnitude
        };

        auto annotation = capture_param_sweep_samples(axis, speed,
            revs, harmonic, params, [&](AccelerometerSample sample) {
                samples.push_back(sample);
            });

        if (!annotation.movement_ok || annotation.accel_error != accelerometer::Error::none) {
            log_error(PhaseStepping, "Param sweep movement failed: acc_error %u, movement_ok %d",
                static_cast<unsigned>(annotation.accel_error), annotation.movement_ok);
            return std::unexpected(CalibrateAxisError::param_sweep_movement_failed);
        }
        ABORT_CHECK();

        auto signal = locate_signal(annotation, samples);

        int analysis_step = std::max(1, static_cast<int>(dir * revs * get_motor_steps(axis) / 4 / calib_config.param_sweep_bins));
        auto analysis = motor_harmonic_dft_sweep(signal, annotation.sampling_freq,
            speed, get_motor_steps(axis), harmonic, calib_config.analysis_window_periods, analysis_step);

        ABORT_CHECK();

        auto smoothed = moving_average(analysis.samples, analysis.samples.size() / 40);
        debug_dump_magnitude_search(harmonic, magnitude, smoothed);

        int margin = analysis.samples.size() / 10;
        auto min_it = std::min_element(analysis.samples.begin() + margin, analysis.samples.end() - margin);
        float min = *min_it;

        if (min < last_minimum) {
            last_minimum = min;
            min_magnitude = magnitude;
            gone_worse_count = 0;
        } else {
            gone_worse_count++;
        }

        if (gone_worse_count >= 2) {
            return min_magnitude;
        }

        magnitude *= calib_config.magnitude_quotient;
        dir = -dir;
    }
    if (gone_worse_count > 0) {
        return min_magnitude;
    } else {
        return std::unexpected(CalibrateAxisError::magnitude_out_of_bounds);
    }
}

// Perform a parametric sweep in both directions and collect a response. There
// are 4 moves in total: 2 movement and 2 sweep directions. The function returns
// a combined response for each movement direction or an error message if the
// sweep failed.
std::expected<std::array<std::vector<float>, 2>, CalibrateAxisError>
collect_param_sweep_response(AxisEnum axis, const AxisCalibrationConfig &calib_config,
    int harmonic, float speed, SweepParams f_params, SweepParams b_params,
    AbortFun should_abort) {
    MotionCharacteristics move_characteristics = characterize_param_sweep(speed, calib_config.fine_movement_duration);
    move_to_measurement_start(axis, move_characteristics);

    std::array<std::vector<float>, 2> responses;

    SignalContainer samples;
    samples.reserve(static_cast<size_t>(MAX_ACC_SAMPLING_RATE * SAMPLE_BUFFER_MARGIN * move_characteristics.duration));

    const float revs = calib_config.fine_movement_duration * speed;
    for (SweepDirection sweep_dir : { SweepDirection::Up, SweepDirection::Down }) {
        for (int movement_dir : { 1, -1 }) {
            ABORT_CHECK();

            auto param_set = movement_dir == 1 ? f_params : b_params;
            if (sweep_dir == SweepDirection::Down) {
                param_set = param_set.reverse();
            }

            samples.clear();
            auto annotation = capture_param_sweep_samples(axis, speed,
                movement_dir * revs, harmonic, param_set, [&](AccelerometerSample sample) {
                    samples.push_back(sample);
                });

            if (!annotation.movement_ok || annotation.accel_error != accelerometer::Error::none) {
                log_error(PhaseStepping, "Param sweep movement failed: acc_error %u, movement_ok %d",
                    static_cast<unsigned>(annotation.accel_error), annotation.movement_ok);
                return std::unexpected(CalibrateAxisError::param_sweep_movement_failed);
            }

            ABORT_CHECK();
            auto signal = locate_signal(annotation, samples);

            int analysis_step = std::max(1, static_cast<int>(movement_dir * revs * get_motor_steps(axis) / 4 / calib_config.param_sweep_bins));
            auto analysis = motor_harmonic_dft_sweep(signal, annotation.sampling_freq,
                speed, get_motor_steps(axis), harmonic, calib_config.analysis_window_periods, analysis_step);

            ABORT_CHECK();

            debug_dump_param_search(harmonic, param_set, movement_dir, analysis.samples);
            auto &response = movement_dir == 1 ? responses[0] : responses[1];
            if (sweep_dir == SweepDirection::Up) {
                response.clear();
                response.resize(analysis.samples.size());
            }

            // The direction of adding the responses together depends on the
            // sweep direction. If the sweep is up, the response is added
            // directly. If the sweep is down, the response is added in reverse
            // order so we add together matching samples.
            int elems = std::min(response.size(), analysis.samples.size());
            int iter_dir = sweep_dir == SweepDirection::Up ? 1 : -1;
            int analysis_start_idx = sweep_dir == SweepDirection::Up ? 0 : analysis.samples.size() - 1;
            for (int i = 0; i < elems; i++) {
                response[i] -= analysis.samples[analysis_start_idx + i * iter_dir];
            }
        }
    }

    return { std::move(responses) };
}

// Given an estimate of magnitude, find the best phase parameter for each
// movement direction. The function returns a tuple of phase parameters for
// forward and backward movement or an error message if the estimation failed.
std::expected<std::array<float, 2>, CalibrateAxisError> find_best_pha(AxisEnum axis,
    const AxisCalibrationConfig &calib_config, int harmonic, float nominal_calib_speed, float magnitude, AbortFun should_abort) {
    // To find a phase correction we perform 2 movements for each direction: one
    // with increasing phase sweep, one with decreasing phase sweep. We add
    // together the results of both sweep responses and identify the minima.
    // We repeat the full cycle PHA_CYCLES times to increase robustness.

    static const int PHA_CYCLES = 2;
    static const float PHA_START = -1.f;
    static const float PHA_END = PHA_CYCLES * 2 * std::numbers::pi_v<float> + 1.f;
    const float speed = nominal_calib_speed * calib_config.peak_speed_shift;

    SweepParams params {
        .pha_start = PHA_START,
        .pha_end = PHA_END,
        .mag_start = magnitude,
        .mag_end = magnitude
    };

    auto sweep_response = collect_param_sweep_response(axis, calib_config, harmonic, speed, params, params, should_abort);
    if (!sweep_response) {
        return std::unexpected(sweep_response.error());
    }

    std::array<float, 2> sweep_phases;
    for (std::size_t i = 0; i != sweep_response->size(); i++) {
        auto &response = (*sweep_response)[i];
        auto idx_to_phase = [&](int idx) {
            return PHA_START + idx * (PHA_END - PHA_START) / (response.size() - 1);
        };
        float peak_position = find_evenly_spaced_peaks(response.begin(), response.end(),
            idx_to_phase, 2 * std::numbers::pi_v<float>);
        if (std::isnan(peak_position)) {
            log_error(PhaseStepping, "Cannot find peaks in phase sweep");
            return std::unexpected(CalibrateAxisError::cannot_find_peaks_in_phase_sweep);
        }

        ABORT_CHECK();

        sweep_phases[i] = std::fmod(peak_position, 2 * std::numbers::pi_v<float>);
    }

    return sweep_phases;
}

// Given a precise measurement of phase for each direction, find the best
// magnitude. The function also extracts the percentage of harmonic improvement.
std::expected<std::array<MagCalibResult, 2>, CalibrateAxisError> find_best_mag(AxisEnum axis,
    const AxisCalibrationConfig &calib_config, int harmonic, float nominal_calib_speed,
    float forward_pha, float backward_pha, float guessed_magnitude,
    AbortFun should_abort) {
    const SweepParams f_params = {
        .pha_start = forward_pha,
        .pha_end = forward_pha,
        .mag_start = 0,
        .mag_end = 2 * guessed_magnitude
    };

    const SweepParams b_params = {
        .pha_start = backward_pha,
        .pha_end = backward_pha,
        .mag_start = 0,
        .mag_end = 2 * guessed_magnitude
    };

    const float speed = nominal_calib_speed * calib_config.peak_speed_shift;
    auto sweep_response = collect_param_sweep_response(axis, calib_config, harmonic, speed, f_params, b_params, should_abort);
    if (!sweep_response) {
        return std::unexpected(sweep_response.error());
    }

    ABORT_CHECK()

    std::array<MagCalibResult, 2> sweep_mags;
    for (std::size_t i = 0; i != sweep_response->size(); i++) {
        auto &response = (*sweep_response)[i];

        ABORT_CHECK();

        // There should be only a single peak in the response:
        auto max_it = std::max_element(response.begin(), response.end());
        float best_magnitude = std::distance(response.begin(), max_it) * 2 * guessed_magnitude / (response.size() - 1);

        auto baseline_it = std::min_element(response.begin(), max_it);

        float score = *max_it / *baseline_it;

        sweep_mags[i] = { best_magnitude, score };
    }

    return sweep_mags;
}

// Perform a calibration of a single harmonic in both directions. Returns a
// CalibrationResult for forward and backward direction. If the
// calibration fails, returns an error message.
static std::expected<std::array<CalibrationResult, 2>, CalibrateAxisError>
calibrate_single_harmonic(AxisEnum axis, const AxisCalibrationConfig &calib_config,
    int harmonic, float nominal_calib_speed, AbortFun should_abort) {
    log_debug(PhaseStepping, "Calibrating harmonic %d on speed %f", harmonic, nominal_calib_speed);
    WithCorrectionDisabled disabler(axis, harmonic);

    auto mag_result = find_approx_mag(axis, calib_config, harmonic, nominal_calib_speed, should_abort);
    if (!mag_result) {
        return std::unexpected(mag_result.error());
    }
    const float test_magnitude = *mag_result;
    log_info(PhaseStepping, "Found approximate magnitude for harmonic %d: %f", harmonic, test_magnitude);

    auto pha_result = find_best_pha(axis, calib_config, harmonic, nominal_calib_speed, test_magnitude, should_abort);
    if (!pha_result) {
        return std::unexpected(pha_result.error());
    }
    const auto [forward_pha, backward_pha] = *pha_result;
    log_info(PhaseStepping, "Found best phase for harmonic %d: %f, %f", harmonic, forward_pha, backward_pha);

    auto mag_calib_result = find_best_mag(axis, calib_config, harmonic, nominal_calib_speed,
        forward_pha, backward_pha, test_magnitude, should_abort);
    if (!mag_calib_result) {
        return std::unexpected(mag_calib_result.error());
    }
    const auto [forward_mag, backward_mag] = *mag_calib_result;
    log_info(PhaseStepping, "Found best magnitude for harmonic %d: %f, %f", harmonic, forward_mag.mag_value, backward_mag.mag_value);

    return { { CalibrationResult { forward_mag.mag_value, forward_pha, forward_mag.score },
        CalibrationResult { backward_mag.mag_value, backward_pha, backward_mag.score } } };
}

static bool makes_improvement(const CalibrationResult &r) {
    return r.score <= 1.f;
}

static bool has_impact(const CalibrationResult &r) {
    // If the magnitude yields phase shift less than one we consider it to be
    // negligible.
    static const float threshold = 1.f / opts::MOTOR_PERIOD / opts::SIN_FRACTION;
    return r.params.mag >= threshold;
}

[[gnu::noinline]]
static void calibrate_axis_body(AxisEnum axis, CalibrateAxisHooks &hooks, CalibrateAxisResult &result) {
    reset_compensation(axis);
    phase_stepping::EnsureEnabled _;

    auto should_abort = [&]() {
        idle(true);
        return hooks.on_idle() == phase_stepping::CalibrateAxisHooks::ContinueOrAbort::Abort;
    };

    const auto &calib_config = get_calibration_config(axis);

    hooks.on_motor_characterization_start();
    auto speed_analysis = with_retries(RETRY_COUNT, [&] {
        return measure_calibration_speeds(axis, calib_config, should_abort);
    });
    if (!speed_analysis) {
        log_error(PhaseStepping, "Speed analysis failed: %s", to_string(speed_analysis.error()));
        result = std::unexpected(speed_analysis.error());
        return;
    }

    int phases_count = std::count_if(speed_analysis->begin(), speed_analysis->end(),
        [](const auto &harmonic_speed) { return harmonic_speed.harmonic != 0; });
    hooks.on_motor_characterization_result(phases_count);

    auto &ok_result = result.emplace();
    int current_phase = 0;
    // We first calibrate even harmonics, then odd harmonics
    for (int parity = 0; parity < 2; parity++) {
        for (const auto &harmonic_speed : *speed_analysis) {
            if (harmonic_speed.harmonic % 2 != parity) {
                continue;
            }

            hooks.on_enter_calibration_phase(current_phase++);
            auto calib_res = with_retries(RETRY_COUNT, [&] {
                return calibrate_single_harmonic(axis, calib_config,
                    harmonic_speed.harmonic, harmonic_speed.value, should_abort);
            });
            if (!calib_res) {
                log_error(PhaseStepping, "Calibration failed: %s", to_string(calib_res.error()));
                result = std::unexpected(calib_res.error());
                return;
            }

            auto &[f_result, b_result] = *calib_res;

            // Motor might not have a defect on the current harmonics or the
            // driver resolution might not be enough to fix it. In that case,
            // we don't want measurement error to trigger false negative.
            for (auto *res : { &f_result, &b_result }) {
                if (!has_impact(*res) && !makes_improvement(*res)) {
                    res->score = 1.f; // No impact, no improvement
                }
            }

            ok_result[0][harmonic_speed.harmonic] = f_result.params;
            ok_result[1][harmonic_speed.harmonic] = b_result.params;
            hooks.on_calibration_phase_result(f_result.score, b_result.score);

            // Apply the correction to the motor
            phase_stepping::axis_states[axis].forward_current.modify_correction_table([&](auto &table) {
                table[harmonic_speed.harmonic] = f_result.params;
            });
            phase_stepping::axis_states[axis].backward_current.modify_correction_table([&](auto &table) {
                table[harmonic_speed.harmonic] = b_result.params;
            });
        }
    }

    hooks.on_termination();
}

void phase_stepping::calibrate_axis(AxisEnum axis, CalibrateAxisHooks &hooks, CalibrateAxisResult &result) {
    mapi::ensure_tool_with_accelerometer_picked();
    calibrate_axis_body(axis, hooks, result); // body extracted to delay stack allocation
}

const char *phase_stepping::to_string(CalibrateAxisError err) {
    switch (err) {
    case CalibrateAxisError::aborted:
        return N_("Aborted.");
    case CalibrateAxisError::speed_sweep_movement_failed:
        return N_("Speed sweep movement failed.");
    case CalibrateAxisError::param_sweep_movement_failed:
        return N_("Param sweep movement failed.");
    case CalibrateAxisError::no_peaks_found:
        return N_("No peaks found.");
    case CalibrateAxisError::cannot_find_peaks_in_phase_sweep:
        return N_("Cannot find peaks in phase sweep.");
    case CalibrateAxisError::magnitude_out_of_bounds:
        return N_("Magnitude out of bounds.");
    }
    bsod_unreachable();
}

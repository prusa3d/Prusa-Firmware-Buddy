#pragma once

#include "Pin.hpp"
#include <inttypes.h>
#include "cmsis_os.h"
#include <cstring>
#include <array>
#include <bsod.h>
#include <limits>
#include "probe_analysis.hpp"
#include <atomic>
#include <printers.h>
#include <option/has_indx.h>

class Loadcell {
public:
    enum class TareMode : uint8_t {
        Continuous,
        Static,
    };

    Loadcell();

    static constexpr int32_t undefined_value = std::numeric_limits<int32_t>::min();
    static constexpr int UNDEFINED_INIT_MAX_CNT = 6; // Maximum number of undefined samples to ignore during startup (>=0)
    static constexpr int UNDEFINED_SAMPLE_MAX_CNT = 2; // About 6ms of stale data @ 320Hz, 78ms over a channel switch
    static constexpr unsigned int STATIC_TARE_SAMPLE_CNT = 48; // 150ms of data @ 320Hz
    static constexpr unsigned int TOUCHDOWN_DELAY_MS = 150; // Milliseconds of pause required after trigger for the analysis model
    static constexpr int32_t MAX_LOADCELL_DATA_AGE_US = 100'000; // Older sample means the stream stalled

    /// Default XY probe trigger threshold/hysteresis [g]; the active values can be overridden per probing
    /// sequence via XyProbeThresholdOverride.
    static constexpr float XY_PROBE_THRESHOLD { 40 };
    static constexpr float XY_PROBE_HYSTERESIS { 20 };

    static constexpr unsigned int ANALYSIS_WINDOW_SIZE = HAS_INDX() ? 600 : 430; // Effective window size (INDX loadcell has higher sampling freq -> need longer window to cover)
    static constexpr float MIN_ANALYSIS_WINDOW_SIZE = buddy::ProbeAnalysisBase::initialFrequency
        * (TOUCHDOWN_DELAY_MS / 1000.f + buddy::ProbeAnalysisBase::analysisLookback + buddy::ProbeAnalysisBase::analysisLookahead);
    static_assert(ANALYSIS_WINDOW_SIZE >= MIN_ANALYSIS_WINDOW_SIZE);

    buddy::ProbeAnalysis<ANALYSIS_WINDOW_SIZE> analysis;
    std::atomic<bool> xy_endstop_enabled { false };
    static_assert(std::atomic<decltype(xy_endstop_enabled)::value_type>::is_always_lock_free, "Lock free type must be used from ISR.");

    /**
     * @brief Wait until a loadcell sample with the specified time is received
     * Wait until at least one sample with the specified timestamp is received.
     * @see WaitBarrier() to wait for a sample at the current time.
     */
    void WaitBarrier(uint32_t ticks_us);

    /**
     * @brief Wait until a new loadcell sample at current time is received
     */
    void WaitBarrier() { WaitBarrier(ticks_us()); }

    /**
     * @brief Zero loadcell offset on current load.
     * @param mode use either static offset or continuous bandpass filter
     * @return measured offset value, informative, can be ignored [grams]
     */
    float Tare(TareMode mode = TareMode::Static);

    /**
     * @brief Clear state when no tool is picked.
     */
    void Clear();

    /**
     * @brief Reset filters
     *
     * Filtered data will be invalid after this call until the filters settle.
     */
    void reset_filters();

    /// Resets endstops to not triggered state
    void reset_endstops();

    float GetScale() const;

    void SetScale(float new_scale);

    void set_xy_endstop(const bool enabled);

    inline float GetThreshold(TareMode tareMode = TareMode::Static) const {
        switch (tareMode) {
        case TareMode::Static:
            return thresholdStatic;
        case TareMode::Continuous:
            return thresholdContinuous;
        }
        return 0;
    }

    float GetHysteresis() const;

    void ProcessSample(int32_t loadcellRaw, uint32_t time_us, uint32_t source_generation);
    inline uint32_t GetLastSampleTimeUs() const { return last_sample_time_us; }

    bool GetMinZEndstop() const;
    bool GetXYEndstop() const;

    // return loadcell load in grams
    inline static float get_tared_z_load(int32_t raw_sample, float scale, float offset) { return (scale * (raw_sample - offset)); }

    int32_t get_raw_value() const;

    /// @brief Request highest precision available from loadcell
    inline void EnableHighPrecision() {
        debug_assert(!highPrecision); // ensure HP is not recursively enabled
        reset_filters(); // reset filters before we turn on HP
        highPrecision = true;
    }
    inline void DisableHighPrecision() {
        debug_assert(highPrecision); // ensure HP is not recursively disabled
        highPrecision = false;
        probe_safety_armed.store(false); // safety net: disarm on HP exit
        reset_endstops();
    }
    inline bool IsHighPrecisionEnabled() const { return highPrecision; }

    void SetFailsOnLoadAbove(float failsOnLoadAbove);
    float GetFailsOnLoadAbove() const;

    /// Active XY probe trigger threshold/hysteresis [g]. Prefer the XyProbeThresholdOverride RAII guard
    /// over calling these directly so the values are always restored.
    void SetXyProbeThreshold(float threshold_g, float hysteresis_g);
    float GetXyProbeThreshold() const { return xy_probe_threshold; }
    float GetXyProbeHysteresis() const { return xy_probe_hysteresis; }

    void SetFailsOnLoadBelow(float failsOnLoadBelow);
    float GetFailsOnLoadBelow() const;

    /// Checks loadcell sample freshness during a probe/homing session; stops Z (failed probe) if samples stop arriving.
    void HomingSafetyCheck();

    /// Wait until a sample no older than max_age_us has arrived, or timeout_us elapses.
    /// Bails early on planner draining / stepper stopping / a tripped probe-safety (@see probe_should_abort()).
    /// @return true iff a fresh sample is currently streaming.
    bool wait_for_fresh_sample(uint32_t timeout_us, uint32_t max_age_us = MAX_LOADCELL_DATA_AGE_US);

    /// True when an in-progress probe/homing wait must abort: motion is being
    /// cancelled (planner draining or a quick-stop in flight) or the loadcell
    /// safety stop has tripped. @see WaitBarrier(), run_z_probe().
    bool probe_should_abort() const;

    /// True if the probe-safety stop tripped on an anomaly (sticky until cleared by disarm/arm or a tare).
    inline bool probe_safety_did_trip() const { return probe_safety_tripped.load(); }

    /// Disarm the probe-safety stop and clear any pending trip
    /// (use around moves that must not be quick-stopped, e.g. XY travel).
    inline void disarm_probe_safety() {
        probe_safety_armed.store(false);
        probe_safety_tripped.store(false);
    }

    /// Arm the probe-safety for a tare + Z-descent window. Also syncs tare_generation
    /// so the arm→tare window can't trip on a stale generation (Tare() re-syncs again).
    inline void arm_probe_safety() {
        tare_generation.store(last_source_generation.load());
        probe_safety_tripped.store(false);
        probe_safety_armed.store(true);
    }

    class FailureOnLoadAboveEnforcer {
    public:
        FailureOnLoadAboveEnforcer(Loadcell &lcell, bool enable, float grams);
        FailureOnLoadAboveEnforcer(FailureOnLoadAboveEnforcer &&) = default;
        ~FailureOnLoadAboveEnforcer();

    private:
        Loadcell &lcell;
        float oldErrThreshold;
    };

    /// RAII guard: enables high precision on construction, disables on destruction.
    /// The `enable` flag allows conditional use without scoping the guard inside an if block.
    /// Safe to nest.
    class HighPrecisionEnabler {
    public:
        HighPrecisionEnabler(Loadcell &lcell, bool enable = true);
        ~HighPrecisionEnabler();

        HighPrecisionEnabler(const HighPrecisionEnabler &) = delete;
        HighPrecisionEnabler &operator=(const HighPrecisionEnabler &) = delete;
        HighPrecisionEnabler(HighPrecisionEnabler &&) = delete;
        HighPrecisionEnabler &operator=(HighPrecisionEnabler &&) = delete;

    private:
        Loadcell &m_lcell;
        bool m_owns_high_precision;
    };

    /// RAII guard: arms probe-safety on construction, disarms on destruction.
    /// The `arm` flag allows conditional use without scoping the guard inside an if block.
    class ProbeSafetyArmer {
    public:
        ProbeSafetyArmer(Loadcell &lcell, bool arm = true);
        ProbeSafetyArmer(ProbeSafetyArmer &&) = default;
        ~ProbeSafetyArmer();

    private:
        Loadcell &m_lcell;
        bool m_armed;
    };

    /// RAII guard: overrides the XY probe trigger threshold for one probing sequence and restores it on
    /// destruction (hysteresis scaled to keep the default ratio).
    class XyProbeThresholdOverride {
    public:
        XyProbeThresholdOverride(Loadcell &lcell, float threshold_g);
        // Non-movable: a defaulted move would leave the moved-from restore twice.
        XyProbeThresholdOverride(XyProbeThresholdOverride &&) = delete;
        ~XyProbeThresholdOverride();

    private:
        Loadcell &m_lcell;
        float m_old_threshold;
        float m_old_hysteresis;
    };

    FailureOnLoadAboveEnforcer CreateLoadAboveErrEnforcer(bool enable = true, float grams = 3000);

private:
    /// Stop Z immediately and fail the probe (no-op unless a probe session is armed).
    void probe_safety_stop();

#if PRINTER_IS_PRUSA_XL()
    // Tweaked butter(2, [0.07 0.11])
    struct ZFilterParams {
        static constexpr float gain = 276.1148366795870f;
        static constexpr std::array<const float, 5> a = { { 1, -3.678167822936356f, 5.211060348827695f, -3.364842682922483f, 0.837181651256023f } };
    };
#elif HAS_INDX()
    // butter(2, [0.01290492, 0.10365027]) — same ~2.4-19.0Hz passband as MK4, recalculated for 366Hz sampling
    struct ZFilterParams {
        static constexpr float gain = 5.9405335153e+01f;
        static constexpr std::array<const float, 5> a = { { 1.0f, -3.5770043970f, 4.8268008110f, -2.9178955271f, 0.6682431852f } };
    };
#else /*PRINTER*/
    // Original
    struct ZFilterParams {
        static constexpr float gain = 5.724846511e+01f;
        static constexpr std::array<const float, 5> a = { { 1.0f, -3.6132919084f, 4.9481816585f, -3.0510427201f, 0.7164075250f } };
    };
#endif /*PRINTER*/

    // Tweaked butter(2, [0.005 0.08])
    // Consider increasing the low threshold in case the offset takes to long to remove
    struct XYFilterParam {
        static constexpr float gain = 1 / 1.185768264324116e-02f;
        static constexpr std::array<const float, 5> a = { { 1.0f, -3.661929127367906f, 5.041628953899732f, -3.096320393316955f, 0.716633873504158f } };
    };

    /// Implements IIR bandpass filter
    template <typename PARAM>
    class BandPassFilter {
    public:
        static constexpr int NZEROS = 4;
        static constexpr int NPOLES = 4;

        /**
         * @brief Filter coefficients.
         *
         * B is fixed to be [1   0  -2   0   1] / GAIN as used by octave's signal package or matlab.
         * Obtained by B = b/b(1) and GAIN = 1/b(1), where b is in octave/matlab format.
         *
         * A is [1  a1  a2  a3  a4] as used by octave/matlab.
         */
        static constexpr std::array<const float, NPOLES + 1> A = PARAM::a;

        /**
         * @brief Filter gain
         *
         * GAIN = 1/b(1), where b is in octave/matlab format
         */
        static constexpr float GAIN = PARAM::gain;

        BandPassFilter() {
            reset();
        }

        inline void reset() {
            initialized_ = false;
        }

        [[nodiscard]] inline float filter(float input) {
            static_assert(NZEROS == 4, "This code works only for NZEROS == 4");
            static_assert(NPOLES == 4, "This code works only for NPOLES == 4");
            static_assert(A[0] == 1, "This code works only A[0] == 1");

            input /= GAIN;

            if (!initialized_) {
                initialized_ = true;

                // Initialize the filter as if it has been receiving the same "input" sample infinitely

                // xv is just a history of the input samples
                for (auto &x : xv) {
                    x = input;
                }

                // yv is the history of the bandpass filter output
                // since we're "simulating" the same output since the beginning of time, it should be zero
                for (auto &y : yv) {
                    y = 0;
                }

            } else {
                xv[0] = xv[1];
                xv[1] = xv[2];
                xv[2] = xv[3];
                xv[3] = xv[4];
                xv[4] = input;

                yv[0] = yv[1];
                yv[1] = yv[2];
                yv[2] = yv[3];
                yv[3] = yv[4];

                yv[4] =
                    // Note: xv cancels each other out if it is the same the whole time
                    xv[0] + -2 * xv[2] + xv[4]

                    // Feedback from the previous filter outputs
                    - A[4] * yv[0] - A[3] * yv[1] - A[2] * yv[2] - A[1] * yv[3];
            }

            return yv[4];
        }

        inline bool initialized() const {
            return initialized_;
        }

    private:
        float xv[NZEROS + 1];
        float yv[NPOLES + 1];
        bool initialized_ = false;
    };

#if HAS_INDX()
    static constexpr float scale_default = 0.0128f;
#else
    static constexpr float scale_default = 0.0192f;
#endif
    static constexpr float thresholdStatic = -125.f;
#if HAS_INDX()
    static constexpr float thresholdContinuous = -50.f;
#else
    static constexpr float thresholdContinuous = -40.f;
#endif
    static constexpr float hysteresis = 80.f;

    float scale;

    float failsOnLoadAbove;
    float failsOnLoadBelow;

    int32_t loadcellRaw; // current sample
    int undefinedCnt; // undefined sample run length

    bool endstop = false;
    std::atomic<bool> xy_endstop = false;
    static_assert(std::atomic<decltype(xy_endstop)::value_type>::is_always_lock_free, "Lock free type must be used from ISR.");

    /// Active XY probe trigger threshold/hysteresis [g], read from the ISR each sample. Default to the
    /// XY_PROBE_* constants; temporarily lowered via XyProbeThresholdOverride.
    std::atomic<float> xy_probe_threshold { XY_PROBE_THRESHOLD };
    static_assert(std::atomic<float>::is_always_lock_free, "Lock free type must be used from ISR.");
    std::atomic<float> xy_probe_hysteresis { XY_PROBE_HYSTERESIS };

    bool highPrecision;

    // When tare is requested, this will store number of samples and countdown to zero
    std::atomic<uint32_t> tareCount;
    static_assert(std::atomic<decltype(tareCount)::value_type>::is_always_lock_free, "Lock free type must be used from ISR.");
    // This will contain summed samples from tare
    int32_t tareSum;

    TareMode tareMode;
    // used when tareMode == Static
    int32_t offset;
    // used when tareMode == Continuous
    BandPassFilter<ZFilterParams> z_filter;

    // Filter for XY probes
    BandPassFilter<XYFilterParam> xy_filter;

    /// Time when last valid sample arrived
    // atomic because its set in interrupt/puppytask, read in default task
    std::atomic<uint32_t> last_sample_time_us;
    static_assert(std::atomic<decltype(last_sample_time_us)::value_type>::is_always_lock_free, "Lock free type must be used from ISR.");

    /// Generation of the data source (puppy reset counter); 0 for sources that cannot reset (direct HX717).
    std::atomic<uint32_t> last_source_generation { 0 };
    static_assert(std::atomic<decltype(last_source_generation)::value_type>::is_always_lock_free, "Lock free type must be used from ISR.");

    /// Source generation the current tare is valid for; a sample whose generation differs
    /// means the head reset since the tare, so the tare is stale.
    std::atomic<uint32_t> tare_generation { 0 };
    static_assert(std::atomic<decltype(tare_generation)::value_type>::is_always_lock_free, "Lock free type must be used from ISR.");

    /// True only during a probe/homing session: gates the immediate safety stop.
    std::atomic<bool> probe_safety_armed { false };
    static_assert(std::atomic<decltype(probe_safety_armed)::value_type>::is_always_lock_free, "Lock free type must be used from ISR.");

    /// Set when a probe-safety stop fired; sticky (unlike PreciseStepping's stop flag,
    /// which idle() clears) so probe_should_abort() keeps returning true once tripped.
    /// Cleared at the start of each tare / probe session.
    std::atomic<bool> probe_safety_tripped { false };
    static_assert(std::atomic<decltype(probe_safety_tripped)::value_type>::is_always_lock_free, "Lock free type must be used from ISR.");
};

extern Loadcell loadcell;

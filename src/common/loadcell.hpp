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

class Loadcell {
public:
    enum class TareMode {
        Continuous,
        Static,
    };

    Loadcell();

    static constexpr int32_t undefined_value = std::numeric_limits<int32_t>::min();
    static constexpr int UNDEFINED_INIT_MAX_CNT = 6; // Maximum number of undefined samples to ignore during startup (>=0)
    static constexpr int UNDEFINED_SAMPLE_MAX_CNT = 2; // About 6ms of stale data @ 320Hz, 78ms over a channel switch
    static constexpr unsigned int STATIC_TARE_SAMPLE_CNT = 16;

    static constexpr float XY_PROBE_THRESHOLD { 40 };
    static constexpr float XY_PROBE_HYSTERESIS { 20 };

    buddy::ProbeAnalysis<320> analysis;
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

    void SetScale(float scale);
    float GetScale() const;

    void set_xy_endstop(const bool enabled);

    inline void SetThreshold(float threshold, TareMode tareMode) {
        switch (tareMode) {
        case TareMode::Static:
            thresholdStatic = threshold;
            break;
        case TareMode::Continuous:
            thresholdContinuous = threshold;
            break;
        }
    }

    inline float GetThreshold(TareMode tareMode = TareMode::Static) const {
        switch (tareMode) {
        case TareMode::Static:
            return thresholdStatic;
        case TareMode::Continuous:
            return thresholdContinuous;
        }
        return 0;
    }

    void SetHysteresis(float hysteresis);
    float GetHysteresis() const;

    void ProcessSample(int32_t loadcellRaw, uint32_t time_us);
    inline uint32_t GetLastSampleTimeUs() const { return last_sample_time_us; }

    bool GetMinZEndstop() const;
    bool GetXYEndstop() const;

    // return loadcell load in grams
    inline float get_tared_z_load() const { return (scale * (loadcellRaw - offset)); }
    inline float get_filtered_z_load() const { return z_filter.get_output() * scale; }
    inline float get_filtered_xy() const { return xy_filter.get_output() * scale; }

    int32_t get_raw_value() const;

    /// @brief Request highest precision available from loadcell
    inline void EnableHighPrecision() {
        assert(!highPrecision); // ensure HP is not recursively enabled
        reset_filters(); // reset filters before we turn on HP
        highPrecision = true;
    }
    inline void DisableHighPrecision() {
        assert(highPrecision); // ensure HP is not recursively disabled
        highPrecision = false;
    }
    inline bool IsHighPrecisionEnabled() const { return highPrecision; }

    void SetFailsOnLoadAbove(float failsOnLoadAbove);
    float GetFailsOnLoadAbove() const;

    void SetFailsOnLoadBelow(float failsOnLoadBelow);
    float GetFailsOnLoadBelow() const;

    /// @brief To be called during homing, will raise redsceen when samples stop comming during homing
    void HomingSafetyCheck() const;

    class IFailureEnforcer {
    protected:
        Loadcell &lcell;
        float oldErrThreshold;
        IFailureEnforcer(Loadcell &lcell, float oldErrThreshold);
        IFailureEnforcer(const IFailureEnforcer &) = delete;
        IFailureEnforcer(IFailureEnforcer &&) = default;
    };

    class FailureOnLoadAboveEnforcer : public IFailureEnforcer {
    public:
        FailureOnLoadAboveEnforcer(Loadcell &lcell, bool enable, float grams);
        FailureOnLoadAboveEnforcer(FailureOnLoadAboveEnforcer &&) = default;
        ~FailureOnLoadAboveEnforcer();
    };

    class FailureOnLoadBelowEnforcer : public IFailureEnforcer {
    public:
        FailureOnLoadBelowEnforcer(Loadcell &lcell, float grams);
        FailureOnLoadBelowEnforcer(FailureOnLoadBelowEnforcer &&) = default;
        ~FailureOnLoadBelowEnforcer();
    };

    class HighPrecisionEnabler {
    public:
        HighPrecisionEnabler(Loadcell &lcell, bool enable = true);
        HighPrecisionEnabler(HighPrecisionEnabler &&) = default;
        ~HighPrecisionEnabler();

    private:
        Loadcell &m_lcell;
        bool m_enable;
    };

    FailureOnLoadAboveEnforcer CreateLoadAboveErrEnforcer(bool enable = true, float grams = 3000);

private:
#if PRINTER_IS_PRUSA_XL()
    // Tweaked butter(2, [0.07 0.11])
    struct ZFilterParams {
        static constexpr float gain = 276.1148366795870;
        static constexpr std::array<const float, 5> a = { { 1, -3.678167822936356, 5.211060348827695, -3.364842682922483, 0.837181651256023 } };
        static constexpr size_t settling_time = 120; // 375ms
    };
#else /*PRINTER*/
    // Original
    struct ZFilterParams {
        static constexpr float gain = 5.724846511e+01f;
        static constexpr std::array<const float, 5> a = { { 1, -3.6132919084, 4.9481816585, -3.0510427201, 0.7164075250 } };
        static constexpr size_t settling_time = 120; // 375ms
    };
#endif /*PRINTER*/

    // Tweaked butter(2, [0.005 0.08])
    // Consider increasing the low threshold in case the offset takes to long to remove
    struct XYFilterParam {
        static constexpr float gain = 1 / 1.185768264324116e-02;
        static constexpr std::array<const float, 5> a = { { 1, -3.661929127367906, 5.041628953899732, -3.096320393316955, 0.716633873504158 } };
        static constexpr size_t settling_time = 120; // 375ms
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

        /**
         * @brief Settling time in samples.
         * The filter must get from 14000 loadcell offset under 40 threshold.
         * Exact value could be calculated from the filter exponentials, but quick approximation is
         * octave command "max(find(abs(impz(B, A)) > 40/14000))". Plus a small reserve.
         */
        static constexpr size_t SETTLING_TIME = PARAM::settling_time;

        BandPassFilter() {
            reset();
        }

        inline void reset() {
            std::memset(&xv, 0, sizeof(xv));
            std::memset(&yv, 0, sizeof(yv));
            samples = 0;
        }

        inline float filter(float input) {
            static_assert(NZEROS == 4, "This code works only for NZEROS == 4");
            static_assert(NPOLES == 4, "This code works only for NPOLES == 4");
            static_assert(A[0] == 1, "This code works only A[0] == 1");
            if (samples < SETTLING_TIME) {
                ++samples;
            }

            xv[0] = xv[1];
            xv[1] = xv[2];
            xv[2] = xv[3];
            xv[3] = xv[4];
            xv[4] = input / GAIN;
            yv[0] = yv[1];
            yv[1] = yv[2];
            yv[2] = yv[3];
            yv[3] = yv[4];
            yv[4] = xv[0] + -2 * xv[2] + xv[4] - A[4] * yv[0] - A[3] * yv[1] - A[2] * yv[2] - A[1] * yv[3];
            return yv[4];
        }

        inline float get_output() const {
            return yv[std::size(yv) - 1];
        }

        inline bool settled() const {
            return (samples == SETTLING_TIME);
        }

    private:
        float xv[NZEROS + 1];
        float yv[NPOLES + 1];
        unsigned int samples; ///< Samples fed until SETTLING_TIME is reached
    };

    float scale;
    float thresholdStatic;
    float thresholdContinuous;
    float hysteresis;
    float failsOnLoadAbove;
    float failsOnLoadBelow;

    int32_t loadcellRaw; // current sample
    int undefinedCnt; // undefined sample run length

    bool endstop;
    std::atomic<bool> xy_endstop;
    static_assert(std::atomic<decltype(xy_endstop)::value_type>::is_always_lock_free, "Lock free type must be used from ISR.");
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
};

extern Loadcell loadcell;

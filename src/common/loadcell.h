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

    static constexpr float XY_PROBE_THRESHOLD { 40 };
    static constexpr float XY_PROBE_HYSTERESIS { 20 };

    buddy::ProbeAnalysis<300> analysis;
    std::atomic<bool> xy_endstop_enabled { false };

    void Tare(TareMode mode = TareMode::Static, bool wait = true);

    /**
     * @brief Clear state when no tool is picked.
     */
    void Clear();

    /**
     * @brief Reset filters
     *
     * FIltered data will be invalid after this call until the filters settle.
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
    inline float GetThreshold(TareMode tareMode) const {
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
    inline uint32_t GetLastSampleTime() const { return last_sample_time; }

    bool GetMinZEndstop() const;
    bool GetXYEndstop() const;

    void ConfigureSignalEvent(osThreadId threadId, int32_t signal);

    // return loadcell load in grams
    inline float get_tared_z_load() const { return (scale * (loadcellRaw - offset)); }
    inline float get_filtered_z_load() const { return z_filter.get_output() * scale; }
    inline float get_filtered_xy() const { return xy_filter.get_output() * scale; }

    int32_t get_raw_value() const;

    bool IsSignalConfigured() const;

    /// @brief Request highest precision available from loadcell and wait for it.
    inline void EnableHighPrecision(bool wait = true) {
        highPrecision = true;
        if (wait)
            WaitForNextSample();
    }
    inline void DisableHighPrecision() { highPrecision = false; }
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
    };

    FailureOnLoadAboveEnforcer CreateLoadAboveErrEnforcer(bool enable = true, float grams = 3000);

private:
#if PRINTER_IS_PRUSA_XL
    // Tweaked butter(2, [0.07 0.11])
    struct ZFilterParams {
        static constexpr float gain = 276.1148366795870;
        static constexpr std::array<const float, 5> a = { { 1, -3.678167822936356, 5.211060348827695, -3.364842682922483, 0.837181651256023 } };
        static constexpr size_t settling_time = 100;
    };
#else  /*PRINTER*/
    // Original
    struct ZFilterParams {
        static constexpr float gain = 5.724846511e+01f;
        static constexpr std::array<const float, 5> a = { { 1, -3.6132919084, 4.9481816585, -3.0510427201, 0.7164075250 } };
        static constexpr size_t settling_time = 110;
    };
#endif /*PRINTER*/

    // Tweaked butter(2, [0.005 0.08])
    // Consider increasing the low threshold in case the offset takes to long to remove
    struct XYFilterParam {
        static constexpr float gain = 1 / 1.185768264324116e-02;
        static constexpr std::array<const float, 5> a = { { 1, -3.661929127367906, 5.041628953899732, -3.096320393316955, 0.716633873504158 } };
        static constexpr size_t settling_time = 120;
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
        }

        inline float filter(float input) {
            static_assert(NZEROS == 4, "This code works only for NZEROS == 4");
            static_assert(NPOLES == 4, "This code works only for NPOLES == 4");
            static_assert(A[0] == 1, "This code works only A[0] == 1");

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

    private:
        float xv[NZEROS + 1];
        float yv[NPOLES + 1];
    };

    float scale;
    float thresholdStatic;
    float thresholdContinuous;
    float hysteresis;
    float failsOnLoadAbove;
    float failsOnLoadBelow;
    osThreadId threadId;
    int32_t signal;
    int32_t loadcellRaw;
    bool endstop;
    std::atomic<bool> xy_endstop;
    bool isSignalEventConfigured;
    bool highPrecision;

    // When tare is requested, this will store number of samples and countdown to zero
    std::atomic<uint32_t> tareCount;
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
    std::atomic<uint32_t> last_sample_time;

    int32_t WaitForNextSample();
};

extern Loadcell loadcell;

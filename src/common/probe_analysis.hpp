#pragma once
#include <array>
#include <cstdint>
#include <algorithm>
#include <tuple>
#include <optional>
#include <numeric>
#include <limits>
#include <cmath>
#include <atomic>
#include <expected>
#if __has_include("metric.h") && !defined(PROBE_ANALYSIS_DISABLE_METRICS) && !defined(UNITTESTS)
    #define PROBE_ANALYSIS_WITH_METRICS
    #include "metric.h"
#endif
#include <circle_buffer.hpp>
#include <option/has_indx.h>

#ifndef M_PI
    // #error dead code found by automatic analyses (see BFW-5461)
    #define M_PI 3.14159265358979323846f
#endif

namespace buddy {

class ProbeAnalysisBase {

public:
    /// Represents a point in time for the analysis
    ///
    /// Zero is at the beginning of the window. Xth sample has time of samplingInterval * X.
    using Time = float;

    struct AnalysisResult {
        /// Coordinate of the bed
        float z_coordinate;
    };

    struct AnalysisError {
        /// Explanation for the classification result.
        const char *description;

        /// Value of the failed parameter
        float arg = 0;
    };

    using Result = std::expected<AnalysisResult, AnalysisError>;

    /// Entry of the moving window used for analysis.
    struct Record {
        /// Extruder's Z coordinate [mm]
        float z;

        /// Load measured [grams]
        float load;
    };

    /// Represents a single sample
    using Sample = CircleBufferBaseT<Record>::iterator;

    /// Line (y = ax + b) used to represent parts of the probe
    struct Line {
        float a;
        float b;

        Line(float a, float b)
            : a(a)
            , b(b) {}

        constexpr Line()
            : a(std::numeric_limits<float>::quiet_NaN())
            , b(std::numeric_limits<float>::quiet_NaN()) {}

        constexpr static Line Invalid() {
            return Line();
        }

        inline bool IsValid() const {
            return !std::isnan(a) && !std::isnan(b);
        }

        inline float GetY(Time time) const {
            return a * static_cast<float>(time) + b;
        }

        inline Time GetTime(float y) const {
            return static_cast<Time>((y - b) / a);
        }

        /// Get the X (time) where this line intersects with the other
        ///
        /// Returns NaN if the two lines are parallel or the lines are not valid.
        Time FindIntersection(Line other) const;

        /// Return the angle in degrees between this line and the other
        float CalculateAngle(Line other, bool normalize = true) const;
    };

    struct SegmentedR2s {
        float compressionStart = std::numeric_limits<Time>::quiet_NaN();
        float compressionEnd = std::numeric_limits<Time>::quiet_NaN();
        float decompressionStart = std::numeric_limits<Time>::quiet_NaN();
        float decompressionEnd = std::numeric_limits<Time>::quiet_NaN();
    };

    struct Features {
        Sample analysisStart;
        Line fallLine;
        Sample fallStart;
        Sample fallEnd;
        Line haltLine;
        Sample riseStart;
        Sample riseEnd;
        Line riseLine;
        Sample analysisEnd;

        Line beforeCompressionLine;
        Time compressionStartTime = std::numeric_limits<Time>::quiet_NaN();
        Line compressionLine;
        Time compressionEndTime = std::numeric_limits<Time>::quiet_NaN();
        Line compressedLine;
        Time decompressionStartTime = std::numeric_limits<Time>::quiet_NaN();
        Line decompressionLine;
        Time decompressionEndTime = std::numeric_limits<Time>::quiet_NaN();
        Line afterDecompressionLine;

        float loadMeanBeforeCompression = std::numeric_limits<Time>::quiet_NaN();
        float loadMeanAfterDecompression = std::numeric_limits<Time>::quiet_NaN();

        float loadAngleCompressionStart = std::numeric_limits<Time>::quiet_NaN();
        float loadAngleCompressionEnd = std::numeric_limits<Time>::quiet_NaN();
        float loadAngleDecompressionStart = std::numeric_limits<Time>::quiet_NaN();
        float loadAngleDecompressionEnd = std::numeric_limits<Time>::quiet_NaN();

        SegmentedR2s r2_20ms;
        SegmentedR2s r2_30ms;
        SegmentedR2s r2_50ms;
        SegmentedR2s r2_60ms;
    };

    struct VarianceInfo {
        float total;
        float unexplained;

        VarianceInfo()
            : total(0)
            , unexplained(0) {}

        VarianceInfo(float total, float unexplained)
            : total(total)
            , unexplained(unexplained) {}
    };

    struct SamplesRange {
        Sample first;
        Sample last;

        SamplesRange(Sample first, Sample last)
            : first(first)
            , last(last) {}

        inline size_t Size() const {
            return last - first + 1;
        }

        inline Sample begin() const { return first; }

        inline Sample end() const { return last + 1; }

        inline SamplesRange beginning(size_t size) const {
            return SamplesRange(first, std::min(first + size - 1, last));
        }

        inline SamplesRange ending(size_t size) const {
            return SamplesRange(std::max(last - size + 1, last), last);
        }
    };

public:
    /// Shall be called before Analyse() with the real measured sampling interval
    void SetSamplingIntervalMs(float interval);

public:
    /// Append current Z coordinate and load to the window for later analysis
    void StoreSample(uint32_t time_us, float currentZ, float currentLoad);

    /// Run the analysis and return its result
    Result Analyse(bool check_angle_after);

    /// Clear the analysis window
    void Reset() {
        window.clear();
        prevSampleValid = false;
        gapWindowRemaining = 0;
    }

public:
    const float loadDelay;

    /// How many samples to ignore at the beginning/end of a sample sequence (related to some features)
    const int skipBorderSamples;

    /// Initial sampling frequency, will be changed by SetSamplingIntervalMs() from a real data
    static constexpr float initialFrequency = HAS_INDX() ? 366.0f : 320.0f;

    /// Time interval in seconds specifying the samples ignored when calculating the intersection
    /// between before and compression line features.
    static constexpr float analysisCompressionGap = 0.010f;

    /// Time interval in seconds specifying the samples ignored when calculating the intersection
    /// between decompression and after-decompression line features.
    static constexpr float analysisDecompressionGap = 0.030f;

    /// Expected time interval in seconds specifying the time required for the nozzle lift off the
    /// bed starting from the trigger endpoint
    static constexpr float analysisExpectedRaiseTime = 0.100f;

    /// Time interval in seconds specifying the length of samples used for the before and after
    /// compression lines.
    static constexpr float analysisBaselineTime = 0.300f;

    /// Time interval in seconds specifying the subset of samples before haltStart that should be used for the analysis.
    static constexpr float analysisLookback = analysisBaselineTime + analysisCompressionGap;

    /// Time interval in seconds specifying the subset of samples after haltEnd that should be used
    /// for the analysis. Lookahead should have enough samples to match the lenght of the
    /// pre-compression line after accounting for raise time, plus some margin.
    static constexpr float analysisLookahead = analysisBaselineTime + analysisDecompressionGap + analysisExpectedRaiseTime;

    /// A gap larger than this many sampling intervals corrupts the index-based
    /// time axis, so such a probe is failed and retried. Sized to tolerate the
    /// dwarf deglitch filter's worst-case reverting-glitch gap (loadcell_deglitcher.hpp,
    /// MAX_SKIPPED) plus jitter, while still catching real transport holes
    /// (e.g. puppy reset), which lose far more samples.
    static constexpr float maxSampleGapFactor = 5.0f;

    /// Currently recorded samples (moving window).
    CircleBufferBaseT<Record> &window;

    /// Last sample's timestamp to be used for metrics
    uint32_t lastSampleTimestamp;

public:
    /// Return time of the given sample
    float TimeOfSample(Sample sample) const {
        return static_cast<float>(sample - window.begin()) * samplingInterval;
    }

    enum class SearchDirection {
        Backward,
        Forward,
        Both,
    };

    /// Find the closest sample for given time
    Sample ClosestSample(Time time, SearchDirection direction);

    /// Linear regression for a given sample range
    ///
    /// The y values to be used by the regression are to be provided
    /// by the getY callable (signature: float getY(Sample))
    /// Returns invalid line if solution does not exists.
    template <typename F>
    Line LinearRegression(SamplesRange samples, F getY) const {
        float xSum, ySum, xySum, x2Sum;
        xSum = ySum = xySum = x2Sum = 0;

        for (auto it = samples.first; it <= samples.last; ++it) {
            float x = static_cast<float>(TimeOfSample(it));
            float y = getY(it);
            xSum += x;
            ySum += y;
            xySum += x * y;
            x2Sum += x * x;
        }

        float len = static_cast<float>(samples.Size());
        float aNum = len * xySum - xSum * ySum;
        float aDenom = len * x2Sum - (xSum * xSum);
        float bNum = x2Sum * ySum - xSum * xySum;
        float bDenom = len * x2Sum - (xSum * xSum);

        if (aDenom == 0 || bDenom == 0) {
            return Line::Invalid();
        }

        float a = aNum / aDenom;
        float b = bNum / bDenom;
        return Line(a, b);
    }

    /// Calculate error when the load of given sample range would be represented by two specific lines
    ///
    /// Those lines are the result of linear regression over [start, split) and [split + gapSamples, end].
    /// Please note, that the `split` sample is included in the second line/regression and excluded from the first.
    ///
    /// Returns (NaN, Line::Invalid, Line::Invalid) on error.
    std::tuple<float, Line, Line> CalculateErrorWhenLoadRepresentedAsLines(SamplesRange samples, Sample split, size_t gapSamples);

    /// Find the best two-line representation of load for given sample range, ignoring the
    /// intersection point (gapSamples).
    ///
    /// Returns the split sample as accepted by CalculateErrorWhenLoadRepresentedAsLines
    /// and the two lines.
    /// Returns (window.end(), Line::Invalid, Line::Invalid) on error.
    std::tuple<Sample, Line, Line> FindBestTwoLinesApproximation(SamplesRange samples, size_t gapSamples);

    /// Compensate for the fact that loadcell data are delayed in respect to Z axis coordinates.
    std::expected<void, AnalysisError> CompensateForSystemDelay();

    /// Calculate fallEnd and riseStart features
    void CalculateHaltSpan(Features &features);

    /// Calculates the analysisStart and analysisEnd features.
    ///
    /// Fails if there is not enough data in the window.
    std::expected<void, AnalysisError> CalculateAnalysisRange(Features &features);

    bool CalculateLoadLineApproximationFeatures(Features &features);

    bool CalculateZLineApproximationFeatures(Features &features);

    /// Check line-approximation based features' sanity for early exit
    bool CheckLineSanity(Features &features);

    /// Calculate load means
    void CalculateLoadMeans(Features &features);

    /// Calculates all load angles in the time ranges we are interested in
    void CalculateLoadAngles(Features &features) const;

    VarianceInfo CalculateVariance(SamplesRange samples, Line regression);

    template <typename Iterator>
    float CalculateR2FromSegmentedVariance(Iterator begin, Iterator end) {
        // FIXME: !!! accumulate functions are wrong, should return acc + r.load. Fixing this causes classify to fail however.
        float totalVariance = std::accumulate(begin, end, 0.0f, []([[maybe_unused]] float acc, VarianceInfo const &r) { return r.total; });
        float unexplainedVariance = std::accumulate(begin, end, 0.0f, []([[maybe_unused]] float acc, VarianceInfo const &r) { return r.unexplained; });
        if (totalVariance == 0) {
            return -std::numeric_limits<float>::infinity();
        }
        return 1 - (unexplainedVariance / totalVariance);
    }

    SegmentedR2s CalculateSegmentedR2s(Features &features, float segment);

    float InterpolateFinalZCoordinate(Features &features);

    /**
     * Classify the probe based on its features.
     *
     * @note This function is auto-generated based on trained models.
     *       Hand-made changes will be lost (so don't make them).
     * @returns 1 if the probe is classified as good, 0 otherwise
     */
    int Classify(Features &features);

    /**
     * Identify any feature being out of its expected range.
     *
     * @note This function is auto-generated based on trained models.
     *       Hand-made changes will be lost (so don't make them).
     */
    bool HasOutOfRangeFeature(Features &features, const char **feature, float *value, bool check_angle_after) const;

protected:
    ProbeAnalysisBase(CircleBufferBaseT<Record> &window, float loadDelay, int skipBorderSamples)
        : loadDelay(loadDelay)
        , skipBorderSamples(skipBorderSamples)
        , window(window) {
    }

protected:
    /// Time interval in seconds between consecutive samples.
    float samplingInterval = 1.0f / initialFrequency;

    /// True if Analyse() is being process (and no samples should be processed)
    std::atomic<bool> analysisInProgress = false;

    /// Timestamp [us] of the previously stored sample (for gap detection)
    uint32_t prevSampleTime = 0;

    /// Whether prevSampleTime holds a real sample yet
    bool prevSampleValid = false;

    /// >0 while a large inter-sample gap is still within the window
    size_t gapWindowRemaining = 0;

    /// Log features into metrics
    void log_features_metrics(const Features &features, std::optional<float> detected_z) const;
};

/**
 * Probe analysis & classification.
 *
 * @tparam W length of the window used for analysis
 */
template <std::size_t W, int LoadDelay = 20, int SkipBorderSamples = 3>
class ProbeAnalysis : public ProbeAnalysisBase {

public:
    ProbeAnalysis()
        : ProbeAnalysisBase(window_instance, LoadDelay / 1000.0f, SkipBorderSamples) {
    }

private:
    /// Currently recorded samples (moving window).
    CircleBuffer<Record, W + 1> window_instance;
};

} // namespace buddy

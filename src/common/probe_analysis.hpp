#pragma once
#include <array>
#include <cstdint>
#include <algorithm>
#include <tuple>
#include <functional>
#include <numeric>
#include <limits>
#include <cassert>
#include <cmath>
#include <atomic>
#if __has_include("metric.h") && !defined(PROBE_ANALYSIS_DISABLE_METRICS) && !defined(UNITTESTS)
    #define PROBE_ANALYSIS_WITH_METRICS
    #include "metric.h"
#endif
#include <circle_buffer.hpp>

#ifndef M_PI
    #define M_PI 3.14159265358979323846f
#endif

namespace buddy {

class ProbeAnalysisBase {

public:
    /// Represents a point in time for the analysis
    ///
    /// Zero is at the beginning of the window. Xth sample has time of samplingInterval * X.
    using Time = float;

    /// Analysis result
    struct Result {
        /// True if the probe is considered precise/good and zCoordinate is set.
        bool isGood;

        /// Explanation for the classification result. Might be nullptr.
        const char *description;

        /// Coordinate of the bed (if isGood == true)
        float zCoordinate;

    public:
        /// Create a result where the probe is classified as good
        static inline Result Good(float zCoordinate) {
            return Result { true, nullptr, zCoordinate };
        }

        /// Create a result where the probe is classified as bad
        static inline Result Bad(const char *description) {
            return Result { false, description, std::numeric_limits<float>::quiet_NaN() };
        }
    };

    /// Entry of the moving window used for analysis.
    struct Record {
        uint32_t timestamp;

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
        Sample fallEnd;
        Line haltLine;
        Sample riseStart;
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
    void StoreSample(float currentZ, float currentLoad);

    /// Run the analysis and return its result
    Result Analyse();

    /// Clear the analysis window
    void Reset() {
        window.clear();
    }

public:
    const float loadDelay;

    /// How many samples to ignore at the beginning/end of a sample sequence (related to some features)
    const int skipBorderSamples;

    /// Time interval in seconds specifying the subset of samples before haltStart that should be used for the analysis.
    static constexpr float analysisLookback = 0.150;

    /// Time interval in seconds specifying the subset of samples after haltEnd that should be used for the analysis.
    static constexpr float analysisLookahead = 0.300;

    /// Currently recorded samples (moving window).
    CircleBufferBaseT<Record> &window;

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
    /// Those lines are the result of linear regression over [start, split) and [split, end].
    /// Please note, that the `split` sample is included in the second line/regression and excluded from the first.
    ///
    /// Returns (NaN, Line::Invalid, Line::Invalid) on error.
    std::tuple<float, Line, Line> CalculateErrorWhenLoadRepresentedAsLines(SamplesRange samples, Sample split);

    /// Find the best two-line representation of load for given sample range
    ///
    /// Returns the split sample as accepted by CalculateErrorWhenLoadRepresentedAsLines
    /// and the two lines.
    /// Returns (window.end(), Line::Invalid, Line::Invalid) on error.
    std::tuple<Sample, Line, Line> FindBestTwoLinesApproximation(SamplesRange samples);

    /// Compensate for the fact that loadcell data are delayed in respect to Z axis coordinates.
    bool CompensateForSystemDelay();

    /// Calculate fallEnd and riseStart features
    void CalculateHaltSpan(Features &features);

    /// Calculates the analysisStart and analysisEnd features.
    ///
    /// Returns true if we have enough data in the window. False otherwise.
    bool CalculateAnalysisRange(Features &features);

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
    bool HasOutOfRangeFeature(Features &features, const char **feature, float *value) const;

protected:
    ProbeAnalysisBase(CircleBufferBaseT<Record> &window, float loadDelay, int skipBorderSamples)
        : loadDelay(loadDelay)
        , skipBorderSamples(skipBorderSamples)
        , window(window) {
    }

protected:
    /// Time interval in seconds between consecutive samples.
    float samplingInterval = 1.0f / 320.0f;

    /// True if Analyse() is being process (and no samples should be processed)
    std::atomic<bool> analysisInProgress = false;
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

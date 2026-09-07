#include "probe_analysis.hpp"
#include <raii/scope_guard.hpp>
#include "metric.h"
#include "timing.h"
#include <printers.h>
#ifdef PROBE_ANALYSIS_WITH_METRICS
    #include <Marlin.h>
#endif

using namespace buddy;

void ProbeAnalysisBase::SetSamplingIntervalMs(float interval) {
    samplingInterval = interval / 1000;
}

void ProbeAnalysisBase::StoreSample(uint32_t time_us, float currentZ, float currentLoad) {
    if (analysisInProgress) {
        return;
    }
    window.push_back({ currentZ, currentLoad });

    // Detect dropped samples: the index-based time axis assumes uniform sampling,
    // so a large gap (e.g. a puppy reset) would skew the analysis. The flag stays
    // raised while the affected samples remain in the window.
    if (prevSampleValid) {
        const uint32_t delta = time_us - prevSampleTime;
        if (delta > static_cast<uint32_t>(maxSampleGapFactor * samplingInterval * 1e6f)) {
            gapWindowRemaining = window.capacity();
        } else if (gapWindowRemaining > 0) {
            --gapWindowRemaining;
        }
    }
    prevSampleTime = time_us;
    prevSampleValid = true;

#if !defined(UNITTESTS)
    lastSampleTimestamp = ticks_us();
#endif
}

ProbeAnalysisBase::Result ProbeAnalysisBase::Analyse(bool check_angle_after) {
    analysisInProgress = true;
    ScopeGuard _sg = [this] {
        analysisInProgress = false;
    };

    // Reject if a large gap is still within the window (it corrupts the index-based time axis).
    if (gapWindowRemaining > 0) {
        return std::unexpected(AnalysisError { "stream-gap" });
    }

    // First of all, shift Z coordinates in order to compansate for the system's delay.
    if (auto r = CompensateForSystemDelay(); !r) {
        return std::unexpected(r.error());
    }

    for (const auto &sample : window) {
        if (std::isnan(sample.load)) {
            return std::unexpected(AnalysisError { "load-nan" });
        }
        if (std::isnan(sample.z)) {
            return std::unexpected(AnalysisError { "z-nan" });
        }
    }

    // Next, calculate all the features
    Features features;
    CalculateHaltSpan(features);
    if (auto r = CalculateAnalysisRange(features); !r) {
        return std::unexpected(r.error());
    }

#ifdef PROBE_ANALYSIS_WITH_METRICS
    auto relative_position = [&window = this->window](Sample sample) {
        return static_cast<double>(sample - window.begin()) / static_cast<double>(window.capacity());
    };
    METRIC_DEF(probe_window_metric, "probe_window", METRIC_VALUE_CUSTOM, 0, METRIC_DISABLED);
    metric_record_custom(&probe_window_metric, " as=%0.3f,fe=%0.3f,rs=%0.3f,ae=%0.3f",
        relative_position(features.analysisStart),
        relative_position(features.fallEnd),
        relative_position(features.riseStart),
        relative_position(features.analysisEnd));
#endif

    if (CalculateLoadLineApproximationFeatures(features) == false) {
        return std::unexpected(AnalysisError { "load-lines" });
    }
    if (CalculateZLineApproximationFeatures(features) == false) {
        return std::unexpected(AnalysisError { "z-lines" });
    }
    if (!CheckLineSanity(features)) {
        return std::unexpected(AnalysisError { "sanity-check" });
    }
    CalculateLoadMeans(features);
    CalculateLoadAngles(features);
    features.r2_20ms = CalculateSegmentedR2s(features, 0.02f);
    features.r2_30ms = CalculateSegmentedR2s(features, 0.03f);
    features.r2_50ms = CalculateSegmentedR2s(features, 0.05f);
    features.r2_60ms = CalculateSegmentedR2s(features, 0.06f);

#ifdef PROBE_ANALYSIS_WITH_METRICS
    {
        // log the load line to metrics
        METRIC_DEF(probe_ll_metric, "probe_load_line", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);

        std::array<std::tuple<uint32_t, float>, 6> load_line_points;

        auto timestamp_for_time = [this](Time time) -> uint32_t {
            return static_cast<uint32_t>(TimeOfSample(ClosestSample(time, SearchDirection::Both)));
        };

        // analysis start
        {
            idle(true);
            auto time = TimeOfSample(features.analysisStart);
            auto load = features.beforeCompressionLine.GetY(time);
            load_line_points.at(0) = { timestamp_for_time(time), load };
        }

        // compression start
        {
            auto time = features.compressionStartTime;
            auto load = features.compressionLine.GetY(time);
            load_line_points.at(1) = { timestamp_for_time(time), load };
        }

        // compression end
        {
            auto time = features.compressionEndTime;
            auto load = features.compressedLine.GetY(time);
            load_line_points.at(2) = { timestamp_for_time(time), load };
        }

        // decompression start
        {
            auto time = features.decompressionStartTime;
            auto load = features.compressedLine.GetY(time);
            load_line_points.at(3) = { timestamp_for_time(time), load };
        }

        // decompression end
        {
            auto time = features.decompressionEndTime;
            auto load = features.decompressionLine.GetY(time);
            load_line_points.at(4) = { timestamp_for_time(time), load };
        }

        // analysis end
        {
            auto time = TimeOfSample(features.analysisEnd);
            auto load = features.afterDecompressionLine.GetY(time);
            load_line_points.at(5) = { timestamp_for_time(time), load };
        }

        for (auto [timestamp, load] : load_line_points) {
            idle(true);
            metric_record_custom_at_time(&probe_ll_metric, timestamp, " v=%0.3f", (double)load);
        }
    }
#endif

    // Check all features are within expected range
    {
        const char *feature = "feature-out-of-range";
        float value;
        if (HasOutOfRangeFeature(features, &feature, &value, check_angle_after)) {
            return std::unexpected(AnalysisError { .description = feature, .arg = value });
        }
    }

    // Last, use the features in our classification model and guess the probe's precision
    bool isGood = Classify(features);
    float zCoordinate = InterpolateFinalZCoordinate(features);
    if (isGood) {
        log_features_metrics(features, zCoordinate);
        return AnalysisResult { .z_coordinate = zCoordinate };
    } else {
        log_features_metrics(features, std::nullopt);
        return std::unexpected(AnalysisError { .description = "low-precision" });
    }
}

ProbeAnalysisBase::Time ProbeAnalysisBase::Line::FindIntersection(Line other) const {
    if (!IsValid() || !other.IsValid()) {
        return std::numeric_limits<Time>::quiet_NaN();
    }
    auto num = -this->b + other.b;
    auto denom = this->a - other.a;
    if (denom == 0) {
        return std::numeric_limits<float>::quiet_NaN();
    }
    return static_cast<Time>(num / denom);
}

float ProbeAnalysisBase::Line::CalculateAngle(Line other, bool normalize) const {
    if (!IsValid() || !other.IsValid()) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    constexpr float normalizationFactor = 250;
    constexpr float toDegrees = 180.f / (float)M_PI;
    float thisA = this->a / normalizationFactor;
    float otherA = other.a / normalizationFactor;
    float angle = atanf((otherA - thisA) / (1 + thisA * otherA)) * toDegrees;
    if (normalize) {
        return angle < 0 ? 180 + angle : angle;
    } else {
        return angle;
    }
}

ProbeAnalysisBase::Sample ProbeAnalysisBase::ClosestSample(Time time, SearchDirection direction) {
    size_t index;
    switch (direction) {

    case SearchDirection::Backward:
        index = static_cast<size_t>(std::floor(static_cast<float>(time) / samplingInterval));
        break;

    case SearchDirection::Forward:
        index = static_cast<size_t>(std::ceil(static_cast<float>(time) / samplingInterval));
        break;

    case SearchDirection::Both:
        index = static_cast<size_t>(std::round(static_cast<float>(time) / samplingInterval));
        break;

    default:
        std::abort();
    }
    Sample sample = window.begin() + index;
    return std::max(std::min(sample, window.end() - 1), window.begin());
}

std::tuple<float, ProbeAnalysisBase::Line, ProbeAnalysisBase::Line> ProbeAnalysisBase::CalculateErrorWhenLoadRepresentedAsLines(SamplesRange samples, Sample split, size_t gap) {
    // linear regression
    auto getLoad = [](Sample s) { return s->load; };
    Line leftLine = LinearRegression(SamplesRange(samples.first, split - 1), getLoad);
    Line rightLine = LinearRegression(SamplesRange(split + gap, samples.last), getLoad);

    // early exit if something went wrong
    if (!leftLine.IsValid() || !rightLine.IsValid()) {
        return std::make_tuple(std::numeric_limits<float>::quiet_NaN(), Line::Invalid(), Line::Invalid());
    }

    // sum the error
    float error = 0;
    for (auto it = samples.first; it <= samples.last; ++it) {
        float diff;
        if (it < split) {
            diff = leftLine.GetY(TimeOfSample(it)) - it->load;
        } else {
            diff = rightLine.GetY(TimeOfSample(it)) - it->load;
        }
        error += diff * diff;
    }
    return std::make_tuple(error, leftLine, rightLine);
}

std::tuple<ProbeAnalysisBase::Sample, ProbeAnalysisBase::Line, ProbeAnalysisBase::Line> ProbeAnalysisBase::FindBestTwoLinesApproximation(SamplesRange samples, size_t sampleGap) {
    // Ensure we have at least 2 samples per segment + gap
    if (samples.Size() < (3 + sampleGap)) {
        return std::make_tuple(window.end(), Line::Invalid(), Line::Invalid());
    }

    float bestError = std::numeric_limits<float>::max();
    Sample bestSplit = window.end();
    Line leftLine, rightLine;

    for (auto split = samples.first + 1; split < (samples.last - sampleGap); ++split) {
        auto result = CalculateErrorWhenLoadRepresentedAsLines(samples, split, sampleGap);
        float error = std::get<0>(result);
        if (!std::isnan(error) && error < bestError) {
            bestSplit = split;
            std::tie(bestError, leftLine, rightLine) = result;
        }
    }

    return std::make_tuple(bestSplit, leftLine, rightLine);
}

std::expected<void, ProbeAnalysisBase::AnalysisError> ProbeAnalysisBase::CompensateForSystemDelay() {
    // Shift Z samples right (to the future)
    int samplesToShift = static_cast<int>(loadDelay / samplingInterval);

    if (window.size() <= static_cast<size_t>(samplesToShift) + 2) {
        return std::unexpected(AnalysisError { .description = "small-window", .arg = (float)window.size() });
    }

    auto it = window.rbegin();
    for (; it < window.rend() - samplesToShift; ++it) {
        it->z = (it + samplesToShift)->z;
    }
    // Approximate the missing Z samples based on the closest two Z samples we already have
    auto diff = (it - 1)->z - (it - 2)->z;
    for (; it < window.rend(); ++it) {
        it->z = (it - 1)->z + diff;
    }

    return {};
}

void ProbeAnalysisBase::CalculateHaltSpan(Features &features) {
    // find the start of the fall line
    Sample fallStart = window.begin();
    for (auto it = fallStart + 1; it < window.end(); ++it) {
        if (fallStart->z != it->z) {
            break;
        }
        fallStart = it;
    }

    // find the end of the raise line
    Sample riseEnd = window.end() - 1;
    for (auto it = riseEnd; it > fallStart; --it) {
        if (riseEnd->z != it->z) {
            break;
        }
        riseEnd = it;
    }

    // iterate backwards and find range of the first global minimum
    Sample fallEnd = riseEnd;
    Sample riseStart = fallStart;
    bool extendingHalt = true;
    for (auto it = riseEnd; it > fallStart; --it) {
        if (it->z < fallEnd->z) {
            fallEnd = riseStart = it;
            extendingHalt = true;
        } else if (extendingHalt && it->z == fallEnd->z) {
            fallEnd = it;
        } else {
            extendingHalt = false;
        }
    }

    features.fallStart = fallStart;
    features.fallEnd = fallEnd;
    features.riseStart = riseStart;
    features.riseEnd = riseEnd;
}

std::expected<void, ProbeAnalysisBase::AnalysisError> ProbeAnalysisBase::CalculateAnalysisRange(Features &features) {
    int lookbackSamples = static_cast<int>(analysisLookback / samplingInterval);
    int lookaheadSamples = static_cast<int>(analysisLookahead / samplingInterval);

    if (auto sz = features.fallEnd - window.begin(); sz < lookbackSamples) {
        return std::unexpected(AnalysisError { .description = "small-lookback", .arg = (float)sz });
    }
    if (auto sz = window.end() - features.riseStart; sz < lookaheadSamples) {
        return std::unexpected(AnalysisError { .description = "small-lookahead", .arg = (float)sz });
    }

    features.analysisStart = features.fallEnd - lookbackSamples;
    features.analysisEnd = features.riseStart + lookaheadSamples;
    return {};
}

bool ProbeAnalysisBase::CalculateLoadLineApproximationFeatures(Features &features) {
    auto getLoad = [](Sample s) { return s->load; };
    size_t compressionGapSamples = static_cast<size_t>(analysisCompressionGap / samplingInterval);
    size_t decompressionGapSamples = static_cast<size_t>(analysisDecompressionGap / samplingInterval);

    std::tie(std::ignore, features.beforeCompressionLine, features.compressionLine) = FindBestTwoLinesApproximation(SamplesRange(features.analysisStart, features.fallEnd - skipBorderSamples), compressionGapSamples);
    features.compressedLine = LinearRegression(SamplesRange(features.fallEnd + skipBorderSamples, features.riseStart - skipBorderSamples), getLoad);
    std::tie(std::ignore, features.decompressionLine, features.afterDecompressionLine) = FindBestTwoLinesApproximation(SamplesRange(features.riseStart + skipBorderSamples, features.analysisEnd), decompressionGapSamples);

    features.compressionStartTime = features.beforeCompressionLine.FindIntersection(features.compressionLine);
    features.compressionEndTime = features.compressionLine.FindIntersection(features.compressedLine);
    features.decompressionStartTime = features.compressedLine.FindIntersection(features.decompressionLine);
    features.decompressionEndTime = features.decompressionLine.FindIntersection(features.afterDecompressionLine);

    return !std::isnan(features.compressionStartTime) && !std::isnan(features.compressionEndTime) && !std::isnan(features.decompressionStartTime) && !std::isnan(features.decompressionEndTime);
}

bool ProbeAnalysisBase::CalculateZLineApproximationFeatures(Features &features) {
    auto getZ = [](Sample s) { return s->z; };

    features.fallLine = LinearRegression(SamplesRange(features.fallStart + skipBorderSamples, features.fallEnd - skipBorderSamples), getZ);
    features.haltLine = LinearRegression(SamplesRange(features.fallEnd + skipBorderSamples, features.riseStart - skipBorderSamples), getZ);
    features.riseLine = LinearRegression(SamplesRange(features.riseStart + skipBorderSamples, features.riseEnd - skipBorderSamples), getZ);

    return features.fallLine.IsValid() && features.haltLine.IsValid() && features.riseLine.IsValid();
}

bool ProbeAnalysisBase::CheckLineSanity(Features &features) {
    const auto analysis_start_time = TimeOfSample(features.analysisStart);
    const auto analysis_end_time = TimeOfSample(features.analysisEnd);

    // times are in expected order
    bool timesInOrder = true;
    timesInOrder &= analysis_start_time < features.compressionStartTime;
    timesInOrder &= features.compressionStartTime < features.compressionEndTime;
    timesInOrder &= features.compressionEndTime < features.decompressionStartTime;
    timesInOrder &= features.decompressionStartTime < features.decompressionEndTime;
    timesInOrder &= features.decompressionEndTime < analysis_end_time;
    return timesInOrder;
}

void ProbeAnalysisBase::CalculateLoadMeans(Features &features) {
    auto calcLoadMean = [](SamplesRange samples) {
        return std::accumulate(samples.begin(), samples.end(), 0.0f, [](float acc, Record const &record) {
            return acc + record.load;
        }) / static_cast<float>(samples.Size());
    };

    SamplesRange beforeCompressionSamples(features.analysisStart, ClosestSample(features.compressionStartTime - analysisCompressionGap, SearchDirection::Backward));
    features.loadMeanBeforeCompression = calcLoadMean(beforeCompressionSamples);

    SamplesRange afterDecompressionSamples(ClosestSample(features.decompressionEndTime + analysisDecompressionGap, SearchDirection::Forward), features.analysisEnd);
    features.loadMeanAfterDecompression = calcLoadMean(afterDecompressionSamples);
}

void ProbeAnalysisBase::CalculateLoadAngles(Features &features) const {
    features.loadAngleCompressionStart = features.beforeCompressionLine.CalculateAngle(features.compressionLine);
    features.loadAngleCompressionEnd = features.compressedLine.CalculateAngle(features.compressionLine);
    features.loadAngleDecompressionStart = features.decompressionLine.CalculateAngle(features.compressedLine);
    features.loadAngleDecompressionEnd = features.decompressionLine.CalculateAngle(features.afterDecompressionLine);
#ifdef PROBE_ANALYSIS_WITH_METRICS
    {
        auto compressedVsDecompressedAngleBefore = features.compressedLine.CalculateAngle(features.beforeCompressionLine, false);
        auto compressedvsDecompressedAngleAfter = features.compressedLine.CalculateAngle(features.afterDecompressionLine, false);
        METRIC_DEF(probe_angle_metric, "probe_angle", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
        metric_record_custom(&probe_angle_metric, " b=%0.3f,a=%0.3f", (double)compressedVsDecompressedAngleBefore, (double)compressedvsDecompressedAngleAfter);
    }
#endif
}

ProbeAnalysisBase::VarianceInfo ProbeAnalysisBase::CalculateVariance(SamplesRange samples, Line regression) {
    // FIXME: !!! accumulate function is wrong, should return acc + r.load. Fixing this causes classify to fail however.
    float mean = std::accumulate(samples.begin(), samples.end(), 0.0f, []([[maybe_unused]] float acc, Record const &r) { return r.load; }) / static_cast<float>(samples.Size());
    float totalVariance = 0, unexplainedVariance = 0;

    for (auto it = samples.first; it <= samples.last; ++it) {
        totalVariance += std::pow(it->load - mean, 2.0f);
        unexplainedVariance += std::pow(regression.GetY(TimeOfSample(it)) - it->load, 2.0f);
    }

    return VarianceInfo(totalVariance, unexplainedVariance);
}

ProbeAnalysisBase::SegmentedR2s ProbeAnalysisBase::CalculateSegmentedR2s(Features &features, float segment) {
    size_t segmentSamples = static_cast<size_t>(segment / samplingInterval);

    auto calcR2AroundTime = [&](SamplesRange leftSamples, Line leftLine,
                                [[maybe_unused]] Time middleTime,
                                SamplesRange rightSamples, Line rightLine) {
        std::array<VarianceInfo, 2> variance;
        variance[0] = CalculateVariance(leftSamples.ending(segmentSamples), leftLine);
        variance[1] = CalculateVariance(rightSamples.beginning(segmentSamples), rightLine);
        return CalculateR2FromSegmentedVariance(variance.begin(), variance.end());
    };

    Sample compressionStart = ClosestSample(features.compressionStartTime, SearchDirection::Backward);
    Sample compressionEnd = ClosestSample(features.compressionEndTime, SearchDirection::Backward);
    Sample decompressionStart = ClosestSample(features.decompressionStartTime, SearchDirection::Backward);
    Sample decompressionEnd = ClosestSample(features.decompressionEndTime, SearchDirection::Backward);

    SegmentedR2s r2s;
    r2s.compressionStart = calcR2AroundTime(
        SamplesRange(features.analysisStart, compressionStart), features.beforeCompressionLine,
        features.compressionStartTime,
        SamplesRange(compressionStart, compressionEnd), features.compressionLine);

    r2s.compressionEnd = calcR2AroundTime(
        SamplesRange(compressionStart, compressionEnd), features.compressionLine,
        features.compressionEndTime,
        SamplesRange(compressionEnd, decompressionStart), features.compressedLine);

    r2s.decompressionStart = calcR2AroundTime(
        SamplesRange(compressionEnd, decompressionStart), features.compressedLine,
        features.decompressionStartTime,
        SamplesRange(decompressionStart, decompressionEnd), features.decompressionLine);

    r2s.decompressionEnd = calcR2AroundTime(
        SamplesRange(decompressionStart, decompressionEnd), features.decompressionLine,
        features.decompressionEndTime,
        SamplesRange(decompressionEnd, features.analysisEnd), features.afterDecompressionLine);

    return r2s;
}

float ProbeAnalysisBase::InterpolateFinalZCoordinate(Features &features) {
    float zDecompressionEnd = features.riseLine.GetY(features.decompressionEndTime);
    float loadAtDecompressionEnd = features.decompressionLine.GetY(features.decompressionEndTime);
    float middleTimestamp = features.decompressionLine.GetTime(loadAtDecompressionEnd - 120 + 70);
    float zDecompressionMiddle = features.riseLine.GetY(middleTimestamp);
    return (zDecompressionEnd + zDecompressionMiddle) / 2
#if PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()
        + 0.02f
#endif
        ;
}

int ProbeAnalysisBase::Classify(Features &features) {
    if (features.loadAngleCompressionStart <= 154.68160247802734f) {
        if (features.r2_50ms.compressionEnd <= 0.6531023383140564f) {
            if (features.r2_30ms.compressionEnd <= 0.005205066641792655f) {
                if (features.riseLine.GetY(features.decompressionEndTime) <= -0.18971788883209229f) {
                    return 0; // bad
                } else { /* if features.riseLine.GetY(features.decompressionEndTime) > -0.18971788883209229f */
                    return 0; // bad
                }
            } else { /* if features.r2_30ms.compressionEnd > 0.005205066641792655f */
                if (features.compressionLine.GetY(features.compressionStartTime) <= -51.45247840881348f) {
                    return 0; // bad
                } else { /* if features.compressionLine.GetY(features.compressionStartTime) > -51.45247840881348f */
                    return 1; // good
                }
            }
        } else { /* if features.r2_50ms.compressionEnd > 0.6531023383140564f */
            if (features.loadAngleCompressionEnd <= 51.137014389038086f) {
                if (features.loadMeanBeforeCompression <= -23.7804012298584f) {
                    return 0; // bad
                } else { /* if features.loadMeanBeforeCompression > -23.7804012298584f */
                    return 1; // good
                }
            } else { /* if features.loadAngleCompressionEnd > 51.137014389038086f */
                if (features.loadAngleDecompressionStart <= 135.77788543701172f) {
                    return 1; // good
                } else { /* if features.loadAngleDecompressionStart > 135.77788543701172f */
                    return 1; // good
                }
            }
        }
    } else { /* if features.loadAngleCompressionStart > 154.68160247802734f */
        if (features.decompressionLine.GetY(features.decompressionEndTime) <= -63.84521484375f) {
            if (features.r2_50ms.compressionStart <= 0.6476757228374481f) {
                if (features.r2_60ms.decompressionEnd <= -321.4128608703613f) {
                    return 1; // good
                } else { /* if features.r2_60ms.decompressionEnd > -321.4128608703613f */
                    return 0; // bad
                }
            } else { /* if features.r2_50ms.compressionStart > 0.6476757228374481f */
                if (features.riseLine.GetY(features.decompressionEndTime) <= 0.07457397505640984f) {
                    return 1; // good
                } else { /* if features.riseLine.GetY(features.decompressionEndTime) > 0.07457397505640984f */
                    return 1; // good
                }
            }
        } else { /* if features.decompressionLine.GetY(features.decompressionEndTime) > -63.84521484375f */
            if (features.loadAngleDecompressionStart <= 152.87728118896484f) {
                if (features.loadMeanBeforeCompression <= -36.56223678588867f) {
                    return 0; // bad
                } else { /* if features.loadMeanBeforeCompression > -36.56223678588867f */
                    return 1; // good
                }
            } else { /* if features.loadAngleDecompressionStart > 152.87728118896484f */
                if (features.loadAngleDecompressionStart <= 161.35951232910156f) {
                    return 0; // bad
                } else { /* if features.loadAngleDecompressionStart > 161.35951232910156f */
                    return 0; // bad
                }
            }
        }
    }
}

bool ProbeAnalysisBase::HasOutOfRangeFeature(Features &features, const char **feature, float *value, bool check_angle_after) const {
    if (features.loadMeanBeforeCompression < -154.48058105323756f || features.loadMeanBeforeCompression > 152.44410035911991f) {
        *feature = "load_mean_before_compression";
        *value = features.loadMeanBeforeCompression;
        return true;
    }
    if (features.loadMeanAfterDecompression < -186.40204083948444f || features.loadMeanAfterDecompression > 194.03907144265904f) {
        *feature = "load_mean_after_decompression";
        *value = features.loadMeanAfterDecompression;
        return true;
    }
    if (features.compressionLine.GetY(features.compressionStartTime) < -256.92789509011595f || features.compressionLine.GetY(features.compressionStartTime) > 159.88252116779623f) {
        *feature = "load_compression_start";
        *value = features.compressionLine.GetY(features.compressionStartTime);
        return true;
    }
    if (features.decompressionLine.GetY(features.decompressionEndTime) < -6731.697754324502f || features.decompressionLine.GetY(features.decompressionEndTime) > 470.6857951091628f) {
        *feature = "load_decompression_end";
        *value = features.decompressionLine.GetY(features.decompressionEndTime);
        return true;
    }
    if (features.loadAngleCompressionStart < -52.492364077060884f || features.loadAngleCompressionStart > 233.5051448593478f) {
        *feature = "load_angle_compression_start";
        *value = features.loadAngleCompressionStart;
        return true;
    }
    if (features.loadAngleCompressionEnd < -29.99231606763046f || features.loadAngleCompressionEnd > 213.1064906506039f) {
        *feature = "load_angle_compression_end";
        *value = features.loadAngleCompressionEnd;
        return true;
    }
    if (features.loadAngleDecompressionStart < -46.98080445417618f || features.loadAngleDecompressionStart > 227.04222071853522f) {
        *feature = "load_angle_decompression_start";
        *value = features.loadAngleDecompressionStart;
        return true;
    }
    if (features.loadAngleDecompressionEnd < -93.72798028750367f || features.loadAngleDecompressionEnd > 273.7232224109812f) {
        *feature = "load_angle_decompression_end";
        *value = features.loadAngleDecompressionEnd;
        return true;
    }
    if (features.r2_20ms.compressionStart < -6535.315705859364f || features.r2_20ms.compressionStart > 133.50744526719794f) {
        *feature = "r2_compression_start_20";
        *value = features.r2_20ms.compressionStart;
        return true;
    }
    if (features.r2_20ms.compressionEnd < -1690.1392305507259f || features.r2_20ms.compressionEnd > 46.445990831041414f) {
        *feature = "r2_compression_end_20";
        *value = features.r2_20ms.compressionEnd;
        return true;
    }
    if (features.r2_30ms.decompressionStart < -14399.59142096975f || features.r2_30ms.decompressionStart > 469.83971245187513f) {
        *feature = "r2_decompression_start_30";
        *value = features.r2_30ms.decompressionStart;
        return true;
    }
    if (features.r2_30ms.decompressionEnd < -62922.69516938705f || features.r2_30ms.decompressionEnd > 2911.3834010814626f) {
        *feature = "r2_decompression_end_30";
        *value = features.r2_30ms.decompressionEnd;
        return true;
    }
    if (features.r2_50ms.compressionStart < -525.66164523715f || features.r2_50ms.compressionStart > 15.14786031499957f) {
        *feature = "r2_compression_start_50";
        *value = features.r2_50ms.compressionStart;
        return true;
    }
    if (features.r2_50ms.compressionEnd < -131.39515975957238f || features.r2_50ms.compressionEnd > 5.006255152513094f) {
        *feature = "r2_compression_end_50";
        *value = features.r2_50ms.compressionEnd;
        return true;
    }
    if (features.r2_50ms.decompressionStart < -8376.465287601053f || features.r2_50ms.decompressionStart > 284.4350844023189f) {
        *feature = "r2_decompression_start_50";
        *value = features.r2_50ms.decompressionStart;
        return true;
    }
    if (features.r2_50ms.decompressionEnd < -42356.61891250352f || features.r2_50ms.decompressionEnd > 1919.5961422774267f) {
        *feature = "r2_decompression_end_50";
        *value = features.r2_50ms.decompressionEnd;
        return true;
    }
    if (features.r2_60ms.decompressionStart < -9867.310839170745f || features.r2_60ms.decompressionStart > 257.2049327020503f) {
        *feature = "r2_decompression_start_60";
        *value = features.r2_60ms.decompressionStart;
        return true;
    }
    if (features.r2_60ms.decompressionEnd < -35308.89153040849f || features.r2_60ms.decompressionEnd > 1336.844303073224f) {
        *feature = "r2_decompression_end_60";
        *value = features.r2_60ms.decompressionEnd;
        return true;
    }
    if (check_angle_after) {
        // this is too strict for nozzle clean, where 3 consecutive good probes are required.
        auto compressedvsDecompressedAngleAfter = features.compressedLine.CalculateAngle(features.afterDecompressionLine, false);
        if (std::abs(compressedvsDecompressedAngleAfter) > 40) {
            *feature = "angle_after";
            *value = compressedvsDecompressedAngleAfter;
            return true;
        }
    }
    return false;
}

void ProbeAnalysisBase::log_features_metrics(const Features &features, std::optional<float> detected_z) const {
#if !defined(UNITTESTS)
    // There are fall-, halt- and rise line in Z and before compression-,
    // compression-, compressed-, decompression- and afterdecompression lines in
    // load.

    uint32_t window_start_ts = static_cast<uint32_t>(lastSampleTimestamp - window.size() * samplingInterval * 1e6f);

    float analysis_start = samplingInterval * std::distance(window.begin(), features.analysisStart);
    float fall_start = samplingInterval * std::distance(window.begin(), features.fallStart);
    float fall_end = samplingInterval * std::distance(window.begin(), features.fallEnd);
    float rise_start = samplingInterval * std::distance(window.begin(), features.riseStart);
    float rise_end = samplingInterval * std::distance(window.begin(), features.riseEnd);
    float analysis_end = samplingInterval * std::distance(window.begin(), features.analysisEnd);

    auto record_line = [&](metric_t &metric, const Line &line, float start_t, float end_t) {
        metric_record_float_at_time(&metric,
            window_start_ts + uint32_t(start_t * 1e6f),
            line.a * start_t + line.b);
        metric_record_float_at_time(&metric,
            window_start_ts + uint32_t(end_t * 1e6f),
            line.a * end_t + line.b);
    };

    METRIC_DEF(mbl_fall_line_m, "mbl_fl", METRIC_VALUE_FLOAT, 0, METRIC_DISABLED);
    record_line(mbl_fall_line_m, features.fallLine, fall_start, fall_end);

    METRIC_DEF(mbl_halt_line_m, "mbl_hl", METRIC_VALUE_FLOAT, 0, METRIC_DISABLED);
    record_line(mbl_halt_line_m, features.haltLine, fall_end, rise_start);

    METRIC_DEF(mbl_rise_line_m, "mbl_rl", METRIC_VALUE_FLOAT, 0, METRIC_DISABLED);
    record_line(mbl_rise_line_m, features.riseLine, rise_start, rise_end);

    METRIC_DEF(mbl_detected_z_m, "mbl_dz", METRIC_VALUE_FLOAT, 0, METRIC_DISABLED);
    if (detected_z.has_value()) {
        record_line(mbl_detected_z_m, Line(0, *detected_z), analysis_start, analysis_end);
    }

    METRIC_DEF(mbl_before_compr_line_m, "mbl_bcl", METRIC_VALUE_FLOAT, 0, METRIC_DISABLED);
    record_line(mbl_before_compr_line_m, features.beforeCompressionLine, analysis_start, features.compressionStartTime);

    METRIC_DEF(mbl_compressing_line_m, "mbl_cl", METRIC_VALUE_FLOAT, 0, METRIC_DISABLED);
    record_line(mbl_compressing_line_m, features.compressionLine, features.compressionStartTime, features.compressionEndTime);

    METRIC_DEF(mbl_compressed_line_m, "mbl_cpl", METRIC_VALUE_FLOAT, 0, METRIC_DISABLED);
    record_line(mbl_compressed_line_m, features.compressedLine, features.compressionEndTime, features.decompressionStartTime);

    METRIC_DEF(mbl_decompression_line_m, "mbl_dcl", METRIC_VALUE_FLOAT, 0, METRIC_DISABLED);
    record_line(mbl_decompression_line_m, features.decompressionLine, features.decompressionStartTime, features.decompressionEndTime);

    METRIC_DEF(mbl_after_decompr_line_m, "mbl_adl", METRIC_VALUE_FLOAT, 0, METRIC_DISABLED);
    record_line(mbl_after_decompr_line_m, features.afterDecompressionLine, features.decompressionEndTime, analysis_end);
#endif
}

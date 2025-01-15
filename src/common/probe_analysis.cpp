#include "probe_analysis.hpp"
#include <scope_guard.hpp>
#include <printers.h>

using namespace buddy;

void ProbeAnalysisBase::SetSamplingIntervalMs(float interval) {
    samplingInterval = interval / 1000;
}

void ProbeAnalysisBase::StoreSample(float currentZ, float currentLoad) {
    if (analysisInProgress) {
        return;
    }
    window.push_back({ currentZ, currentLoad });
}

ProbeAnalysisBase::Result ProbeAnalysisBase::Analyse() {
    analysisInProgress = true;
    ScopeGuard _sg = [this] {
        analysisInProgress = false;
    };

    // First of all, shift Z coordinates in order to compansate for the system's delay.
    if (!CompensateForSystemDelay()) {
        return Result::Bad("not-ready");
    }

    // Next, calculate all the features
    Features features;
    CalculateHaltSpan(features);
    if (!CalculateAnalysisRange(features)) {
        return Result::Bad("not-ready");
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
        return Result::Bad("load-lines");
    }
    if (CalculateZLineApproximationFeatures(features) == false) {
        return Result::Bad("z-lines");
    }
    if (!CheckLineSanity(features)) {
        return Result::Bad("sanity-check");
    }
    CalculateLoadMeans(features);
    CalculateLoadAngles(features);
    features.r2_20ms = CalculateSegmentedR2s(features, 0.020);
    features.r2_30ms = CalculateSegmentedR2s(features, 0.030);
    features.r2_50ms = CalculateSegmentedR2s(features, 0.050);
    features.r2_60ms = CalculateSegmentedR2s(features, 0.060);

    // Check all features are within expected range
    {
        const char *feature;
        float value;
        if (HasOutOfRangeFeature(features, &feature, &value)) {
            return Result::Bad("feature-out-of-range");
        }
    }

    // Last, use the features in our classification model and guess the probe's precision
    bool isGood = Classify(features);
    if (isGood) {
        float zCoordinate = InterpolateFinalZCoordinate(features);
        return Result::Good(zCoordinate);
    } else {
        return Result::Bad("low-precision");
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

float ProbeAnalysisBase::Line::CalculateAngle(Line other) const {
    if (!IsValid() || !other.IsValid()) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    constexpr float normalizationFactor = 250;
    constexpr float toDegrees = 180.f / (float)M_PI;
    float thisA = this->a / normalizationFactor;
    float otherA = other.a / normalizationFactor;
    float angle = atanf((otherA - thisA) / (1 + thisA * otherA)) * toDegrees;
    return angle < 0 ? 180 + angle : angle;
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

    default:
        std::abort();
    }
    Sample sample = window.begin() + index;
    return std::max(std::min(sample, window.end() - 1), window.begin());
}

std::tuple<float, ProbeAnalysisBase::Line, ProbeAnalysisBase::Line> ProbeAnalysisBase::CalculateErrorWhenLoadRepresentedAsLines(SamplesRange samples, Sample split) {
    // linear regression
    auto getLoad = [](Sample s) { return s->load; };
    Line leftLine = LinearRegression(SamplesRange(samples.first, split - 1), getLoad);
    Line rightLine = LinearRegression(SamplesRange(split, samples.last), getLoad);

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

std::tuple<ProbeAnalysisBase::Sample, ProbeAnalysisBase::Line, ProbeAnalysisBase::Line> ProbeAnalysisBase::FindBestTwoLinesApproximation(SamplesRange samples) {
    if (samples.Size() < 3) {
        return std::make_tuple(window.end(), Line::Invalid(), Line::Invalid());
    }

    float bestError = std::numeric_limits<float>::max();
    Sample bestSplit = window.end();
    Line leftLine, rightLine;

    for (auto split = samples.first + 1; split < samples.last; ++split) {
        auto result = CalculateErrorWhenLoadRepresentedAsLines(samples, split);
        float error = std::get<0>(result);
        if (!std::isnan(error) && error < bestError) {
            bestSplit = split;
            std::tie(bestError, leftLine, rightLine) = result;
        }
    }

    return std::make_tuple(bestSplit, leftLine, rightLine);
}

bool ProbeAnalysisBase::CompensateForSystemDelay() {
    // Shift Z samples right (to the future)
    int samplesToShift = loadDelay / samplingInterval;

    if (window.size() <= static_cast<size_t>(samplesToShift) + 2) {
        return false;
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

    return true;
}

void ProbeAnalysisBase::CalculateHaltSpan(Features &features) {
    Sample fallEnd = window.end() - 1;
    Sample riseStart = fallEnd;
    bool extendingHalt = true;

    const auto to_forward_iterator = [](auto rit) {
        auto r = rit.base();
        return --r;
    };

    // iterate backwards and find range of the first global minimum
    for (auto it = window.rbegin(), e = window.rend(); it != e; ++it) {
        if (it->z < fallEnd->z) {
            fallEnd = riseStart = to_forward_iterator(it);
            extendingHalt = true;
        } else if (extendingHalt && it->z == fallEnd->z) {
            fallEnd = to_forward_iterator(it);
        } else {
            extendingHalt = false;
        }
    }

    features.fallEnd = fallEnd;
    features.riseStart = riseStart;
}

bool ProbeAnalysisBase::CalculateAnalysisRange(Features &features) {
    int lookbackSamples = analysisLookback / samplingInterval;
    int lookaheadSamples = analysisLookahead / samplingInterval;

    if (features.fallEnd - window.begin() < lookbackSamples) {
        return false;
    }
    if (window.end() - features.riseStart < lookaheadSamples) {
        return false;
    }

    features.analysisStart = features.fallEnd - lookbackSamples;
    features.analysisEnd = features.riseStart + lookaheadSamples;
    return true;
}

bool ProbeAnalysisBase::CalculateLoadLineApproximationFeatures(Features &features) {
    auto getLoad = [](Sample s) { return s->load; };

    std::tie(std::ignore, features.beforeCompressionLine, features.compressionLine) = FindBestTwoLinesApproximation(SamplesRange(features.analysisStart, features.fallEnd - skipBorderSamples));
    features.compressedLine = LinearRegression(SamplesRange(features.fallEnd + skipBorderSamples, features.riseStart - skipBorderSamples), getLoad);
    std::tie(std::ignore, features.decompressionLine, features.afterDecompressionLine) = FindBestTwoLinesApproximation(SamplesRange(features.riseStart + skipBorderSamples, features.analysisEnd));

    features.compressionStartTime = features.beforeCompressionLine.FindIntersection(features.compressionLine);
    features.compressionEndTime = features.compressionLine.FindIntersection(features.compressedLine);
    features.decompressionStartTime = features.compressedLine.FindIntersection(features.decompressionLine);
    features.decompressionEndTime = features.decompressionLine.FindIntersection(features.afterDecompressionLine);

    return !std::isnan(features.compressionStartTime) && !std::isnan(features.compressionEndTime) && !std::isnan(features.decompressionStartTime) && !std::isnan(features.decompressionEndTime);
}

bool ProbeAnalysisBase::CalculateZLineApproximationFeatures(Features &features) {
    auto getZ = [](Sample s) { return s->z; };

    features.fallLine = LinearRegression(SamplesRange(features.analysisStart, features.fallEnd - skipBorderSamples), getZ);
    features.haltLine = LinearRegression(SamplesRange(features.fallEnd + skipBorderSamples, features.riseStart - skipBorderSamples), getZ);
    features.riseLine = LinearRegression(SamplesRange(features.riseStart + skipBorderSamples, features.analysisEnd), getZ);

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

    SamplesRange beforeCompressionSamples(features.analysisStart, ClosestSample(features.compressionStartTime, SearchDirection::Backward));
    features.loadMeanBeforeCompression = calcLoadMean(beforeCompressionSamples);

    SamplesRange afterDecompressionSamples(ClosestSample(features.decompressionEndTime, SearchDirection::Forward), features.analysisEnd);
    features.loadMeanAfterDecompression = calcLoadMean(afterDecompressionSamples);
}

void ProbeAnalysisBase::CalculateLoadAngles(Features &features) const {
    features.loadAngleCompressionStart = features.beforeCompressionLine.CalculateAngle(features.compressionLine);
    features.loadAngleCompressionEnd = features.compressedLine.CalculateAngle(features.compressionLine);
    features.loadAngleDecompressionStart = features.decompressionLine.CalculateAngle(features.compressedLine);
    features.loadAngleDecompressionEnd = features.decompressionLine.CalculateAngle(features.afterDecompressionLine);
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
#if PRINTER_IS_PRUSA_COREONE()
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

bool ProbeAnalysisBase::HasOutOfRangeFeature(Features &features, const char **feature, float *value) const {
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
    auto compressedvsDecompressedAngleAfter = features.compressedLine.CalculateAngle(features.afterDecompressionLine);
    if (std::abs(compressedvsDecompressedAngleAfter) > 40) {
        *feature = "angle_after";
        *value = compressedvsDecompressedAngleAfter;
        return true;
    }
    return false;
}

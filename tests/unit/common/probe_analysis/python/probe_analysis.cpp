#include <pybind11/pybind11.h>

#define private public
#include "probe_analysis.hpp"

namespace py = pybind11;
using namespace pybind11::literals;
using ProbeAnalysis = buddy::ProbeAnalysis<1000>;

PYBIND11_MODULE(probe_analysis, m) {

    py::class_<ProbeAnalysis>(m, "ProbeAnalysis")
        .def(py::init<>())
        .def("store_sample", &ProbeAnalysis::StoreSample)
        .def("set_sampling_interval", &ProbeAnalysis::SetSamplingIntervalMs)
        .def("reset", &ProbeAnalysis::Reset)
        .def("calc_features", [](ProbeAnalysis &analysis) {
            ProbeAnalysis::Features features;
            analysis.CalculateHaltSpan(features);
            if (analysis.CalculateAnalysisRange(features) == false) {
                return py::dict("error"_a = "not-ready");
            }
            if (analysis.CalculateLoadLineApproximationFeatures(features) == false) {
                return py::dict("error"_a = "load-lines");
            }
            if (analysis.CalculateZLineApproximationFeatures(features) == false) {
                return py::dict("error"_a = "z-lines");
            }
            if (!analysis.CheckLineSanity(features)) {
                return py::dict("error"_a = "sanity-check");
            }
            analysis.CalculateLoadMeans(features);
            analysis.CalculateLoadAngles(features);
            features.r2_60ms = analysis.CalculateSegmentedR2s(features, 0.060);
            float finalZ = analysis.InterpolateFinalZCoordinate(features);

            return py::dict(
                "last_sample_end"_a = analysis.TimeOfSample(analysis.window.end() - 1),
                "analysis_start"_a = analysis.TimeOfSample(features.analysisStart),
                "fall_end"_a = analysis.TimeOfSample(features.fallEnd),
                "rise_start"_a = analysis.TimeOfSample(features.riseStart),
                "analysis_end"_a = analysis.TimeOfSample(features.analysisEnd),

                "load_mean_before_compression"_a = features.loadMeanBeforeCompression,
                "load_mean_after_decompression"_a = features.loadMeanAfterDecompression,

                "z_final"_a = finalZ,

                "load_angle_compression_start"_a = features.loadAngleCompressionStart,
                "load_angle_compression_end"_a = features.loadAngleCompressionEnd,
                "load_angle_decompression_start"_a = features.loadAngleDecompressionStart,
                "load_angle_decompression_end"_a = features.loadAngleDecompressionEnd,
                "r2_compression_start_60"_a = features.r2_60ms.compressionStart,
                "r2_compression_end_60"_a = features.r2_60ms.compressionEnd,
                "r2_decompression_start_60"_a = features.r2_60ms.decompressionStart,
                "r2_decompression_end_60"_a = features.r2_60ms.decompressionEnd);
        });
}

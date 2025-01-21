#include "catch2/catch.hpp"
#include <tuple>
#include <sstream>
#include <iostream>
#include <cstring>

#define PROBE_ANALYSIS_DISABLE_METRICS
#define private public
#include "probe_analysis.hpp"

using namespace buddy;

extern "C" uint32_t ticks_us() {
    return 0;
}

template <typename AnalysisType>
void StoreSamples(AnalysisType &analysis, int simulate_delay, std::initializer_list<std::tuple<float, float>> samples) {
    std::size_t idx = 0;
    for (auto &tup : samples) {
        float load = std::get<0>(tup);
        float z;
        if (idx + simulate_delay < samples.size()) {
            z = std::get<1>(*(samples.begin() + idx + simulate_delay));
        } else {
            z = std::get<1>(*(samples.end() - 1));
        }
        analysis.StoreSample(z, load);
        idx++;
    }
}

template <typename It>
class IteratorDistance : public Catch::MatcherBase<int> {
    It a;
    It b;

public:
    IteratorDistance(It a, It b)
        : a(a)
        , b(b) {}

    bool match(int const &other) const override {
        return (b - a) == other;
    }

    virtual std::string describe() const override {
        std::ostringstream ss;
        int distance = b - a;
        ss << "is " << distance << " (";
        if (distance == 0) {
            ss << "left == right";
        } else if (distance > 0) {
            ss << "left + " << distance << " == right";
        } else {
            ss << "left - " << -distance << " == right";
        }
        ss << ")";
        return ss.str();
    }
};

template <typename It>
inline IteratorDistance<It> IsDistanceBetween(It a, It b) {
    return IteratorDistance<It>(a, b);
}

SCENARIO("Analysis properly handles its sample window", "[probe_analysis]") {

    GIVEN("An analysis with an empty window") {
        ProbeAnalysis</*window size=*/12, /*load delay=*/20> analysis;
        analysis.SetSamplingIntervalMs(10);

        WHEN("is without samples") {

            THEN("reports it's not ready for analysis") {
                auto result = analysis.Analyse();
                REQUIRE(result.isGood == false);
                REQUIRE_THAT(result.description, Catch::Equals("not-ready"));
            }
        }

        WHEN("receives just a few samples") {
            analysis.StoreSample(0, 0);
            analysis.StoreSample(0, 0);

            THEN("the internal counter increases") {
                REQUIRE(analysis.window.size() == 2);
            }

            THEN("it is still not ready") {
                auto result = analysis.Analyse();
                REQUIRE(result.isGood == false);
                REQUIRE_THAT(result.description, Catch::Equals("not-ready"));
            }
        }

        WHEN("receives just about enough samples") {
            analysis.SetSamplingIntervalMs(1000);
            for (int i = 0; i < 4; i++) {
                analysis.StoreSample(0, 0);
            }
            for (int i = 0; i < 4; i++) {
                analysis.StoreSample(-1, 0);
            }
            for (int i = 0; i < 4; i++) {
                analysis.StoreSample(0, 0);
            }

            THEN("it is ready for analysis") {
                auto result = analysis.Analyse();
                REQUIRE_THAT(result.description, Catch::Equals("load-lines"));
            }
        }

        WHEN("filled up with a U shape Z samples and U shape load without delay") {
            float load = 100;
            for (int i = 0; i < 4; i++) {
                analysis.StoreSample(/*current_z=*/0, /*current_load=*/load++);
            }
            for (int i = 0; i < 4; i++) {
                analysis.StoreSample(/*current_z=*/-10, /*current_load=*/load++);
            }
            for (int i = 0; i < 4; i++) {
                analysis.StoreSample(/*current_z=*/0, /*current_load=*/load++);
            }

            THEN("the analysis calculates proper halt_start and halt_end times") {
                decltype(analysis)::Features features;
                analysis.CalculateHaltSpan(features);

                REQUIRE(features.fallEnd == analysis.window.begin() + 4);
                REQUIRE(features.riseStart == analysis.window.begin() + 7);
            }

            THEN("compensation for system delay shifts Z values to the future") {
                analysis.CompensateForSystemDelay();

                REQUIRE(analysis.window[5].z == 0);
                REQUIRE(analysis.window[6].z == -10);
                REQUIRE(analysis.window[9].z == -10);
                REQUIRE(analysis.window[10].z == 0);
            }

            THEN("compensation depends on used sampling interval") {
                analysis.SetSamplingIntervalMs(20);
                analysis.CompensateForSystemDelay();

                REQUIRE(analysis.window[4].z == 0);
                REQUIRE(analysis.window[5].z == -10);
                REQUIRE(analysis.window[8].z == -10);
                REQUIRE(analysis.window[9].z == 0);
            }
        }

        WHEN("filled up with rising Z samples") {
            for (int i = 0; i < 12; i++) {
                analysis.StoreSample(/*current_z=*/100 + i * 2, /*current_load=*/0);
            }

            THEN("compensation for system delay fills missing Z coordinates") {
                analysis.CompensateForSystemDelay();

                REQUIRE(analysis.window[2].z == 100);
                REQUIRE(analysis.window[1].z == 100 - 2);
                REQUIRE(analysis.window[0].z == 100 - 4);
            }
        }
    }

    GIVEN("An instance ready for processing") {
        ProbeAnalysis<12> analysis;
        for (int i = 0; i < 12; i++) {
            analysis.StoreSample(0, 0);
        }

        WHEN("Reset() is called") {
            analysis.Reset();

            THEN("the instance is no longer ready") {
                auto result = analysis.Analyse();
                REQUIRE(result.isGood == false);
                REQUIRE_THAT(result.description, Catch::Equals("not-ready"));
            }

            THEN("the instance has an empty window") {
                REQUIRE(analysis.window.size() == 0);
            }
        }
    }
}

SCENARIO("analysis properly calculates probe features", "[probe_analysis]") {
    GIVEN("a simple instance") {
        using ProbeAnalysisT = ProbeAnalysis<5>;
        ProbeAnalysisT analysis;
        analysis.SetSamplingIntervalMs(1000);

        analysis.StoreSample(0, 0);
        analysis.StoreSample(0, 1);
        analysis.StoreSample(0, 3);
        analysis.StoreSample(0, 1);
        analysis.StoreSample(0, 0);

        WHEN("load is being approximated using two specific lines") {
            auto result = analysis.CalculateErrorWhenLoadRepresentedAsLines(
                ProbeAnalysisT::SamplesRange(
                    analysis.window.begin(), analysis.window.begin() + 4),
                analysis.window.begin() + 3);
            REQUIRE_THAT(std::get<0>(result), Catch::Matchers::WithinRel(0.16666666f));
        }

        WHEN("load is being approximated by two lines") {
            auto result = analysis.FindBestTwoLinesApproximation(ProbeAnalysisT::SamplesRange(
                analysis.window.begin(),
                analysis.window.begin() + 4));
            REQUIRE_THAT(2, IsDistanceBetween(analysis.window.begin(), std::get<0>(result)));
        }
    }

    GIVEN("an instance with a simple U shape data") {
        using ProbeAnalysisT = ProbeAnalysis<12>;
        ProbeAnalysisT analysis;
        for (int i = 0; i < 4; i++) {
            analysis.StoreSample(/*current_z=*/0, /*current_load=*/-10);
        }
        for (int i = 0; i < 4; i++) {
            analysis.StoreSample(/*current_z=*/-10, /*current_load=*/-20);
        }
        for (int i = 0; i < 4; i++) {
            analysis.StoreSample(/*current_z=*/0, /*current_load=*/-10);
        }

        WHEN("z is being interpolated over the Z = 0 part") {
            analysis.SetSamplingIntervalMs(1);
            auto samples = ProbeAnalysisT::SamplesRange(analysis.window.begin(), analysis.window.begin() + 3);
            auto line = analysis.LinearRegression(samples, [](ProbeAnalysisT::Sample s) {
                return s->z;
            });
            THEN("both a and b are zero") {
                REQUIRE_THAT(line.a, Catch::Matchers::WithinRel(0.0f));
                REQUIRE_THAT(line.b, Catch::Matchers::WithinRel(0.0f));
            }
        }

        WHEN("z is being interpolated over the Z = 0, Z=-10 part") {
            analysis.SetSamplingIntervalMs(1000);
            auto samples = ProbeAnalysisT::SamplesRange(analysis.window.begin() + 3, analysis.window.begin() + 4);
            auto line = analysis.LinearRegression(samples, [](ProbeAnalysisT::Sample s) {
                return s->z;
            });
            THEN("both a and b have the expected value") {
                REQUIRE_THAT(line.a, Catch::Matchers::WithinRel(-10.0f));
                REQUIRE_THAT(line.b, Catch::Matchers::WithinRel(30.0f));
            }
        }

        WHEN("load is being approximated using line segments") {

            THEN("the approximation error is equal to zero for perfect fitting") {
                /*
                REQUIRE(analysis.CalculateLoadFittingError(analysis.window.begin() + 0, analysis.window.begin() + 3) == 0);
                REQUIRE(analysis.CalculateLoadFittingError(analysis.window.begin() + 3, analysis.window.begin() + 3) == 0);
                REQUIRE(analysis.CalculateLoadFittingError(analysis.window.begin() + 8, analysis.window.begin() + 8) == 0);
                REQUIRE(analysis.CalculateLoadFittingError(analysis.window.begin() + 8, analysis.window.begin() + 11) == 0);
                */
            }

            THEN("analysis finds the best approximation for falling segment") {
                /*
                REQUIRE(analysis.FindBestTwoLinesApproximation(analysis.window.begin() + 0, analysis.window.begin() + 3)
                    == analysis.window.begin() + 0);
                    */
            }
        }
    }

    GIVEN("an instance with a simple ⎺V⎺ shape data ") {
        /*
         * Z shape: \_/
         * load shape ⎺\_/⎺
         *
         *  Simplified version of a clean probe
         */
        using ProbeAnalysisT = ProbeAnalysis<12, /*load delay=*/0, /*skip border samples*/ 0>;
        ProbeAnalysisT analysis;
        analysis.SetSamplingIntervalMs(100);
        float load = 100;
        float z = -1;
        for (int i = 0; i < 3; i++) {
            analysis.StoreSample(/*current_z=*/z--, /*current_load=*/load);
        }
        for (int i = 0; i < 2; i++) {
            analysis.StoreSample(/*current_z=*/z--, /*current_load=*/load--);
        }
        for (int i = 0; i < 2; i++) {
            analysis.StoreSample(/*current_z=*/z, /*current_load=*/load);
        }
        for (int i = 0; i < 2; i++) {
            analysis.StoreSample(/*current_z=*/++z, /*current_load=*/++load);
        }
        for (int i = 0; i < 3; i++) {
            analysis.StoreSample(/*current_z=*/++z, /*current_load=*/load);
        }

        /*
         * TODO: Fix me :(
        WHEN("features are calculated") {
            decltype(analysis)::Features features;
            analysis.CalculateHaltSpan(features);
            REQUIRE(analysis.CalculateAnalysisRange(features) == true);
            analysis.CalculateLoadLineApproximationFeatures(features);
            analysis.CalculateZLineApproximationFeatures(features);
            analysis.CalculateLoadMeans(features);
            analysis.CalculateLoadAngles(features);

            for (size_t i = 0; i < analysis.window.size(); i++)
                std::cout << i << "\t";
            std::cout << std::endl;
            for (size_t i = 0; i < analysis.window.size(); i++)
                std::cout << analysis.window[i].z << "\t";
            std::cout << std::endl;
            for (size_t i = 0; i < analysis.window.size(); i++)
                std::cout << analysis.window[i].load << "\t";
            std::cout << std::endl;

            THEN("halt span is corrent") {
                std::cout << std::endl;
                REQUIRE_THAT(5, IsDistanceBetween(analysis.window.begin(), features.fallEnd));
                REQUIRE_THAT(6, IsDistanceBetween(analysis.window.begin(), features.riseStart));
            }

            THEN("analysis range is correct") {
                REQUIRE_THAT(0, IsDistanceBetween(analysis.window.begin(), features.analysisStart));
                REQUIRE_THAT(11, IsDistanceBetween(analysis.window.begin(), features.analysisEnd));
            }

            THEN("compression range is correct") {
                REQUIRE_THAT(3, IsDistanceBetween(analysis.window.begin(), analysis.ClosestSample(features.compressionStartTime, decltype(analysis)::SearchDirection::Forward)));
                REQUIRE_THAT(8, IsDistanceBetween(analysis.window.begin(), analysis.ClosestSample(features.decompressionEndTime, decltype(analysis)::SearchDirection::Backward)));
            }

            THEN("load means are correct") {
                REQUIRE_THAT(100, Catch::Matchers::WithinRel(features.loadMeanBeforeCompression));
                REQUIRE_THAT(100, Catch::Matchers::WithinRel(features.loadMeanAfterDecompression));
            }

            THEN("load angles are correct") {
                REQUIRE_THAT(features.loadAngleCompressionStart, Catch::Matchers::WithinRel(177.70937f));
                REQUIRE_THAT(features.loadAngleCompressionEnd, Catch::Matchers::WithinRel(177.70937f));
                REQUIRE_THAT(features.loadAngleDecompressionStart, Catch::Matchers::WithinRel(177.70937f));
                REQUIRE_THAT(features.loadAngleDecompressionEnd, Catch::Matchers::WithinRel(177.70937f));
            }
        } */
    }
}

TEST_CASE("probe analysis does not crash on problematic probe instances", "[probe_analysis]") {
    SECTION("instance 01") {
        using ProbeAnalysisT = ProbeAnalysis<1000>;
        ProbeAnalysisT analysis;
#include "probes/probe_110_1600937955.ipp"
        analysis.Analyse();
    }
}

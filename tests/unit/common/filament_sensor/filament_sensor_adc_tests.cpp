#include "catch2/catch.hpp"
#include "filament_sensor_adc_eval.hpp"
#include <fstream>
#include <string>
#include <chrono>
#include <regex>
#include <string_view>
#include <optional>
#include "filters/median_filter.hpp"

using TimeStamp = std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>;
using DataPoint = std::pair<TimeStamp, uint32_t>;
using namespace FSensorADCEval;

std::optional<DataPoint> ParseLine(const std::string &line) {
    // Regular expression pattern for timestamp and value extraction
    std::regex pattern(R"((\d{4})-(\d{2})-(\d{2}) (\d{2}):(\d{2}):(\d{2}),(\d+))");

    // Match results
    std::smatch match;

    if (std::regex_search(line, match, pattern) && match.size() == 8) {
        // Extract timestamp components from the regex match
        int year = std::stoi(match[1]);
        int month = std::stoi(match[2]);
        int day = std::stoi(match[3]);
        int hour = std::stoi(match[4]);
        int minute = std::stoi(match[5]);
        int second = std::stoi(match[6]);

        // Create a std::tm struct
        std::tm tm = {};
        tm.tm_year = year - 1900; // Year since 1900
        tm.tm_mon = month - 1; // Month (0-based)
        tm.tm_mday = day; // Day of the month
        tm.tm_hour = hour; // Hour
        tm.tm_min = minute; // Minute
        tm.tm_sec = second; // Second

        // Convert the std::tm struct to time_t
        std::time_t t = std::mktime(&tm);

        // Convert the value string to uint32_t
        uint32_t value = std::stoi(match[7]);

        return DataPoint(std::chrono::seconds(t), value);
    }
    return std::nullopt;
}

using DataPointsDeq = std::deque<DataPoint>;

bool LoadCSV(const char *fpath, DataPointsDeq &dataPoints) {
    using namespace std;
    ifstream f(fpath);
    if (!f.is_open()) {
        return false;
    }
    string line;
    getline(f, line); // skip first row

    while (getline(f, line)) {
        // split into timestamp and value (there are lines with missing values, so using getline to split by ',' is not a stable way of doing stuff
        auto dp = ParseLine(line);
        if (dp) {
            dataPoints.emplace_back(dp.value());
        }
    }
    return true;
}

FilamentSensorState FilterAndEvaluate(int32_t fs_raw_value, int32_t fs_ref_nins_value, int32_t fs_ref_ins_value, int32_t fs_value_span) {
    static MedianFilter filter;
    if (filter.filter(fs_raw_value)) { // fs_raw_value is rewritten - passed by reference ... WTF?
        return evaluate_state(fs_raw_value, fs_ref_nins_value, fs_ref_ins_value, fs_value_span);
    } else {
        return evaluate_state(filtered_value_not_ready, fs_ref_nins_value, fs_ref_ins_value, fs_value_span);
    }
}

void FSBasicTest(int32_t fsRefNins, int32_t fsRefIns, int32_t fsSpan, int32_t noFil, int32_t hasFil) {
    CHECK(evaluate_state(noFil, fsRefNins, fsRefIns, fsSpan) == FilamentSensorState::NoFilament);
    CHECK(evaluate_state(hasFil, fsRefNins, fsRefIns, fsSpan) == FilamentSensorState::HasFilament);
    CHECK(evaluate_state(filtered_value_not_ready, fsRefNins, fsRefIns, fsSpan) == FilamentSensorState::NotInitialized);
    CHECK(evaluate_state(noFil, ref_value_not_calibrated, fsRefIns, fsSpan) == FilamentSensorState::NotCalibrated);
    CHECK(evaluate_state(lower_limit - 1, fsRefNins, fsRefIns, fsSpan) == FilamentSensorState::NotConnected);
}

TEST_CASE("FilamentSensor basic test", "[filament_sensor]") {
    // using values observed on real printers
    FSBasicTest(1'800'000, ref_value_not_calibrated, 350'000, 1'700'000, 240'000);
    FSBasicTest(1'800'000, 240'000, 350'000, 1'700'000, 240'000);
}

TEST_CASE("FilamentSensor basic test inverted", "[filament_sensor]") {
    // using values observed on real printers
    FSBasicTest(27'000, ref_value_not_calibrated, 350'000, 240'000, 1'700'000);
    FSBasicTest(27'000, 1'700'000, 350'000, 240'000, 1'700'000);
}

TEST_CASE("FilamentSensor flipped", "[filament_sensor]") {
    // false positive check - this needed to be fixed
    // filtered value out of interval range, but in the "correct" direction
    CHECK(evaluate_state(200'000, 240'000, 400'000, 10'000) == FilamentSensorState::NoFilament);
}

TEST_CASE("FilamentSensor flipped dataset1", "[filament_sensor]") {

    DataPointsDeq dataPoints;
    REQUIRE(LoadCSV("2023-04-05_16-14.csv", dataPoints));

    // run the data through the fsensor filtration algorithm
    // - something is confusing the fsensor in the data but nobody has revealed that so far
    FilamentSensorState expectedResult = FilamentSensorState::NoFilament;
    // hypothesis:
    // If incoming fs filtered value < fs_ref_value - fs_value_span, then we may accidentally fail to resolve fsensor correctly
    // example:
    // fs_ref_value = 440'000
    // fs_value_span = 200'000
    // incoming filtered value = 160'000
    //
    // The incoming value may get out of the supported (calibrated) +-value span accidentally - and if this happens, we may get a flipped fsensor state
    // like in this example:
    std::for_each(dataPoints.begin(), dataPoints.end(), [&](auto dp) { expectedResult = FilterAndEvaluate(dp.second, 440'000, 800'000, 200'000); });

    // In the end of the data we must end in filament NOT present
    CHECK(expectedResult == FilamentSensorState::NoFilament);
}

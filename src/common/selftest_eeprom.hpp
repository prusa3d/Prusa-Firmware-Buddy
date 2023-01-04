/**
 * @file selftest_eeprom.hpp
 * @author Radek Vana
 * @brief defines how result of selftest parts are stored in eeprom
 * @date 2022-01-23
 */

#pragma once
#include <stdint.h>

#define SelftestResult_Unknown 0
#define SelftestResult_Skipped 1
#define SelftestResult_Passed  2
#define SelftestResult_Failed  3

union SelftestResultEEprom_t {
    uint32_t ui32;
    uint8_t arr[4];
    struct {
        uint8_t printFan : 2;     // bit 0-1
        uint8_t heatBreakFan : 2; // bit 2-3
        uint8_t xaxis : 2;        // bit 4-5
        uint8_t yaxis : 2;        // bit 6-7
        uint8_t zaxis : 2;        // bit 8-9
        uint8_t nozzle : 2;       // bit 10-11
        uint8_t bed : 2;          // bit 12-13
        uint8_t reserved_0 : 2;   // bit 14-15
        uint8_t reserved_1 : 2;   // bit 16-17
        uint8_t eth : 2;          // bit 17-19
        uint8_t wifi : 2;         // bit 20-21
        uint8_t reserved1 : 2;    // bit 22-23
        uint8_t reserved0;        // bit 24-31
    };
};

#ifdef __cplusplus
static_assert(sizeof(SelftestResultEEprom_t) == sizeof(uint32_t), "Incorrect SelftestResultEEprom_t size");

enum class TestResult_t : uint8_t {
    Unknown = SelftestResult_Unknown,
    Skipped = SelftestResult_Skipped,
    Passed = SelftestResult_Passed,
    Failed = SelftestResult_Failed,
};

constexpr const char *ToString(TestResult_t res) {
    switch (res) {
    case TestResult_t::Unknown:
        return "Unknown";
    case TestResult_t::Skipped:
        return "Skipped";
    case TestResult_t::Passed:
        return "Passed";
    case TestResult_t::Failed:
        return "Failed";
    default:
        break;
    }
    return "ERROR";
}

enum class TestResultNet_t : uint8_t {
    Unknown,   //test did not run
    Unlinked,  // wifi not present, eth cable unplugged
    Down,      // wifi present, eth cable plugged, not selected in lan settings
    NoAddress, // wifi present, no address obtained from DHCP
    Up,        // wifi present, eth cable plugged, selected in lan settings
};

constexpr const char *ToString(TestResultNet_t res) {
    switch (res) {
    case TestResultNet_t::Unknown:
        return "Unknown";
    case TestResultNet_t::Unlinked:
        return "Unlinked";
    case TestResultNet_t::Down:
        return "Down";
    case TestResultNet_t::NoAddress:
        return "NoAddress";
    case TestResultNet_t::Up:
        return "Up";
    default:
        break;
    }
    return "ERROR";
}
#endif // __cplusplus

#pragma once

#include <config_store/constants.hpp>
#include <config_store/old_eeprom/constants.hpp>

/**
 * @brief Generic selftest results.
 */
typedef enum {
    TestResult_Unknown = 0,
    TestResult_Skipped = 1,
    TestResult_Passed = 2,
    TestResult_Failed = 3,
} TestResult;

/**
 * @brief Selftest results for a network interface.
 */
typedef enum {
    TestResultNet_Unknown = 0, // test did not run
    TestResultNet_Unlinked = 1, // wifi not present, eth cable unplugged
    TestResultNet_Down = 2, // wifi present, eth cable plugged, not selected in lan settings
    TestResultNet_NoAddress = 3, // wifi present, no address obtained from DHCP
    TestResultNet_Up = 4, // wifi present, eth cable plugged, selected in lan settings
} TestResultNet;

#pragma pack(push, 1)
/**
 * @brief Results for selftests of one tool.
 * Old version used for eeprom upgrade.
 */
struct SelftestTool_pre_23 {
    TestResult printFan : 2;
    TestResult heatBreakFan : 2;
    TestResult nozzle : 2;
    TestResult fsensor : 2;
    TestResult loadcell : 2;
    TestResult sideFsensor : 2;
    TestResult dockoffset : 2;
    TestResult tooloffset : 2;
};

bool operator==(SelftestTool_pre_23 lhs, SelftestTool_pre_23 rhs);

/**
 * @brief Test results compacted in eeprom.
 * Old version used for eeprom upgrade.
 */
struct SelftestResult_pre_23 {
    TestResult xaxis : 2;
    TestResult yaxis : 2;
    TestResult zaxis : 2;
    TestResult bed : 2;
    TestResultNet eth : 3;
    TestResultNet wifi : 3;
    TestResult zalign : 2;
    SelftestTool_pre_23 tools[config_store_ns::old_eeprom::EEPROM_MAX_TOOL_COUNT];
};

bool operator==(SelftestResult_pre_23 lhs, SelftestResult_pre_23 rhs);

/**
 * @brief Results for selftests of one tool.
 */
struct SelftestTool {
    TestResult printFan : 2;
    TestResult heatBreakFan : 2;
    TestResult fansSwitched : 2; // encapsuling with HAS_SWITCHED_FAN_TEST macro would introduce problems since selftest_result is saved on eeprom as a whole structure
    TestResult nozzle : 2;
    TestResult fsensor : 2;
    TestResult loadcell : 2;
    TestResult sideFsensor : 2;
    TestResult dockoffset : 2;
    TestResult tooloffset : 2;

    bool has_heatbreak_fan_passed();
    TestResult evaluate_fans();
    void reset_fan_tests();
};

bool operator==(SelftestTool lhs, SelftestTool rhs);

/**
 * @brief Test results compacted in eeprom. This struct cannot have constructors because it's part of old eeprom implementation.
 */
struct SelftestResultV23 {
    TestResult xaxis : 2;
    TestResult yaxis : 2;
    TestResult zaxis : 2;
    TestResult bed : 2;
    TestResultNet eth : 3;
    TestResultNet wifi : 3;
    TestResult zalign : 2;
    SelftestTool tools[config_store_ns::old_eeprom::EEPROM_MAX_TOOL_COUNT];
};

/**
 * @brief Test results compacted in eeprom. Copy of SelftestResultV23 only because the old type was needed in old eeprom implementation and was not allowed to have any constructors (ie new version won't have to do this copy)
 */
struct SelftestResult_pre_gears {
    SelftestResult_pre_gears() = default;
    SelftestResult_pre_gears(const SelftestResult_pre_23 &sr_pre23);
    TestResult xaxis : 2 {};
    TestResult yaxis : 2 {};
    TestResult zaxis : 2 {};
    TestResult bed : 2 {};
    TestResultNet eth : 3 {};
    TestResultNet wifi : 3 {};
    TestResult zalign : 2 {};
    SelftestTool tools[config_store_ns::max_tool_count] {};
};

bool operator==(SelftestResult_pre_gears lhs, SelftestResult_pre_gears rhs);

/**
 * @brief Test results compacted in eeprom. Added gears calibration result to eeprom for snake selftest compatibility
 */
struct SelftestResult {
    SelftestResult() = default;
    SelftestResult(const SelftestResult_pre_gears &sr_pre_gears);
    TestResult xaxis : 2 {};
    TestResult yaxis : 2 {};
    TestResult zaxis : 2 {};
    TestResult bed : 2 {};
    TestResultNet eth : 3 {};
    TestResultNet wifi : 3 {};
    TestResult zalign : 2 {};
    TestResult gears : 2 {};
    SelftestTool tools[config_store_ns::max_tool_count] {};
};

bool operator==(SelftestResult lhs, SelftestResult rhs);

#pragma pack(pop)

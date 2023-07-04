#pragma once

#include "eeprom.h"

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
    SelftestTool_pre_23 tools[EEPROM_MAX_TOOL_COUNT];
};

bool operator==(SelftestResult_pre_23 lhs, SelftestResult_pre_23 rhs);

/**
 * @brief Results for selftests of one tool.
 */
struct SelftestTool {
    TestResult printFan : 2;
    TestResult heatBreakFan : 2;
    TestResult fansSwitched : 2;
    TestResult nozzle : 2;
    TestResult fsensor : 2;
    TestResult loadcell : 2;
    TestResult sideFsensor : 2;
    TestResult dockoffset : 2;
    TestResult tooloffset : 2;
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
    SelftestTool tools[EEPROM_MAX_TOOL_COUNT];
};

/**
 * @brief Test results compacted in eeprom. Copy of SelftestResultV23 only because the old type was needed in old eeprom implementation and was not allowed to have any constructors (ie new version won't have to do this copy)
 */
struct SelftestResult {
    SelftestResult() = default;
    SelftestResult(const SelftestResult_pre_23 &sr_pre23);
    TestResult xaxis : 2;
    TestResult yaxis : 2;
    TestResult zaxis : 2;
    TestResult bed : 2;
    TestResultNet eth : 3;
    TestResultNet wifi : 3;
    TestResult zalign : 2;
    SelftestTool tools[EEPROM_MAX_TOOL_COUNT];
};

bool operator==(SelftestResult lhs, SelftestResult rhs);

#pragma pack(pop)

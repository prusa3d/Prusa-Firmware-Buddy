#pragma once

#include <test_result.hpp>
#include <tool_index.hpp>
#include <utils/storage/strong_index_array.hpp>
#include <option/has_switched_fan_test.h>
#include <option/has_indx.h>

static_assert(HAS_INDX());

#pragma pack(push, 1)

/**
 * @brief Results for selftests of one tool.
 * Members are public because the type must be structural (used as NTTP in StoreItem). Use getters/setters for access.
 */
struct SelftestToolIndx {
    TestResult _print_fan : 2;
    TestResult _heatbreak_fan : 2;
    TestResult _nozzle : 2;
    TestResult _loadcell : 2;
    TestResult _dock_fan : 2;
    uint8_t _reserved[1]; // reserved for future use

    bool operator==(const SelftestToolIndx &rhs) const = default;
};
static_assert(sizeof(SelftestToolIndx) == 3);

/**
 * @brief Test results compacted in eeprom.
 * Members are public because the type must be structural (used as NTTP in StoreItem). Use getters/setters for access.
 */
struct SelftestResultImplIndx {
    SelftestResultImplIndx() = default;
    TestResult _xaxis : 2 {};
    TestResult _yaxis : 2 {};
    TestResult _zaxis : 2 {};
    TestResult _bed : 2 {};
    TestResultNet _eth : 3 {};
    TestResultNet _wifi : 3 {};
    TestResult _zalign : 2 {};
    TestResult _dock_calibration : 2 {};
    SelftestToolIndx _tool {};
    TestResult _reserved : 2 * 4 {}; // reserved for future use

    bool operator==(const SelftestResultImplIndx &) const = default;

    inline TestResult get_print_fan([[maybe_unused]] PhysicalToolIndex tool) const { return _tool._print_fan; }

    [[deprecated("Use the ToolIndex overload")]]
    inline void set_print_fan([[maybe_unused]] uint8_t tool, TestResult result) { _tool._print_fan = result; }
    inline void set_print_fan([[maybe_unused]] PhysicalToolIndex tool, TestResult result) { _tool._print_fan = result; }

    inline TestResult get_heatbreak_fan([[maybe_unused]] PhysicalToolIndex tool) const { return _tool._heatbreak_fan; }

    [[deprecated("Use the ToolIndex overload")]]
    inline void set_heatbreak_fan([[maybe_unused]] uint8_t tool, TestResult result) { _tool._heatbreak_fan = result; }
    inline void set_heatbreak_fan([[maybe_unused]] PhysicalToolIndex tool, TestResult result) { _tool._heatbreak_fan = result; }

    inline TestResult get_dock_fan() const { return _tool._dock_fan; }
    inline void set_dock_fan(TestResult result) { _tool._dock_fan = result; }

    inline TestResult get_fans_switched([[maybe_unused]] PhysicalToolIndex tool) const { return TestResult::passed; }

    [[deprecated("Use the ToolIndex overload")]]
    inline void set_fans_switched([[maybe_unused]] uint8_t tool, [[maybe_unused]] TestResult result) { /* empty */
    }
    inline void set_fans_switched([[maybe_unused]] PhysicalToolIndex tool, [[maybe_unused]] TestResult result) { /* empty */
    }

    inline bool has_heatbreak_fan_passed([[maybe_unused]] PhysicalToolIndex tool) const {
        return _tool._heatbreak_fan == TestResult::passed;
    }

    inline TestResult evaluate_fans([[maybe_unused]] PhysicalToolIndex tool) const {
        return test_result::evaluate_results(_tool._print_fan, _tool._heatbreak_fan, _tool._dock_fan);
    }

    inline TestResult get_nozzle_heater([[maybe_unused]] PhysicalToolIndex tool) const { return _tool._nozzle; }

    [[deprecated("Use the ToolIndex overload")]]
    inline void set_nozzle_heater([[maybe_unused]] uint8_t tool, TestResult result) { _tool._nozzle = result; }
    inline void set_nozzle_heater([[maybe_unused]] PhysicalToolIndex tool, TestResult result) { _tool._nozzle = result; }

    inline TestResult get_loadcell([[maybe_unused]] PhysicalToolIndex tool) const { return _tool._loadcell; }
    inline void set_loadcell([[maybe_unused]] PhysicalToolIndex tool, TestResult result) { _tool._loadcell = result; }

    inline TestResult get_dock_offset([[maybe_unused]] PhysicalToolIndex tool) const { return _dock_calibration; }
    inline void set_dock_offset([[maybe_unused]] PhysicalToolIndex tool, TestResult result) { _dock_calibration = result; }

    inline TestResult get_gearbox([[maybe_unused]] PhysicalToolIndex tool) const { return TestResult::passed; }
    inline void set_gearbox([[maybe_unused]] PhysicalToolIndex tool, [[maybe_unused]] TestResult result) { /* empty */
    }

    inline TestResult get_xaxis() const { return _xaxis; }
    inline void set_xaxis(TestResult result) { _xaxis = result; }

    inline TestResult get_yaxis() const { return _yaxis; }
    inline void set_yaxis(TestResult result) { _yaxis = result; }

    inline TestResult get_zaxis() const { return _zaxis; }
    inline void set_zaxis(TestResult result) { _zaxis = result; }

    inline TestResult get_bed_heater() const { return _bed; }
    inline void set_bed_heater(TestResult result) { _bed = result; }

    inline TestResultNet get_ethernet() const { return _eth; }
    inline void set_ethernet(TestResultNet result) { _eth = result; }

    inline TestResultNet get_wifi() const { return _wifi; }
    inline void set_wifi(TestResultNet result) { _wifi = result; }

    inline TestResult get_zalign() const { return _zalign; }
    inline void set_zalign(TestResult result) { _zalign = result; }

    inline TestResult get_deprecated_gears() const {
        // we consider indx printers as new product, so if migration happens, just reset this calibration
        static_assert(TestResult {} == TestResult::unknown); // sanity check
        return TestResult {};
    }
};

static_assert(sizeof(SelftestResultImplIndx) == 7);

#pragma pack(pop)

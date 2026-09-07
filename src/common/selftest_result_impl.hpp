#pragma once

#include <test_result.hpp>
#include <tool_index.hpp>
#include <utils/storage/strong_index_array.hpp>
#include <option/has_switched_fan_test.h>
#include <option/has_indx.h>

static_assert(!HAS_INDX());

#pragma pack(push, 1)

/**
 * @brief Results for selftests of one tool.
 * Members are public because the type must be structural (used as NTTP in StoreItem). Use getters/setters for access.
 */
struct SelftestTool {
    TestResult _print_fan : 2;
    TestResult _heatbreak_fan : 2;
    TestResult _fans_switched : 2; // encapsuling with HAS_SWITCHED_FAN_TEST macro would introduce problems since selftest_result is saved on eeprom as a whole structure
    TestResult _nozzle : 2;
    // BFW-8374: fsensor and sideFsensor are no longer used. Selftest results
    // are derived directly from existence of calibration data, not from
    // separate flags. Do not reuse, or we get confusion on upgrades/downgrades.
    TestResult _deprecated_fsensor : 2;
    TestResult _loadcell : 2;
    // Same as above.
    TestResult _deprecated_side_fsensor : 2;
    TestResult _dockoffset : 2;
    // WARNING: This is doubled storage of the Tool Offset Calibration results, to be corrected
    // TODO: BFW-9196
    TestResult _tooloffset : 2;
    TestResult _gears : 2;

    bool operator==(const SelftestTool &rhs) const = default;
};
static_assert(sizeof(SelftestTool) == 3);

/**
 * @brief Test results compacted in eeprom. Added gearbox alignment result to eeprom for snake selftest compatibility
 * Members are public because the type must be structural (used as NTTP in StoreItem). Use getters/setters for access.
 */
struct SelftestResultImpl {
    static constexpr size_t tools_count = 6; ///< Number of tool slots in stored layout. Do not change without config store migration.

    SelftestResultImpl() = default;
    TestResult _xaxis : 2 {};
    TestResult _yaxis : 2 {};
    TestResult _zaxis : 2 {};
    TestResult _bed : 2 {};
    TestResultNet _eth : 3 {};
    TestResultNet _wifi : 3 {};
    TestResult _zalign : 2 {};
    // This member is no longer used and is kept to allow backwards compatibility with config store
    // It was replaced by a result supporting more than one toolhead, that can
    // be found in SelftTool class
    TestResult _deprecated_gears : 2 {};
    StrongIndexArray<SelftestTool, tools_count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static, strong_index_array::AllowWeakIndexing::yes> _tools {};

    bool operator==(const SelftestResultImpl &) const = default;

    [[deprecated("Use the ToolIndex overload")]]
    inline TestResult get_print_fan(uint8_t tool) const { return _tools[tool]._print_fan; }
    inline TestResult get_print_fan(PhysicalToolIndex tool) const { return _tools[tool]._print_fan; }

    [[deprecated("Use the ToolIndex overload")]]
    inline void set_print_fan(uint8_t tool, TestResult result) { _tools[tool]._print_fan = result; }
    inline void set_print_fan(PhysicalToolIndex tool, TestResult result) { _tools[tool]._print_fan = result; }

    [[deprecated("Use the ToolIndex overload")]]
    inline TestResult get_heatbreak_fan(uint8_t tool) const { return _tools[tool]._heatbreak_fan; }
    inline TestResult get_heatbreak_fan(PhysicalToolIndex tool) const { return _tools[tool]._heatbreak_fan; }

    [[deprecated("Use the ToolIndex overload")]]
    inline void set_heatbreak_fan(uint8_t tool, TestResult result) { _tools[tool]._heatbreak_fan = result; }
    inline void set_heatbreak_fan(PhysicalToolIndex tool, TestResult result) { _tools[tool]._heatbreak_fan = result; }

    [[deprecated("Use the ToolIndex overload")]]
    inline TestResult get_fans_switched(uint8_t tool) const { return _tools[tool]._fans_switched; }
    inline TestResult get_fans_switched(PhysicalToolIndex tool) const { return _tools[tool]._fans_switched; }

    [[deprecated("Use the ToolIndex overload")]]
    inline void set_fans_switched(uint8_t tool, TestResult result) { _tools[tool]._fans_switched = result; }
    inline void set_fans_switched(PhysicalToolIndex tool, TestResult result) { _tools[tool]._fans_switched = result; }

    [[deprecated("Use the ToolIndex overload")]]
    inline bool has_heatbreak_fan_passed(uint8_t tool) const {
#if HAS_SWITCHED_FAN_TEST()
        return _tools[tool]._heatbreak_fan == TestResult::passed && _tools[tool]._fans_switched == TestResult::passed;
#else
        return _tools[tool]._heatbreak_fan == TestResult::passed;
#endif
    }
    inline bool has_heatbreak_fan_passed(PhysicalToolIndex tool) const { return has_heatbreak_fan_passed(tool.to_raw()); }

    [[deprecated("Use the ToolIndex overload")]]
    inline TestResult evaluate_fans(uint8_t tool) const {
#if HAS_SWITCHED_FAN_TEST()
        return test_result::evaluate_results(_tools[tool]._print_fan, _tools[tool]._heatbreak_fan, _tools[tool]._fans_switched);
#else
        return test_result::evaluate_results(_tools[tool]._print_fan, _tools[tool]._heatbreak_fan);
#endif
    }
    inline TestResult evaluate_fans(PhysicalToolIndex tool) const { return evaluate_fans(tool.to_raw()); }

    [[deprecated("Use the ToolIndex overload")]]
    inline TestResult get_nozzle_heater(uint8_t tool) const { return _tools[tool]._nozzle; }
    inline TestResult get_nozzle_heater(PhysicalToolIndex tool) const { return _tools[tool]._nozzle; }

    [[deprecated("Use the ToolIndex overload")]]
    inline void set_nozzle_heater(uint8_t tool, TestResult result) { _tools[tool]._nozzle = result; }
    inline void set_nozzle_heater(PhysicalToolIndex tool, TestResult result) { _tools[tool]._nozzle = result; }

    [[deprecated("Use the ToolIndex overload")]]
    inline TestResult get_loadcell(uint8_t tool) const { return _tools[tool]._loadcell; }
    inline TestResult get_loadcell(PhysicalToolIndex tool) const { return _tools[tool]._loadcell; }

    [[deprecated("Use the ToolIndex overload")]]
    inline void set_loadcell(uint8_t tool, TestResult result) { _tools[tool]._loadcell = result; }
    inline void set_loadcell(PhysicalToolIndex tool, TestResult result) { _tools[tool]._loadcell = result; }

    [[deprecated("Use the ToolIndex overload")]]
    inline TestResult get_dock_offset(uint8_t tool) const { return _tools[tool]._dockoffset; }
    inline TestResult get_dock_offset(PhysicalToolIndex tool) const { return _tools[tool]._dockoffset; }

    [[deprecated("Use the ToolIndex overload")]]
    inline void set_dock_offset(uint8_t tool, TestResult result) { _tools[tool]._dockoffset = result; }
    inline void set_dock_offset(PhysicalToolIndex tool, TestResult result) { _tools[tool]._dockoffset = result; }

    [[deprecated("Use the ToolIndex overload")]]
    inline TestResult get_tool_offset(uint8_t tool) const { return _tools[tool]._tooloffset; }
    inline TestResult get_tool_offset(PhysicalToolIndex tool) const { return _tools[tool]._tooloffset; }

    [[deprecated("Use the ToolIndex overload")]]
    inline void set_tool_offset(uint8_t tool, TestResult result) { _tools[tool]._tooloffset = result; }
    inline void set_tool_offset(PhysicalToolIndex tool, TestResult result) { _tools[tool]._tooloffset = result; }

    [[deprecated("Use the ToolIndex overload")]]
    inline TestResult get_gearbox(uint8_t tool) const { return _tools[tool]._gears; }
    inline TestResult get_gearbox(PhysicalToolIndex tool) const { return _tools[tool]._gears; }

    [[deprecated("Use the ToolIndex overload")]]
    inline void set_gearbox(uint8_t tool, TestResult result) { _tools[tool]._gears = result; }
    inline void set_gearbox(PhysicalToolIndex tool, TestResult result) { _tools[tool]._gears = result; }

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

    inline TestResult get_deprecated_gears() const { return _deprecated_gears; }
};

#pragma pack(pop)

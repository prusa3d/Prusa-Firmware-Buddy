// selftest.cpp

#include "printer_selftest.hpp"
#include <selftest/selftest_invocation.hpp>
#include <fcntl.h>
#include <unistd.h>
#include "selftest_axis.h"
#include "selftest_heater.h"
#include "selftest_loadcell.h"
#include "selftest_dock.h"
#include "stdarg.h"
#include "otp.hpp"
#include "hwio.h"
#include "marlin_server.hpp"
#include <guiconfig/wizard_config.hpp>
#include "../../Marlin/src/module/stepper.h"
#include "../../Marlin/src/module/temperature.h"
#include "selftest_axis_type.hpp"
#include "selftest_heaters_type.hpp"
#include "selftest_heaters_interface.hpp"
#include "selftest_loadcell_interface.hpp"
#include "selftest_axis_interface.hpp"
#include "selftest_netstatus_interface.hpp"
#include "selftest_dock_interface.hpp"
#include "selftest_tool_offsets_interface.hpp"
#include "selftest_axis_config.hpp"
#include "selftest_heater_config.hpp"
#include "selftest_loadcell_config.hpp"
#include "calibration_z.hpp"
#include "fanctl.hpp"
#include "timing.h"
#include "selftest_result_type.hpp"
#include <config_store/store_instance.hpp>
#include "common/selftest/selftest_data.hpp"
#include "i_selftest.hpp"
#include "i_selftest_part.hpp"
#include "selftest_result_type.hpp"

using namespace selftest;

#define HOMING_TIME 15000 // ~15s when X and Y axes are at opposite side to home position
static constexpr feedRate_t maxFeedrates[] = DEFAULT_MAX_FEEDRATE;

/// These speeds create major chord
/// https://en.wikipedia.org/wiki/Just_intonation

static constexpr float XYfr_table[] = { HOMING_FEEDRATE_XY / 60 };
static constexpr size_t xy_fr_table_size = sizeof(XYfr_table) / sizeof(XYfr_table[0]);
static constexpr float Zfr_table_fw[] = { maxFeedrates[Z_AXIS] }; // up
static constexpr float Zfr_table_bw[] = { HOMING_FEEDRATE_Z / 60 };
static constexpr size_t z_fr_tables_size = sizeof(Zfr_table_fw) / sizeof(Zfr_table_fw[0]);

// reads data from eeprom, cannot be constexpr
//  FIXME: remove fixed lengths once the printer specs are finalized
const AxisConfig_t selftest::Config_XAxis = {
    .partname = "X-Axis",
    // The test overshoots by EXTRA_LEN_MM to hit the axis end; cancel it, the collision
    // would unlock a picked tool. +2 is margin against length_min lost to step rounding.
    .length = X_BED_SIZE - CSelftestPart_Axis::EXTRA_LEN_MM + 2,
    .fr_table_fw = XYfr_table,
    .fr_table_bw = XYfr_table,
    .length_min = X_BED_SIZE,
    .length_max = X_BED_SIZE + X_END_GAP + 10,
    .axis = X_AXIS,
    .steps = xy_fr_table_size * 2,
    .movement_dir = 1,
    .park = true,
    .park_pos = 10,
};

const AxisConfig_t selftest::Config_YAxis = {
    .partname = "Y-Axis",
    // Same cancellation as on X, here to stay clear of the toolchanger; XL has no Y end
    // to hit. +2 is margin against length_min lost to step rounding.
    .length = Y_BED_SIZE - CSelftestPart_Axis::EXTRA_LEN_MM + 2,
    .fr_table_fw = XYfr_table,
    .fr_table_bw = XYfr_table,
    .length_min = Y_BED_SIZE,
    .length_max = Y_BED_SIZE + Y_END_GAP + 10,
    .axis = Y_AXIS,
    .steps = xy_fr_table_size * 2,
    .movement_dir = 1,
    .park = true,
    .park_pos = 10,
};

static const AxisConfig_t Config_ZAxis = {
    .partname = "Z-Axis",
    .length = get_z_max_pos_mm(),
    .fr_table_fw = Zfr_table_fw,
    .fr_table_bw = Zfr_table_bw,
    .length_min = get_z_max_pos_mm() - 3,
    .length_max = get_z_max_pos_mm() + 3,
    .axis = Z_AXIS,
    .steps = z_fr_tables_size,
    .movement_dir = 1,
    .park = false,
    .park_pos = 0,
};

template <int index>
static consteval HeaterConfig_t make_nozzle_config(const char *name) {
    static_assert(index >= 0 && index < PhysicalToolIndex::count);
    static constexpr auto tool = PhysicalToolIndex::from_raw(index);
    return {
        .partname = name,
        .type = heater_type_t::Nozzle,
        .tool_nr = tool,
        .getTemp = []() { return Hotend::for_tool(tool).nozzle_temp(); },
        .setTargetTemp = [](int target_temp) { Hotend::for_tool(tool).set_nozzle_target_temp(target_temp); },
        .get_pid = []() { return Hotend::for_tool(tool).nozzle_pid_config_compat(); },
        .set_pid = [](const PID_t &pid) { Hotend::for_tool(tool).set_nozzle_pid_config_compat(pid); },
        .heatbreak_fan_fnc = Fans::heat_break,
        .print_fan_fnc = Fans::print,
        .heat_time_ms = 42000,
        .start_temp = 80,
        .undercool_temp = 75,
        .target_temp = 290,
        /**
         * @note Resulting temperature after nozzle heater test is set by the internal model control that is used in Dwarf.
         * @todo Completely retune the PID in dwarf.
         */
        .heat_min_temp = 155,
        .heat_max_temp = 245,
        .heatbreak_min_temp = 10,
        .heatbreak_max_temp = 45,
        .heater_load_stable_ms = 1000,
        .heater_full_load_min_W = 20, // 35 W +- 43%
        .heater_full_load_max_W = 50,
        .min_pwm_to_measure = 255 // Check power only when fully on
    };
}

static constexpr HeaterConfig_t Config_HeaterNozzle[] = {
    make_nozzle_config<0>("Nozzle 1"),
    make_nozzle_config<1>("Nozzle 2"),
    make_nozzle_config<2>("Nozzle 3"),
    make_nozzle_config<3>("Nozzle 4"),
    make_nozzle_config<4>("Nozzle 5")
};

static constexpr HeaterConfig_t Config_HeaterBed = {
    .partname = "Bed",
    .type = heater_type_t::Bed,
    .tool_nr = PhysicalToolIndex::from_raw(0),
    .getTemp = []() -> Hotend::OptionalTemperature { return thermalManager.temp_bed.celsius; },
    .setTargetTemp = [](int target_temp) { thermalManager.setTargetBed(target_temp); },
    .get_pid = []() { return PID_t {}; },
    .set_pid = [](const PID_t &) {},
    .heatbreak_fan_fnc = Fans::heat_break,
    .print_fan_fnc = Fans::print,
    .heat_time_ms = 65000,
    .start_temp = 40,
    .undercool_temp = 39,
    .target_temp = 110,
    .heat_min_temp = 50,
    .heat_max_temp = 65,
    .heatbreak_min_temp = -1,
    .heatbreak_max_temp = -1,
    .heater_load_stable_ms = 3000,
    .heater_full_load_min_W = 268,
    .heater_full_load_max_W = 499,
    .min_pwm_to_measure = 26
};

static consteval LoadcellConfig_t make_loadcell_config(PhysicalToolIndex tool, const char *name) {
    return {
        .partname = name,
        .tool_nr = tool,
        .heatbreak_fan_fnc = Fans::heat_break,
        .print_fan_fnc = Fans::print,
        .cool_temp = 50,
        .countdown_sec = 5,
        .countdown_load_error_value = 250,
        .tap_min_load_ok = 500,
        .tap_max_load_ok = 2000,
        .tap_timeout_ms = 2000,
        .z_extra_pos = 100,
        .z_extra_pos_fr = uint32_t(maxFeedrates[Z_AXIS]),
        .max_validation_time = 1000
    };
}

static constexpr LoadcellConfig_t Config_Loadcell[] = {
    make_loadcell_config(PhysicalToolIndex::from_raw(0), "Loadcell 1"),
    make_loadcell_config(PhysicalToolIndex::from_raw(1), "Loadcell 2"),
    make_loadcell_config(PhysicalToolIndex::from_raw(2), "Loadcell 3"),
    make_loadcell_config(PhysicalToolIndex::from_raw(3), "Loadcell 4"),
    make_loadcell_config(PhysicalToolIndex::from_raw(4), "Loadcell 5")
};

static consteval DockConfig_t make_dock_config(PhysicalToolIndex tool) {
    return {
        .dock_id = tool,
        .z_extra_pos = 100,
        .z_extra_pos_fr = maxFeedrates[Z_AXIS],
    };
}

static constexpr std::array<const DockConfig_t, PhysicalToolIndex::count> Config_Docks = { {
    make_dock_config(PhysicalToolIndex::from_raw(0)),
    make_dock_config(PhysicalToolIndex::from_raw(1)),
    make_dock_config(PhysicalToolIndex::from_raw(2)),
    make_dock_config(PhysicalToolIndex::from_raw(3)),
    make_dock_config(PhysicalToolIndex::from_raw(4)),
} };

static constexpr ToolOffsetsConfig_t Config_ToolOffsets = {};

// class representing whole self-test
class CSelftest : public ISelftest {
public:
    CSelftest() {}

public:
    virtual bool IsInProgress() const override;
    virtual bool IsAborted() const override;
    virtual bool Start(const uint64_t test_mask, const selftest::TestData test_data) override; // parent has no clue about SelftestMask_t
    virtual void Loop() override;
    virtual bool Abort() override;

protected:
    void restoreAfterSelftest();
    virtual void next() override;
    void phaseDidSelftestPass();

protected:
    SelftestState_t m_State = stsIdle;
    SelftestMask_t m_Mask = stmNone;
    ToolMask tool_mask = AllTools {};
    selftest::IPartHandler *pXAxis = nullptr;
    selftest::IPartHandler *pYAxis = nullptr;
    selftest::IPartHandler *pZAxis = nullptr;
    std::array<selftest::IPartHandler *, PhysicalToolIndex::count> pNozzles;
    selftest::IPartHandler *pBed = nullptr;
    std::array<selftest::IPartHandler *, PhysicalToolIndex::count> m_pLoadcell;
    std::array<selftest::IPartHandler *, PhysicalToolIndex::count> pDocks;
    selftest::IPartHandler *pToolOffsets;

    SelftestResult m_result;
};

bool CSelftest::IsInProgress() const {
    return ((m_State != stsIdle) && (m_State != stsFinished) && (m_State != stsAborted));
}

bool CSelftest::IsAborted() const {
    return (m_State == stsAborted);
}

bool CSelftest::Start(const uint64_t test_mask, const selftest::TestData test_data) {
    m_result = config_store().selftest_result.get();
    m_Mask = SelftestMask_t(test_mask);
    if (m_Mask & (stmXAxis | stmYAxis | stmZAxis)) {
        m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmWait_axes));
        if (m_result.get_zaxis() != TestResult::passed) {
            m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmEnsureZAway)); // Ensure Z is away enough if Z not calibrated yet
        }
    }
    if (m_Mask & stmHeaters) {
        m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmWait_heaters));
    }
    if (m_Mask & stmLoadcell) {
        m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmWait_loadcell));
    }
    m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmSelftestStop)); // any selftest state will trigger selftest additional deinit

    if (std::holds_alternative<ToolMask>(test_data)) {
        this->tool_mask = std::get<ToolMask>(test_data);
    } else {
        this->tool_mask = AllTools {};
    }

    m_State = stsStart;
    return true;
}

void CSelftest::Loop() {
    uint32_t time = ticks_ms();
    if ((time - m_Time) < SELFTEST_LOOP_PERIODE) {
        return;
    }
    m_Time = time;

    selftest::TestReturn ret = true;
    switch (m_State) {
    case stsIdle:
        return;
    case stsStart:
        phaseStart();
        break;
    case stsDocks:
        if (prusa_toolchanger.is_toolchanger_enabled() && (ret = selftest::phaseDocks(tool_mask, pDocks, Config_Docks))) {
            return;
        }
        break;
    case stsToolOffsets:
        if ((ret = selftest::phaseToolOffsets(tool_mask, pToolOffsets, Config_ToolOffsets))) {
            return;
        }
        break;
    case stsLoadcell:
        if ((ret = selftest::phaseLoadcell(tool_mask, m_pLoadcell, Config_Loadcell))) {
            return;
        }
        break;
    case stsWait_loadcell:
        if (phaseWait()) {
            return;
        }
        break;
    case stsZcalib: {
        // calib_Z(true) requires picked tool, which at this time may not be
        calib_Z(false);
        break;
    }
    case stsEnsureZAway: {
        do_z_clearance(10);
        break;
    }
    case stsXAxis: {
        if (selftest::phaseAxis(pXAxis, Config_XAxis, Separate::yes)) {
            return;
        }
        break;
    }
    case stsYAxis: {
        if (selftest::phaseAxis(pYAxis, Config_YAxis, Separate::yes)) {
            return;
        }
        break;
    }
    case stsZAxis: {
        if (selftest::phaseAxis(pZAxis, Config_ZAxis, Separate::yes)) {
            return;
        }
        break;
    }
    case stsWait_axes:
        if (phaseWait()) {
            return;
        }
        break;
    case stsHeaters_noz_ena:
        selftest::phaseHeaters_noz_ena(pNozzles, Config_HeaterNozzle);
        break;
    case stsHeaters_bed_ena:
        selftest::phaseHeaters_bed_ena(pBed, Config_HeaterBed);
        break;
    case stsHeaters:
        if (selftest::phaseHeaters(pNozzles, &pBed)) {
            return;
        }
        break;

    case stsWait_heaters:
        if (phaseWait()) {
            return;
        }
        break;

    case stsReviseSetupAfterHeaters:
        m_result = config_store().selftest_result.get();

        if (m_result.get_bed_heater() == TestResult::failed) {
            marlin_server::fsm_change(PhasesSelftest::Heaters_AskBedSheetAfterFail, {});
            switch (marlin_server::get_response_from_phase(PhasesSelftest::Heaters_AskBedSheetAfterFail)) {

            case Response::Retry:
                m_State = stsHeaters_noz_ena;
                return;

            case Response::Ok:
                break;

            default:
                return;
            }
        }
        break;
    case stsSelftestStop:
        restoreAfterSelftest();
        break;
    case stsFinish:
        phaseFinish();
        break;
    case stsFinished:
    case stsAborted:
        return;
    }

    if (ret.WasSkipped()) {
        Abort();
    } else {
        next();
    }
}

void CSelftest::phaseDidSelftestPass() {
    m_result = config_store().selftest_result.get();
    SelftestResult_Log(m_result);
}

bool CSelftest::Abort() {
    if (!IsInProgress()) {
        return false;
    }
    abort_part((selftest::IPartHandler **)&pXAxis);
    abort_part((selftest::IPartHandler **)&pYAxis);
    abort_part((selftest::IPartHandler **)&pZAxis);
    abort_part(&pBed);
    for (auto &pNozzle : pNozzles) {
        abort_part(&pNozzle);
    }
    for (auto &loadcell : m_pLoadcell) {
        abort_part(&loadcell);
    }
    for (auto &dock : pDocks) {
        abort_part(&dock);
    }
    abort_part(&pToolOffsets);
    selftest_invocation::mark_aborted();
    m_State = stsAborted;

    phaseFinish();
    return true;
}

void CSelftest::restoreAfterSelftest() {
    // disable heater target values - thermalManager.disable_all_heaters does not do that
    thermalManager.setTargetBed(0);
    for (auto tool : PhysicalToolIndex::all()) {
        if (buddy::puppies::dwarfs[tool].is_enabled()) {
            thermalManager.setTargetHotend(0, tool);
        }
    }

    thermalManager.disable_all_heaters();
}

void CSelftest::next() {
    if ((m_State == stsFinished) || (m_State == stsAborted)) {
        return;
    }
    int state = m_State + 1;
    while ((((uint64_t(1) << state) & m_Mask) == 0) && (state < stsFinish)) {
        state++;
    }
    m_State = (SelftestState_t)state;
}

// declared in parent source file
ISelftest &SelftestInstance() {
    static CSelftest ret = CSelftest();
    return ret;
}

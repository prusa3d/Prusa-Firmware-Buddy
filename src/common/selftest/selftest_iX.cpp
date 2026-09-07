/**
 * @file
 */

#include "printer_selftest.hpp"
#include <selftest/selftest_invocation.hpp>
#include <fcntl.h>
#include <unistd.h>
#include "selftest_axis.h"
#include "selftest_heater.h"
#include "selftest_loadcell.h"
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
#include "selftest_axis_config.hpp"
#include "selftest_heater_config.hpp"
#include "selftest_loadcell_config.hpp"
#include "calibration_z.hpp"
#include "fanctl.hpp"
#include "timing.h"
#include "selftest_result_type.hpp"
#include "selftest_types.hpp"
#include <config_store/store_instance.hpp>
#include "i_selftest.hpp"

using namespace selftest;

#define Z_AXIS_DO_NOT_TEST_MOVE_DOWN
#define HOMING_TIME 15000 // ~15s when X and Y axes are at opposite side to home position
static constexpr feedRate_t maxFeedrates[] = DEFAULT_MAX_FEEDRATE;

/// These speeds create major chord
/// https://en.wikipedia.org/wiki/Just_intonation

static constexpr float XYfr_table[] = { HOMING_FEEDRATE_XY / 60 };
static constexpr size_t xy_fr_table_size = sizeof(XYfr_table) / sizeof(XYfr_table[0]);
static constexpr float Zfr_table_fw[] = { maxFeedrates[Z_AXIS] }; // up
static constexpr float Zfr_table_bw[] = { HOMING_FEEDRATE_Z / 60 };
#ifdef Z_AXIS_DO_NOT_TEST_MOVE_DOWN
static constexpr size_t z_fr_tables_size = sizeof(Zfr_table_fw) / sizeof(Zfr_table_fw[0]);
#else
// #error dead code found by automatic analyses (see BFW-5461)
static constexpr size_t z_fr_tables_size = sizeof(Zfr_table_fw) / sizeof(Zfr_table_fw[0]) + sizeof(Zfr_table_bw) / sizeof(Zfr_table_bw[0]);
#endif

static constexpr float homing_backoff[] = HOMING_BACKOFF_POST_MM;

// reads data from eeprom, cannot be constexpr
const AxisConfig_t selftest::Config_XAxis = {
    .partname = "X-Axis",
    .length = X_MAX_LENGTH,
    .fr_table_fw = XYfr_table,
    .fr_table_bw = XYfr_table,
    .length_min = X_MAX_LENGTH,
    .length_max = X_MAX_LENGTH + X_END_GAP,
    .axis = X_AXIS,
    .steps = xy_fr_table_size,
    .movement_dir = -1,
    .park = true,
    .park_pos = 15,
};

const AxisConfig_t selftest::Config_YAxis = {
    .partname = "Y-Axis",
    .length = Y_MAX_LENGTH,
    .fr_table_fw = XYfr_table,
    .fr_table_bw = XYfr_table,
    .length_min = Y_MAX_LENGTH,
    .length_max = Y_MAX_LENGTH + Y_END_GAP,
    .axis = Y_AXIS,
    .steps = xy_fr_table_size,
    .movement_dir = -1,
    .park = true,
    .park_pos = 15,
};

static const AxisConfig_t Config_ZAxis = {
    .partname = "Z-Axis",
    .length = get_z_max_pos_mm(),
    .fr_table_fw = Zfr_table_fw,
    .fr_table_bw = Zfr_table_bw,
    .length_min = get_z_max_pos_mm() - 4,
    .length_max = get_z_max_pos_mm() + 6,
    .axis = Z_AXIS,
    .steps = z_fr_tables_size,
    .movement_dir = 1,
    .park = false,
    .park_pos = 0,
};

static constexpr HeaterConfig_t Config_HeaterNozzle[] = {
    {
        .partname = "Nozzle",
        .type = heater_type_t::Nozzle,
        .getTemp = []() { return Hotend::for_tool(0).nozzle_temp(); },
        .setTargetTemp = [](int target_temp) { thermalManager.setTargetHotend(target_temp, 0); },
        .get_pid = []() { return Hotend::for_tool(0).nozzle_pid_config_compat(); },
        .set_pid = [](const PID_t &pid) { Hotend::for_tool(0).set_nozzle_pid_config_compat(pid); },
        .heatbreak_fan_fnc = Fans::heat_break,
        .print_fan_fnc = Fans::print,
        .heat_time_ms = 42000,
        .start_temp = 80,
        .undercool_temp = 75,
        .target_temp = 290,
        .heat_min_temp = 195,
        .heat_max_temp = 245,
        .heatbreak_min_temp = 10,
        .heatbreak_max_temp = 45,
        .heater_load_stable_ms = 200,
        .heater_full_load_min_W = 20,
        .heater_full_load_max_W = 50,
        .min_pwm_to_measure = 26,
    }
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
    .heat_min_temp = 61, // midpoint around 68 since selftest is run without a sheet
    .heat_max_temp = 76,
    .heatbreak_min_temp = -1,
    .heatbreak_max_temp = -1,
    .heater_load_stable_ms = 3000,
    .heater_full_load_min_W = 150,
    .heater_full_load_max_W = 285,
    .min_pwm_to_measure = 26
};

#if HAS_HEATERS_SELFTEST_GCODE()
namespace selftest {
HeaterConfig_t nozzle_heater_config() {
    return Config_HeaterNozzle[0];
}
HeaterConfig_t bed_heater_config() {
    return Config_HeaterBed;
}
} // namespace selftest
#endif

static constexpr LoadcellConfig_t Config_Loadcell[] = { {
    .partname = "Loadcell",
    .tool_nr = PhysicalToolIndex::from_raw(0),
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
    .max_validation_time = 1000,
} };

// class representing whole self-test
class CSelftest : public ISelftest {
public:
    CSelftest();

public:
    virtual bool IsInProgress() const override;
    virtual bool IsAborted() const override;
    virtual bool Start(const uint64_t test_mask, const selftest::TestData test_data) override; // parent has no clue about SelftestMask_t
    virtual void Loop() override;
    virtual bool Abort() override;

protected:
    void restoreAfterSelftest();
    virtual void next() override;
    bool phaseWaitUser(PhasesSelftest phase);
    void phaseDidSelftestPass();

protected:
    SelftestState_t m_State;
    SelftestMask_t m_Mask;
    selftest::IPartHandler *pXAxis;
    selftest::IPartHandler *pYAxis;
    selftest::IPartHandler *pZAxis;
    std::array<selftest::IPartHandler *, PhysicalToolIndex::count> pNozzles;
    selftest::IPartHandler *pBed;
    std::array<selftest::IPartHandler *, PhysicalToolIndex::count> m_pLoadcell;

    SelftestResult m_result;
};

CSelftest::CSelftest()
    : m_State(stsIdle)
    , m_Mask(stmNone)
    , pXAxis(nullptr)
    , pYAxis(nullptr)
    , pZAxis(nullptr)
    , pBed(nullptr) {
}

bool CSelftest::IsInProgress() const {
    return ((m_State != stsIdle) && (m_State != stsFinished) && (m_State != stsAborted));
}

bool CSelftest::IsAborted() const {
    return (m_State == stsAborted);
}

bool CSelftest::Start(const uint64_t test_mask, [[maybe_unused]] const TestData test_data) {
    m_Mask = SelftestMask_t(test_mask);
    if (m_Mask & (stmXAxis | stmYAxis | stmZAxis)) {
        m_Mask = static_cast<SelftestMask_t>(m_Mask | uint64_t(stmWait_axes));
        if (m_result.get_zaxis() != TestResult::passed) {
            m_Mask = static_cast<SelftestMask_t>(m_Mask | static_cast<uint64_t>(stmEnsureZAway));
        }
    }
    if (m_Mask & stmHeaters) {
        m_Mask = static_cast<SelftestMask_t>(m_Mask | uint64_t(stmWait_heaters));
    }
    if (m_Mask & stmLoadcell) {
        m_Mask = static_cast<SelftestMask_t>(m_Mask | uint64_t(stmWait_loadcell));
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
    switch (m_State) {
    case stsIdle:
        return;
    case stsStart:
        phaseStart();
        break;
    case stsLoadcell:
        if (selftest::phaseLoadcell(AllTools {}, m_pLoadcell, Config_Loadcell)) {
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
        // Y is not skipped even if X fails
        break;
    }
    case stsYAxis: {
        if (selftest::phaseAxis(pYAxis, Config_YAxis, Separate::yes)) {
            return;
        }
        break;
    } break;
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
    next();
}

void CSelftest::phaseDidSelftestPass() {
    m_result = config_store().selftest_result.get();
    SelftestResult_Log(m_result);
}

bool CSelftest::phaseWaitUser(PhasesSelftest phase) {
    const Response response = marlin_server::get_response_from_phase(phase);
    if (response == Response::Abort || response == Response::Cancel) {
        Abort();
    }
    if (response == Response::Ignore) {
        Abort();
    }
    return response == Response::_none;
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
    selftest_invocation::mark_aborted();
    m_State = stsAborted;

    phaseFinish();
    return true;
}

void CSelftest::restoreAfterSelftest() {
    // disable heater target values - thermalManager.disable_all_heaters does not do that
    thermalManager.setTargetBed(0);
    thermalManager.setTargetHotend(0, 0);

    thermalManager.disable_all_heaters();
    disable_all_steppers();
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

    // check, if state can run
    // this must be done after mask check
    m_result = config_store().selftest_result.get();
    switch (m_State) {
// don't skip Z calibration and X and Y axis tests when loadcell fails
// currently only disabled we might want it back
#if 0
    // #error dead code found by automatic analyses (see BFW-5461)
    case stsZcalib:
    case stsXAxis:
    case stsYAxis: // Y is not skipped even if X fails
        if (TestResult_t(m_result.loadcell) == TestResult_t::Passed)
            return; // current state can be run
        break;      // current state cannot be run
#endif
    default:
        return; // current state can be run
    }

    // current state cannot be run
    // call recursively: it is fine, this function is tiny and there will be few iterations
    marlin_server::set_warning(WarningType::ActionSelftestRequired);
    next();
}

// declared in parent source file
ISelftest &SelftestInstance() {
    static CSelftest ret = CSelftest();
    return ret;
}

/**
 * @file
 */
#include "i_selftest.hpp"
#include <selftest/selftest_invocation.hpp>

#include "calibration_z.hpp"
#include "printer_selftest.hpp"
#include "selftest_axis.h"
#include "selftest_axis_config.hpp"
#include "selftest_axis_interface.hpp"
#include "selftest_heater_config.hpp"
#include "selftest_heaters_interface.hpp"
#include "selftest_loadcell_config.hpp"
#include "selftest_loadcell_interface.hpp"
#include "selftest_result_type.hpp"
#include "selftest_types.hpp"

#include <Marlin/src/module/temperature.h>
#include <bsod.h>
#include <common/fanctl/fanctl.hpp>
#include <common/marlin_server.hpp>
#include <common/timing.h>
#include <config_store/store_instance.hpp>
#include <option/has_ht_hotend.h>
#if HAS_HT_HOTEND()
    #include <hotend_type.hpp>
#endif
#include <logging/log.hpp>
#include <guiconfig/wizard_config.hpp>
#include <option/has_indx_head.h>
#include <option/has_nextruder.h>

#include <cstdarg>
#include <fcntl.h>
#include <span>
#include <unistd.h>

using namespace selftest;

LOG_COMPONENT_REF(Selftest);

static constexpr auto maxFeedrates = std::to_array<feedRate_t>(DEFAULT_MAX_FEEDRATE);
static constexpr auto XYfr_table = std::to_array<float>({ HOMING_FEEDRATE_XY / 60 });
static constexpr auto Zfr_table = std::to_array<float>({ HOMING_FEEDRATE_Z / 60 });

#if HAS_INDX_HEAD()
static constexpr float x_len_min = X_MAX_LENGTH;
static constexpr float y_len_min = Y_MAX_LENGTH;
#elif HAS_NEXTRUDER()
static constexpr float x_len_min = 302;
static constexpr float y_len_min = 310;
#else
    #error "Fine-tune for the new nozzle option"
#endif

// reads data from eeprom, cannot be constexpr
const AxisConfig_t selftest::Config_XAxis = {
    .partname = "X-Axis",
    .length = X_MAX_LENGTH,
    .fr_table_fw = XYfr_table.data(),
    .fr_table_bw = XYfr_table.data(),
    .length_min = x_len_min,
    .length_max = x_len_min + X_END_GAP,
    .axis = X_AXIS,
    .steps = XYfr_table.size(),
    .movement_dir = option::has_indx ? 1 : -1,
    .park = true,
    .park_pos = 15,
};

const AxisConfig_t selftest::Config_YAxis = {
    .partname = "Y-Axis",
    .length = Y_MAX_LENGTH,
    .fr_table_fw = XYfr_table.data(),
    .fr_table_bw = XYfr_table.data(),
    .length_min = y_len_min,
    .length_max = y_len_min + Y_END_GAP,
    .axis = Y_AXIS,
    .steps = XYfr_table.size(),
    .movement_dir = 1,
    .park = true,
    .park_pos = 15,
};

static const AxisConfig_t Config_ZAxis = {
    .partname = "Z-Axis",
    .length = 338,
    .fr_table_fw = Zfr_table.data(),
    .fr_table_bw = Zfr_table.data(),
    .length_min = 333,
    .length_max = 338,
    .axis = Z_AXIS,
    .steps = Zfr_table.size(),
    .movement_dir = 1,
    .park = false,
    .park_pos = 0,
};

#include "selftest_nextruder.ipp"

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
    .heat_time_ms = 60000,
    .start_temp = 40,
    .undercool_temp = 39,
    .target_temp = 110,
    .heat_min_temp = 50,
    .heat_max_temp = 75,
    .heatbreak_min_temp = -1,
    .heatbreak_max_temp = -1,
    .heater_load_stable_ms = 200,
    .heater_full_load_min_W = 100,
    .heater_full_load_max_W = 220,
    .min_pwm_to_measure = 26
};

#if HAS_HEATERS_SELFTEST_GCODE()
namespace selftest {
HeaterConfig_t nozzle_heater_config() {
    return Config_HeaterNozzle()[0];
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
    bool IsInProgress() const final;
    bool IsAborted() const final;
    bool Start(const uint64_t test_mask, const selftest::TestData test_data) final; // parent has no clue about SelftestMask_t
    void Loop() final;
    bool Abort() final;

protected:
    void restoreAfterSelftest();
    void next() final;
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
        selftest::phaseHeaters_noz_ena(pNozzles, Config_HeaterNozzle());
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

    // restore fan behavior
    Fans::print(PhysicalToolIndex::from_raw(0)).exit_selftest_mode();
    Fans::heat_break(PhysicalToolIndex::from_raw(0)).exit_selftest_mode();

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
    case stsZAxis: { // loadcell and both X and Y must be OK to test Z
        bool loadcell_passed = m_result.get_loadcell(0) == TestResult::passed;
        bool xy_axis_passed = m_result.get_xaxis() == TestResult::passed && m_result.get_yaxis() == TestResult::passed;
        if (loadcell_passed && xy_axis_passed) {
            return; // current state can be run
        }
        m_result.set_zaxis(TestResult::unknown);
        break; // current state cannot be run
    }
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

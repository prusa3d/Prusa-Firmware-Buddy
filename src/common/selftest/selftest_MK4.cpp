// selftest.cpp

#include "printer_selftest.hpp"
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
#include "selftest_fsensor_interface.hpp"
#include "selftest_axis_interface.hpp"
#include "selftest_netstatus_interface.hpp"
#include "selftest_axis_config.hpp"
#include "selftest_heater_config.hpp"
#include "selftest_loadcell_config.hpp"
#include "selftest_fsensor_config.hpp"
#include "selftest_gears_config.hpp"
#include "selftest_gears.hpp"
#include "selftest_revise_printer_setup.hpp"
#include "calibration_z.hpp"
#include "fanctl.hpp"
#include "timing.h"
#include "selftest_result_type.hpp"
#include "selftest_types.hpp"

#include <filament_sensors_handler.hpp>
#include <config_store/store_instance.hpp>
#include "i_selftest.hpp"

using namespace selftest;

#define Z_AXIS_DO_NOT_TEST_MOVE_DOWN
#define HOMING_TIME 15000 // ~15s when X and Y axes are at opposite side to home position
static constexpr feedRate_t maxFeedrates[] = DEFAULT_MAX_FEEDRATE;

/// These speeds create major chord
/// https://en.wikipedia.org/wiki/Just_intonation

// Feedrate for measuring X/Y axis length. Used to be HOMING_FEEDRATE_XY / 60,
// but at those speeds the move sometimes, on certain printers, stalls during
// the axis measurements. Higher speeds alleviate this problem.
static const float XYfr_table[] = { 120.F };
static constexpr size_t xy_fr_table_size = sizeof(XYfr_table) / sizeof(XYfr_table[0]);
static constexpr float Zfr_table_fw[] = { maxFeedrates[Z_AXIS] }; // up
static constexpr float Zfr_table_bw[] = { HOMING_FEEDRATE_Z / 60 };
#ifdef Z_AXIS_DO_NOT_TEST_MOVE_DOWN
static constexpr size_t z_fr_tables_size = sizeof(Zfr_table_fw) / sizeof(Zfr_table_fw[0]);
#else
static constexpr size_t z_fr_tables_size = sizeof(Zfr_table_fw) / sizeof(Zfr_table_fw[0]) + sizeof(Zfr_table_bw) / sizeof(Zfr_table_bw[0]);
#endif

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
    .movement_dir = 1,
    .park = true,
    .park_pos = 125, // park in the middle to not hit stuff in enclosure (BFW-5966)
}; // MINI has movement_dir -1

const AxisConfig_t selftest::Config_YAxis = {
    .partname = "Y-Axis",
    .length = Y_MAX_LENGTH,
    .fr_table_fw = XYfr_table,
    .fr_table_bw = XYfr_table,
    .length_min = Y_MAX_LENGTH,
    .length_max = Y_MAX_LENGTH + Y_END_GAP,
    .axis = Y_AXIS,
    .steps = xy_fr_table_size,
    .movement_dir = 1,
    .park = true,
    .park_pos = 150,
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
        .getTemp = []() { return thermalManager.temp_hotend[0].celsius; },
        .setTargetTemp = [](int target_temp) { marlin_server::set_temp_to_display(target_temp, 0); thermalManager.setTargetHotend(target_temp, 0); },
        .refKp = Temperature::temp_hotend[0].pid.Kp,
        .refKi = Temperature::temp_hotend[0].pid.Ki,
        .refKd = Temperature::temp_hotend[0].pid.Kd,
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
        .pwm_100percent_equivalent_value = 127,
        .min_pwm_to_measure = 26,
        .hotend_type_temp_offsets = EnumArray<HotendType, int8_t, HotendType::_cnt> {
            { HotendType::stock, 0 },
            { HotendType::stock_with_sock, -20 },
            { HotendType::e3d_revo, -127 }, // Not supported on this printer
        },
    }
};

static constexpr HeaterConfig_t Config_HeaterBed = {
    .partname = "Bed",
    .type = heater_type_t::Bed,
    .getTemp = []() { return thermalManager.temp_bed.celsius; },
    .setTargetTemp = [](int target_temp) { thermalManager.setTargetBed(target_temp); },
    .refKp = Temperature::temp_bed.pid.Kp,
    .refKi = Temperature::temp_bed.pid.Ki,
    .refKd = Temperature::temp_bed.pid.Kd,
    .heatbreak_fan_fnc = Fans::heat_break,
    .print_fan_fnc = Fans::print,
    .heat_time_ms = 60000,
    .start_temp = 40,
    .undercool_temp = 39,
    .target_temp = 110,
    .heat_min_temp = 50,
    .heat_max_temp = 65,
    .heatbreak_min_temp = -1,
    .heatbreak_max_temp = -1,
    .heater_load_stable_ms = 200,
    // TODO RESTORE current measurement does not work
    .heater_full_load_min_W = 100, // 150,
    .heater_full_load_max_W = 220,
    .pwm_100percent_equivalent_value = 127,
    .min_pwm_to_measure = 26,
};

static constexpr LoadcellConfig_t Config_Loadcell[] = { {
    .partname = "Loadcell",
    .tool_nr = 0,
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

static constexpr std::array<const FSensorConfig_t, HOTENDS> Config_FSensor = { {
    { .extruder_id = 0 },
} };

static constexpr std::array<const FSensorConfig_t, HOTENDS> Config_FSensorMMU = { {
    { .extruder_id = 0 },
} };

static constexpr SelftestGearsConfig gears_config = { .feedrate = 8 };

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
    void phaseSelftestStart();
    void restoreAfterSelftest();
    virtual void next() override;
    void phaseDidSelftestPass();

protected:
    SelftestState_t m_State;
    SelftestMask_t m_Mask;
    selftest::IPartHandler *pXAxis;
    selftest::IPartHandler *pYAxis;
    selftest::IPartHandler *pZAxis;
    std::array<selftest::IPartHandler *, HOTENDS> pNozzles;
    selftest::IPartHandler *pBed;
    selftest::IPartHandler *pHotendSpecify;
    std::array<selftest::IPartHandler *, HOTENDS> m_pLoadcell;
    std::array<selftest::IPartHandler *, HOTENDS> pFSensor;
    selftest::IPartHandler *pGearsCalib;

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
    if (m_Mask & stmXYZAxis) {
        m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmWait_axes) | uint64_t(stmZcalib));
    }
    if (m_Mask & stmHeaters) {
        m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmWait_heaters));
    }
    if (m_Mask & stmLoadcell) {
        m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmWait_loadcell));
    }
    if (m_Mask & stmZAxis) {
        m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmMoveZup)); // if Z is calibrated, move it up
    }
    m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmSelftestStart)); // any selftest state will trigger selftest additional init
    m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmSelftestStop)); // any selftest state will trigger selftest additional deinit

    uint32_t full_test_check_mask = stmXYZAxis | stmHeaters | stmLoadcell | stmFSensor;
    if ((full_test_check_mask & test_mask) == full_test_check_mask) {
        m_Mask = (SelftestMask_t)(m_Mask | to_one_hot(stsXAxis));
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
    case stsSelftestStart:
        phaseSelftestStart();
        break;
    case stsLoadcell:
        if (selftest::phaseLoadcell(ToolMask::AllTools, m_pLoadcell, Config_Loadcell)) {
            return;
        }
        break;
    case stsWait_loadcell:
        if (phaseWait()) {
            return;
        }
        break;
    case stsZcalib: {
        // calib_Z(true) will move it back after calibration
        // in case Z_SAFE_HOMING is disabled and Z is first axis to be homed
        // it would be better to call calib_Z(false)
        // and just measure Z length on the way down
        calib_Z(true);

        // Store Z aligned
        m_result = config_store().selftest_result.get();
        m_result.zalign = TestResult_Passed;
        config_store().selftest_result.set(m_result);
        break;
    }
    case stsXAxis: {
        if (selftest::phaseAxis(pXAxis, Config_XAxis, Separate::no)) {
            return;
        }
        break;
    }
    case stsYAxis: {
        if (selftest::phaseAxis(pYAxis, Config_YAxis, Separate::no)) {
            return;
        }
        break;
    }
    case stsZAxis: {
        if (selftest::phaseAxis(pZAxis, Config_ZAxis, Separate::no)) {
            return;
        }
        break;
    }
    case stsMoveZup:
#ifndef Z_AXIS_DO_NOT_TEST_MOVE_DOWN
        queue.enqueue_one_now("G0 Z100"); // move to 100 mm
#endif
        break;
    case stsWait_axes:
        if (phaseWait()) {
            return;
        }
        break;

    case stsReviseSetupAfterAxes:
        m_result = config_store().selftest_result.get();
        if (m_result.xaxis == TestResult_Failed || m_result.yaxis == TestResult_Failed) {
            switch (phase_revise_printer_setup()) {

            case RevisePrinterSetupResult::running:
                return;

            case RevisePrinterSetupResult::do_not_retry:
                break;

            case RevisePrinterSetupResult::retry:
                m_result.xaxis = TestResult_Unknown;
                m_result.yaxis = TestResult_Unknown;
                m_result.zaxis = TestResult_Unknown;
                config_store().selftest_result.set(m_result);

                m_State = stsXAxis;
                return;
            }
        }
        break;

    case stsHeaters_noz_ena:
        selftest::phaseHeaters_noz_ena(pNozzles, Config_HeaterNozzle);
        break;
    case stsHeaters_bed_ena: {
        selftest::phaseHeaters_bed_ena(pBed, Config_HeaterBed);
        break;
    }
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

        if (m_result.bed == TestResult_Failed) {
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

        if (m_result.tools[0].nozzle == TestResult_Failed) {
            switch (phase_revise_printer_setup()) {

            case RevisePrinterSetupResult::running:
                return;

            case RevisePrinterSetupResult::do_not_retry:
                break;

            case RevisePrinterSetupResult::retry:
                m_State = stsHeaters_noz_ena;
                return;
            }
        }
        break;

    case stsFSensor_calibration:
        if (selftest::phaseFSensor(ToolMask::AllTools, pFSensor, Config_FSensor)) {
            return;
        }
        break;
    case stsFSensor_flip_mmu_at_the_end:
#if HAS_MMU2()
        // enable/disable the MMU according to the MMU Rework toggle. Used from
        // the menus when we need to calibrate the FS before enabling/disabling
        // the rework or the MMU itself.

        // We don't check the result here. If FS is calibrated and enabled
        // at the end of the selftest, MMU will be enabled, otherwise not.
        marlin_server::enqueue_gcode(config_store().is_mmu_rework.get() ? "M709 S1" : "M709 S0");
#endif

        break;
    case stsGears:
        if (selftest::phase_gears(pGearsCalib, gears_config)) {
            return;
        }
        break;
    case stsSelftestStop:
        restoreAfterSelftest();
        break;
    case stsNet_status:
        selftest::phaseNetStatus();
        break;
    case stsDidSelftestPass:
        phaseDidSelftestPass();
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

    // dont run wizard again
    if (SelftestResult_Passed_All(m_result)) {
        auto &store = config_store();
        auto transaction = store.get_backend().transaction_guard();
        store.run_selftest.set(false); // clear selftest flag
    }
}

bool CSelftest::Abort() {
    if (!IsInProgress()) {
        return false;
    }
    abort_part((selftest::IPartHandler **)&pXAxis);
    abort_part((selftest::IPartHandler **)&pYAxis);
    abort_part((selftest::IPartHandler **)&pZAxis);
    for (auto &pNozzle : pNozzles) {
        abort_part(&pNozzle);
    }
    abort_part(&pBed);
    for (auto &loadcell : m_pLoadcell) {
        abort_part(&loadcell);
    }
    abort_part((selftest::IPartHandler **)&pFSensor);
    abort_part(&pGearsCalib);

    m_State = stsAborted;

    phaseFinish();
    return true;
}

void CSelftest::phaseSelftestStart() {
    if (m_Mask & stmHeaters) {
        // set bed to 35°C
        // heater test will start after temperature pass tru 40°C (we dont want to entire bed and sheet to be tempered at it)
        // so don't set 40°C, it could also trigger cooldown in case temperature is or similar 40.1°C
        thermalManager.setTargetBed(35);
        // no need to preheat nozzle, it heats up much faster than bed
        thermalManager.setTargetHotend(0, 0);
        marlin_server::set_temp_to_display(0, 0);
    }

    m_result = config_store().selftest_result.get(); // read previous result
    if (m_Mask & stmXAxis) {
        m_result.xaxis = TestResult_Unknown;
    }
    if (m_Mask & stmYAxis) {
        m_result.yaxis = TestResult_Unknown;
    }
    if (m_Mask & stmZAxis) {
        m_result.zaxis = TestResult_Unknown;
    }
    if (m_Mask & stmZcalib) {
        m_result.zalign = TestResult_Unknown;
    }
    if (m_Mask & stmHeaters) {
        m_result.tools[0].nozzle = TestResult_Unknown;
        m_result.bed = TestResult_Unknown;
    }
    config_store().selftest_result.set(m_result); // reset status for all selftest parts in eeprom
}

void CSelftest::restoreAfterSelftest() {
    // disable heater target values - thermalManager.disable_all_heaters does not do that
    thermalManager.setTargetBed(0);
    thermalManager.setTargetHotend(0, 0);
    marlin_server::set_temp_to_display(0, 0);

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
    case stsZcalib:
    case stsXAxis:
    case stsYAxis: // Y is not skipped even if X fails
        if (TestResult_t(m_result.loadcell) == TestResult_t::Passed)
            return; // current state can be run
        break;      // current state cannot be run
#endif
    case stsZAxis: // loadcell and both X and Y must be OK to test Z
        if (m_result.tools[0].loadcell == TestResult_Passed && m_result.xaxis == TestResult_Passed && m_result.yaxis == TestResult_Passed) {
            return; // current state can be run
        }
        break; // current state cannot be run
    case stsMoveZup: // Z must be OK, if axis are not homed, it could be stacked at the top and generate noise, but the way states are generated from mask should prevent it
        if (m_result.zaxis == TestResult_Passed) {
            return; // current state can be run
        }
        break; // current state cannot be run
    default:
        return; // current state can be run
    }

    // current state cannot be run
    // call recursively: it is fine, this function is tiny and there will be few iterations
    next();
}

// declared in parent source file
ISelftest &SelftestInstance() {
    static CSelftest ret = CSelftest();
    return ret;
}

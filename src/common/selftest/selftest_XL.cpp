// selftest.cpp

#include "printer_selftest.hpp"
#include <fcntl.h>
#include <unistd.h>
#include "selftest_fan.h"
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
#include "selftest_fans_type.hpp"
#include "selftest_axis_type.hpp"
#include "selftest_heaters_type.hpp"
#include "selftest_heaters_interface.hpp"
#include "selftest_fans_interface.hpp"
#include "selftest_loadcell_interface.hpp"
#include "selftest_fsensor_interface.hpp"
#include "selftest_axis_interface.hpp"
#include "selftest_netstatus_interface.hpp"
#include "selftest_dock_interface.hpp"
#include "selftest_tool_offsets_interface.hpp"
#include "selftest_axis_config.hpp"
#include "selftest_heater_config.hpp"
#include "selftest_loadcell_config.hpp"
#include "selftest_fsensor_config.hpp"
#include "calibration_z.hpp"
#include "fanctl.hpp"
#include "timing.h"
#include "selftest_result_type.hpp"
#include <config_store/store_instance.hpp>
#include "common/selftest/selftest_data.hpp"
#include "i_selftest.hpp"
#include "i_selftest_part.hpp"
#include "selftest_result_type.hpp"
#include <option/has_switched_fan_test.h>

using namespace selftest;

#define HOMING_TIME 15000 // ~15s when X and Y axes are at opposite side to home position
static constexpr feedRate_t maxFeedrates[] = DEFAULT_MAX_FEEDRATE;

static constexpr const char *_suffix[] = { "_fan", "_xyz", "_heaters" };
/// These speeds create major chord
/// https://en.wikipedia.org/wiki/Just_intonation

static constexpr float XYfr_table[] = { HOMING_FEEDRATE_XY / 60 };
static constexpr size_t xy_fr_table_size = sizeof(XYfr_table) / sizeof(XYfr_table[0]);
static constexpr float Zfr_table_fw[] = { maxFeedrates[Z_AXIS] }; // up
static constexpr float Zfr_table_bw[] = { HOMING_FEEDRATE_Z / 60 };
static constexpr size_t z_fr_tables_size = sizeof(Zfr_table_fw) / sizeof(Zfr_table_fw[0]);

static consteval SelftestFansConfig make_fan_config(uint8_t index) {
    return {
        .tool_nr = index,
        .print_fan = {
            ///@note Datasheet says 5900 +-10%, but that is without any fan shroud.
            ///  Blocked fan increases its RPMs over 7000.
            ///  With XL shroud the values can be 6200 - 6600 depending on fan shroud version.
            .rpm_min = 5300,
            .rpm_max = 6799,
        },
        .heatbreak_fan = {
            .rpm_min = 6800,
            .rpm_max = 8700,
        },
    };
}

#if HAS_SWITCHED_FAN_TEST()
static_assert(make_fan_config(0).print_fan.rpm_max < make_fan_config(0).heatbreak_fan.rpm_min, "These cannot overlap for switched fan detection.");
#endif /* HAS_SWITCHED_FAN_TEST() */

static constexpr SelftestFansConfig fans_configs[] = {
    make_fan_config(0),
    make_fan_config(1),
    make_fan_config(2),
    make_fan_config(3),
    make_fan_config(4),
    make_fan_config(5),
};
static_assert(std::size(fans_configs) == HOTENDS);

// reads data from eeprom, cannot be constexpr
//  FIXME: remove fixed lengths once the printer specs are finalized
const AxisConfig_t selftest::Config_XAxis = {
    .partname = "X-Axis",
    .length = X_BED_SIZE - CSelftestPart_Axis::EXTRA_LEN_MM,
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
    .length = Y_BED_SIZE - CSelftestPart_Axis::EXTRA_LEN_MM,
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
    return {
        .partname = name,
        .type = heater_type_t::Nozzle,
        .tool_nr = index,
        .getTemp = []() { return thermalManager.temp_hotend[index].celsius; },
        .setTargetTemp = [](int target_temp) { marlin_server::set_temp_to_display(target_temp, index); thermalManager.setTargetHotend(target_temp, index); },
        .refKp = Temperature::temp_hotend[index].pid.Kp,
        .refKi = Temperature::temp_hotend[index].pid.Ki,
        .refKd = Temperature::temp_hotend[index].pid.Kd,
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
        .pwm_100percent_equivalent_value = 127,
        .min_pwm_to_measure = 127 // Check power only when fully on
    };
}

static constexpr HeaterConfig_t Config_HeaterNozzle[] = {
    make_nozzle_config<0>("Nozzle 1"),
    make_nozzle_config<1>("Nozzle 2"),
    make_nozzle_config<2>("Nozzle 3"),
    make_nozzle_config<3>("Nozzle 4"),
    make_nozzle_config<4>("Nozzle 5")
};

static float bed_fake_pid_constant = 0.0; // No PID change support for XL's bed (for now)

static constexpr HeaterConfig_t Config_HeaterBed = {
    .partname = "Bed",
    .type = heater_type_t::Bed,
    .tool_nr = 0,
    .getTemp = []() { return thermalManager.temp_bed.celsius; },
    .setTargetTemp = [](int target_temp) { thermalManager.setTargetBed(target_temp); },
    .refKp = bed_fake_pid_constant,
    .refKi = bed_fake_pid_constant,
    .refKd = bed_fake_pid_constant,
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
    .pwm_100percent_equivalent_value = 127,
    .min_pwm_to_measure = 26
};

static consteval LoadcellConfig_t make_loadcell_config(uint8_t index, const char *name) {
    return {
        .partname = name,
        .tool_nr = index,
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
    make_loadcell_config(0, "Loadcell 1"),
    make_loadcell_config(1, "Loadcell 2"),
    make_loadcell_config(2, "Loadcell 3"),
    make_loadcell_config(3, "Loadcell 4"),
    make_loadcell_config(4, "Loadcell 5")
};

static constexpr std::array<const FSensorConfig_t, HOTENDS> Config_FSensor = { {
    { .extruder_id = 0 },
    { .extruder_id = 1 },
    { .extruder_id = 2 },
    { .extruder_id = 3 },
    { .extruder_id = 4 },
    { .extruder_id = 5 },
} };

static consteval DockConfig_t make_dock_config(uint8_t index) {
    return {
        .dock_id = index,
        .z_extra_pos = 100,
        .z_extra_pos_fr = maxFeedrates[Z_AXIS],
    };
}

static constexpr std::array<const DockConfig_t, HOTENDS> Config_Docks = { { make_dock_config(0),
    make_dock_config(1),
    make_dock_config(2),
    make_dock_config(3),
    make_dock_config(4),
    make_dock_config(5) } };

static constexpr ToolOffsetsConfig_t Config_ToolOffsets = {};

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
    void phaseShowResult();
    void phaseDidSelftestPass();

protected:
    SelftestState_t m_State;
    SelftestMask_t m_Mask;
    ToolMask tool_mask = ToolMask::AllTools;
    std::array<selftest::IPartHandler *, HOTENDS> pFans;
    selftest::IPartHandler *pXAxis;
    selftest::IPartHandler *pYAxis;
    selftest::IPartHandler *pZAxis;
    std::array<selftest::IPartHandler *, HOTENDS> pNozzles;
    selftest::IPartHandler *pBed;
    std::array<selftest::IPartHandler *, HOTENDS> m_pLoadcell;
    std::array<selftest::IPartHandler *, HOTENDS> pDocks;
    selftest::IPartHandler *pToolOffsets;
    std::array<selftest::IPartHandler *, HOTENDS> pFSensor;
    selftest::IPartHandler *pPhaseStepping;

    SelftestResult m_result;
};

CSelftest::CSelftest()
    : m_State(stsIdle)
    , m_Mask(stmNone)
    , tool_mask(ToolMask::AllTools)
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

bool CSelftest::Start(const uint64_t test_mask, const selftest::TestData test_data) {
    m_result = config_store().selftest_result.get();
    m_Mask = SelftestMask_t(test_mask);
    if (m_Mask & stmFans) {
        m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmWait_fans));
    }
    if (m_Mask & (stmXAxis | stmYAxis | stmZAxis)) {
        m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmWait_axes));
        if (m_result.zaxis != TestResult_Passed) {
            m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmEnsureZAway)); // Ensure Z is away enough if Z not calibrated yet
        }
    }
    if (m_Mask & stmHeaters) {
        m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmWait_heaters));
    }
    if (m_Mask & stmLoadcell) {
        m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmWait_loadcell));
    }
    m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmSelftestStart)); // any selftest state will trigger selftest additional init
    m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmSelftestStop)); // any selftest state will trigger selftest additional deinit

    if (std::holds_alternative<ToolMask>(test_data)) {
        this->tool_mask = std::get<ToolMask>(test_data);
    } else {
        this->tool_mask = ToolMask::AllTools;
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
    case stsSelftestStart:
        phaseSelftestStart();
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
    case stsFans:
        if (selftest::phaseFans(pFans, fans_configs)) {
            return;
        }
        break;
    case stsWait_fans:
        if (phaseWait()) {
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

        // Store Z aligned
        m_result = config_store().selftest_result.get();
        m_result.zalign = TestResult_Passed;
        config_store().selftest_result.set(m_result);
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
        break;

    case stsFSensor_calibration:
        if ((ret = selftest::phaseFSensor(tool_mask, pFSensor, Config_FSensor))) {
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

    if (ret.WasSkipped()) {
        Abort();
    } else {
        next();
    }
}

void CSelftest::phaseShowResult() {
    m_result = config_store().selftest_result.get();
    marlin_server::fsm_change(PhasesSelftest::Result, FsmSelftestResult().Serialize());
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
    for (auto &pFan : pFans) {
        abort_part(&pFan);
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
    abort_part((selftest::IPartHandler **)&pFSensor);
    for (auto &dock : pDocks) {
        abort_part(&dock);
    }
    abort_part(&pToolOffsets);
    for (auto &fsensor : pFSensor) {
        abort_part(&fsensor);
    }
    m_State = stsAborted;

    phaseFinish();
    return true;
}

void CSelftest::phaseSelftestStart() {
    if (m_Mask & to_one_hot(stsHeaters_bed_ena)) {
        // set bed to 35°C
        // heater test will start after temperature pass tru 40°C (we dont want to entire bed and sheet to be tempered at it)
        // so don't set 40°C, it could also trigger cooldown in case temperature is or similar 40.1°C
        thermalManager.setTargetBed(35);
    }

    if (m_Mask & to_one_hot(stsHeaters_noz_ena)) {
        // no need to preheat nozzle, it heats up much faster than bed
        HOTEND_LOOP() {
            thermalManager.setTargetHotend(0, e);
            marlin_server::set_temp_to_display(0, e);
        }
    }

    m_result = config_store().selftest_result.get(); // read previous result
    if (m_Mask & stmFans) {
        HOTEND_LOOP() {
            m_result.tools[e].reset_fan_tests();
        }
    }
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
    if (m_Mask & to_one_hot(stsHeaters_bed_ena)) {
        m_result.bed = TestResult_Unknown;
    }
    if (m_Mask & to_one_hot(stsHeaters_noz_ena)) {
        HOTEND_LOOP() {
            m_result.tools[e].nozzle = TestResult_Unknown;
        }
    }
    config_store().selftest_result.set(m_result); // reset status for all selftest parts in eeprom
}

void CSelftest::restoreAfterSelftest() {
    // disable heater target values - thermalManager.disable_all_heaters does not do that
    thermalManager.setTargetBed(0);
    HOTEND_LOOP() {
        thermalManager.setTargetHotend(0, 0);
        marlin_server::set_temp_to_display(0, 0);
    }

    // restore fan behavior
    Fans::print(0).exitSelftestMode();
    Fans::heat_break(0).exitSelftestMode();

    thermalManager.disable_all_heaters();

    // Keep steppers enabled, so the next test doesn't have to rehome
    // Reset stepper timeout
    marlin_server::enqueue_gcode_printf("M18 S%d", DEFAULT_STEPPER_DEACTIVE_TIME);
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

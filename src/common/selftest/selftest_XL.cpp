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
#include "app.h"
#include "otp.hpp"
#include "hwio.h"
#include "marlin_server.hpp"
#include "wizard_config.hpp"
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

using namespace selftest;

#define Z_AXIS_DO_NOT_TEST_MOVE_DOWN
#define HOMING_TIME 15000 // ~15s when X and Y axes are at opposite side to home position
static constexpr feedRate_t maxFeedrates[] = DEFAULT_MAX_FEEDRATE;

static constexpr const char *_suffix[] = { "_fan", "_xyz", "_heaters" };
/// These speeds create major chord
/// https://en.wikipedia.org/wiki/Just_intonation

static constexpr float XYfr_table[] = { HOMING_FEEDRATE_XY / 60 };
static constexpr size_t xy_fr_table_size = sizeof(XYfr_table) / sizeof(XYfr_table[0]);
static constexpr float Zfr_table_fw[] = { maxFeedrates[Z_AXIS] }; // up
static constexpr float Zfr_table_bw[] = { HOMING_FEEDRATE_Z / 60 };
#ifdef Z_AXIS_DO_NOT_TEST_MOVE_DOWN
static constexpr size_t z_fr_tables_size = sizeof(Zfr_table_fw) / sizeof(Zfr_table_fw[0]);
#else
static constexpr size_t z_fr_tables_size = sizeof(Zfr_table_fw) / sizeof(Zfr_table_fw[0]) + sizeof(Zfr_table_bw) / sizeof(Zfr_table_bw[0]);
#endif

static constexpr std::array<uint16_t, 5> print_fan_min_rpm_table = { 10, 10, 10, 10, 10 };
static constexpr std::array<uint16_t, 5> print_fan_max_rpm_table = { 10000, 10000, 10000, 10000, 10000 };
static constexpr std::array<uint16_t, 5> heatbreak_fan_min_rpm_table = { 10, 10, 10, 10, 10 };
static constexpr std::array<uint16_t, 5> heatbreak_fan_max_rpm_table = { 10000, 10000, 10000, 10000, 10000 };

// clang-format off
// We test two steps, at 20% (just to check if the fans spin at low PWM) and at
// 100%, where we also check the rpm range
static constexpr SelftestFansConfig fans_configs[] = {
    {
        .tool_nr = 0,
        .print_fan = {
            .pwm_start = 51,
            .pwm_step = 204,
            .rpm_min_table = { 10, 5300 },
            .rpm_max_table = { 10000, 6500 },
            .fanctl_fnc = Fans::print
        },
        .heatbreak_fan = {
            .pwm_start = 51,
            .pwm_step = 204,
            .rpm_min_table = { 10, 6800 },
            .rpm_max_table = { 10000, 8700 },
            .fanctl_fnc = Fans::heat_break
        }
    },
    {
        .tool_nr = 1,
        .print_fan = {
            .pwm_start = 51,
            .pwm_step = 204,
            .rpm_min_table = { 10, 5300 },
            .rpm_max_table = { 10000, 6500 },
            .fanctl_fnc = Fans::print
        },
        .heatbreak_fan = {
            .pwm_start = 51,
            .pwm_step = 204,
            .rpm_min_table = { 10, 6800 },
            .rpm_max_table = { 10000, 8700 },
            .fanctl_fnc = Fans::heat_break
        }
    },
    {
        .tool_nr = 2,
        .print_fan = {
            .pwm_start = 51,
            .pwm_step = 204,
            .rpm_min_table = { 10, 5300 },
            .rpm_max_table = { 10000, 6500 },
            .fanctl_fnc = Fans::print
        },
        .heatbreak_fan = {
            .pwm_start = 51,
            .pwm_step = 204,
            .rpm_min_table = { 10, 6800 },
            .rpm_max_table = { 10000, 8700 },
            .fanctl_fnc = Fans::heat_break
        }
    },
    {
        .tool_nr = 3,
        .print_fan = {
            .pwm_start = 51,
            .pwm_step = 204,
            .rpm_min_table = { 10, 5300 },
            .rpm_max_table = { 10000, 6500 },
            .fanctl_fnc = Fans::print
        },
        .heatbreak_fan = {
            .pwm_start = 51,
            .pwm_step = 204,
            .rpm_min_table = { 10, 6800 },
            .rpm_max_table = { 10000, 8700 },
            .fanctl_fnc = Fans::heat_break
        }
    },
    {
        .tool_nr = 4,
        .print_fan = {
            .pwm_start = 51,
            .pwm_step = 204,
            .rpm_min_table = { 10, 5300 },
            .rpm_max_table = { 10000, 6500 },
            .fanctl_fnc = Fans::print
        },
        .heatbreak_fan = {
            .pwm_start = 51,
            .pwm_step = 204,
            .rpm_min_table = { 10, 6800 },
            .rpm_max_table = { 10000, 8700 },
            .fanctl_fnc = Fans::heat_break
        }
    },
    {
        .tool_nr = 5,
        .print_fan = {
            .pwm_start = 51,
            .pwm_step = 204,
            .rpm_min_table = { 10, 5300 },
            .rpm_max_table = { 10000, 6500 },
            .fanctl_fnc = Fans::print
        },
        .heatbreak_fan = {
            .pwm_start = 51,
            .pwm_step = 204,
            .rpm_min_table = { 10, 6800 },
            .rpm_max_table = { 10000, 8700 },
            .fanctl_fnc = Fans::heat_break
        }
    },
};
// clang-format on

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

static constexpr HeaterConfig_t Config_HeaterNozzle[] = {
    { .partname = "Nozzle 1",
        .type = heater_type_t::Nozzle,
        .tool_nr = 0,
        .getTemp = []() { return thermalManager.temp_hotend[0].celsius; },
        .setTargetTemp = [](int target_temp) { marlin_server::set_temp_to_display(target_temp, 0); thermalManager.setTargetHotend(target_temp, 0); },
        .refKp = Temperature::temp_hotend[0].pid.Kp,
        .refKi = Temperature::temp_hotend[0].pid.Ki,
        .refKd = Temperature::temp_hotend[0].pid.Kd,
        .heatbreak_fan_fnc = Fans::heat_break,
        .print_fan_fnc = Fans::print,
        .heat_time_ms = 70000,
        .start_temp = 40,
        .undercool_temp = 37,
        .target_temp = 290,
        .heat_min_temp = 180,
        .heat_max_temp = 230,
        .heatbreak_min_temp = 10,
        .heatbreak_max_temp = 45,
        .heater_load_stable_ms = 200,
        .heater_full_load_min_W = 20,
        .heater_full_load_max_W = 50,
        .pwm_100percent_equivalent_value = 255,
        .min_pwm_to_measure = 26 },
    { .partname = "Nozzle 2",
        .type = heater_type_t::Nozzle,
        .tool_nr = 1,
        .getTemp = []() { return thermalManager.temp_hotend[1].celsius; },
        .setTargetTemp = [](int target_temp) { marlin_server::set_temp_to_display(target_temp, 1); thermalManager.setTargetHotend(target_temp, 1); },
        .refKp = Temperature::temp_hotend[1].pid.Kp,
        .refKi = Temperature::temp_hotend[1].pid.Ki,
        .refKd = Temperature::temp_hotend[1].pid.Kd,
        .heatbreak_fan_fnc = Fans::heat_break,
        .print_fan_fnc = Fans::print,
        .heat_time_ms = 70000,
        .start_temp = 40,
        .undercool_temp = 37,
        .target_temp = 290,
        .heat_min_temp = 180,
        .heat_max_temp = 230,
        .heatbreak_min_temp = 10,
        .heatbreak_max_temp = 45,
        .heater_load_stable_ms = 200,
        .heater_full_load_min_W = 20,
        .heater_full_load_max_W = 50,
        .pwm_100percent_equivalent_value = 255,
        .min_pwm_to_measure = 26 },
    { .partname = "Nozzle 3",
        .type = heater_type_t::Nozzle,
        .tool_nr = 2,
        .getTemp = []() { return thermalManager.temp_hotend[2].celsius; },
        .setTargetTemp = [](int target_temp) { marlin_server::set_temp_to_display(target_temp, 2); thermalManager.setTargetHotend(target_temp, 2); },
        .refKp = Temperature::temp_hotend[2].pid.Kp,
        .refKi = Temperature::temp_hotend[2].pid.Ki,
        .refKd = Temperature::temp_hotend[2].pid.Kd,
        .heatbreak_fan_fnc = Fans::heat_break,
        .print_fan_fnc = Fans::print,
        .heat_time_ms = 70000,
        .start_temp = 40,
        .undercool_temp = 37,
        .target_temp = 290,
        .heat_min_temp = 180,
        .heat_max_temp = 230,
        .heatbreak_min_temp = 10,
        .heatbreak_max_temp = 45,
        .heater_load_stable_ms = 200,
        .heater_full_load_min_W = 20,
        .heater_full_load_max_W = 50,
        .pwm_100percent_equivalent_value = 255,
        .min_pwm_to_measure = 26 },
    { .partname = "Nozzle 4",
        .type = heater_type_t::Nozzle,
        .tool_nr = 3,
        .getTemp = []() { return thermalManager.temp_hotend[3].celsius; },
        .setTargetTemp = [](int target_temp) { marlin_server::set_temp_to_display(target_temp, 3); thermalManager.setTargetHotend(target_temp, 3); },
        .refKp = Temperature::temp_hotend[3].pid.Kp,
        .refKi = Temperature::temp_hotend[3].pid.Ki,
        .refKd = Temperature::temp_hotend[3].pid.Kd,
        .heatbreak_fan_fnc = Fans::heat_break,
        .print_fan_fnc = Fans::print,
        .heat_time_ms = 70000,
        .start_temp = 40,
        .undercool_temp = 37,
        .target_temp = 290,
        .heat_min_temp = 180,
        .heat_max_temp = 230,
        .heatbreak_min_temp = 10,
        .heatbreak_max_temp = 45,
        .heater_load_stable_ms = 200,
        .heater_full_load_min_W = 20,
        .heater_full_load_max_W = 50,
        .pwm_100percent_equivalent_value = 255,
        .min_pwm_to_measure = 26 },
    { .partname = "Nozzle 5",
        .type = heater_type_t::Nozzle,
        .tool_nr = 4,
        .getTemp = []() { return thermalManager.temp_hotend[4].celsius; },
        .setTargetTemp = [](int target_temp) { marlin_server::set_temp_to_display(target_temp, 4); thermalManager.setTargetHotend(target_temp, 4); },
        .refKp = Temperature::temp_hotend[4].pid.Kp,
        .refKi = Temperature::temp_hotend[4].pid.Ki,
        .refKd = Temperature::temp_hotend[4].pid.Kd,
        .heatbreak_fan_fnc = Fans::heat_break,
        .print_fan_fnc = Fans::print,
        .heat_time_ms = 70000,
        .start_temp = 40,
        .undercool_temp = 37,
        .target_temp = 290,
        .heat_min_temp = 180,
        .heat_max_temp = 230,
        .heatbreak_min_temp = 10,
        .heatbreak_max_temp = 45,
        .heater_load_stable_ms = 200,
        .heater_full_load_min_W = 20,
        .heater_full_load_max_W = 50,
        .pwm_100percent_equivalent_value = 255,
        .min_pwm_to_measure = 26 },
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
    .heater_load_stable_ms = 200,
    // TODO RESTORE current measurement does not work
    .heater_full_load_min_W = 100, // 150,
    .heater_full_load_max_W = 220,
    .pwm_100percent_equivalent_value = 127,
    .min_pwm_to_measure = 26
};

static constexpr LoadcellConfig_t Config_Loadcell[] = {
    { .partname = "Loadcell 1",
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
        .max_validation_time = 1000 },
    { .partname = "Loadcell 2",
        .tool_nr = 1,
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
        .max_validation_time = 1000 },
    { .partname = "Loadcell 3",
        .tool_nr = 2,
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
        .max_validation_time = 1000 },
    { .partname = "Loadcell 4",
        .tool_nr = 3,
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
        .max_validation_time = 1000 },
    { .partname = "Loadcell 5",
        .tool_nr = 4,
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
        .max_validation_time = 1000 },
};

static constexpr std::array<const FSensorConfig_t, HOTENDS> Config_FSensor = { {
    { .extruder_id = 0 },
    { .extruder_id = 1 },
    { .extruder_id = 2 },
    { .extruder_id = 3 },
    { .extruder_id = 4 },
    { .extruder_id = 5 },
} };

static constexpr std::array<const DockConfig_t, HOTENDS> Config_Docks = { {
    {
        .dock_id = 0,
        .z_extra_pos = 100,
        .z_extra_pos_fr = maxFeedrates[Z_AXIS],
    },
    {
        .dock_id = 1,
        .z_extra_pos = 100,
        .z_extra_pos_fr = maxFeedrates[Z_AXIS],
    },
    {
        .dock_id = 2,
        .z_extra_pos = 100,
        .z_extra_pos_fr = maxFeedrates[Z_AXIS],
    },
    {
        .dock_id = 3,
        .z_extra_pos = 100,
        .z_extra_pos_fr = maxFeedrates[Z_AXIS],
    },
    {
        .dock_id = 4,
        .z_extra_pos = 100,
        .z_extra_pos_fr = maxFeedrates[Z_AXIS],
    },
    {
        .dock_id = 5,
        .z_extra_pos = 100,
        .z_extra_pos_fr = maxFeedrates[Z_AXIS],
    },
} };

static constexpr ToolOffsetsConfig_t Config_ToolOffsets = {};

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

bool CSelftest::Start(const uint64_t test_mask, const uint8_t tool_mask) {
    m_result = config_store().selftest_result.get();
    m_Mask = SelftestMask_t(test_mask);
    if (m_Mask & stmFans)
        m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmWait_fans));
    if (m_Mask & stmXYZAxis) {
        m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmWait_axes));
        if (m_result.zaxis != TestResult_Passed) {
            m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmEnsureZAway)); // Ensure Z is away enough if Z not calibrated yet
        }
    }
    if (m_Mask & stmHeaters)
        m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmWait_heaters));
    if (m_Mask & stmLoadcell)
        m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmWait_loadcell));
    if (m_Mask & stmZAxis)
        m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmMoveZup));       // if Z is calibrated, move it up
    if (m_Mask & stmFullSelftest)
        m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmSelftestStart)); // any selftest state will trigger selftest additional init
    if (m_Mask & stmFullSelftest)
        m_Mask = (SelftestMask_t)(m_Mask | uint64_t(stmSelftestStop));  // any selftest state will trigger selftest additional deinit
    this->tool_mask = static_cast<ToolMask>(tool_mask);

    // dont show message about footer and do not wait response
    m_Mask = (SelftestMask_t)(m_Mask & (~(uint64_t(1) << stsPrologueInfo)));
    m_Mask = (SelftestMask_t)(m_Mask & (~(uint64_t(1) << stsPrologueInfo_wait_user)));

    m_State = stsStart;
    return true;
}

void CSelftest::Loop() {
    uint32_t time = ticks_ms();
    if ((time - m_Time) < SELFTEST_LOOP_PERIODE)
        return;
    m_Time = time;

    selftest::TestReturn ret = true;
    switch (m_State) {
    case stsIdle:
        return;
    case stsStart:
        phaseStart();
        break;
    case stsPrologueAskRun:
        FSM_CHANGE__LOGGING(Selftest, PhasesSelftest::WizardPrologue_ask_run);
        break;
    case stsPrologueAskRun_wait_user:
        if (phaseWaitUser(PhasesSelftest::WizardPrologue_ask_run))
            return;
        break;
    case stsSelftestStart:
        phaseSelftestStart();
        break;
    case stsPrologueInfo:
        FSM_CHANGE__LOGGING(Selftest, PhasesSelftest::WizardPrologue_info);
        break;
    case stsPrologueInfo_wait_user:
        if (phaseWaitUser(PhasesSelftest::WizardPrologue_info))
            return;
        break;
    case stsPrologueInfoDetailed:
        FSM_CHANGE__LOGGING(Selftest, PhasesSelftest::WizardPrologue_info_detailed);
        break;
    case stsPrologueInfoDetailed_wait_user:
        if (phaseWaitUser(PhasesSelftest::WizardPrologue_info_detailed))
            return;
        break;
    case stsDocks:
        if (prusa_toolchanger.is_toolchanger_enabled() && (ret = selftest::phaseDocks(tool_mask, pDocks, Config_Docks)))
            return;
        break;
    case stsToolOffsets:
        if ((ret = selftest::phaseToolOffsets(tool_mask, pToolOffsets, Config_ToolOffsets)))
            return;
        break;
    case stsFans:
        if (selftest::phaseFans(pFans, fans_configs))
            return;
        break;
    case stsWait_fans:
        if (phaseWait())
            return;
        break;
    case stsLoadcell:
        if ((ret = selftest::phaseLoadcell(tool_mask, m_pLoadcell, Config_Loadcell)))
            return;
        break;
    case stsWait_loadcell:
        if (phaseWait())
            return;
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
        if (selftest::phaseAxis(pXAxis, Config_XAxis, true))
            return;
        break;
    }
    case stsYAxis: {
        if (selftest::phaseAxis(pYAxis, Config_YAxis, true))
            return;
        break;
    }
    case stsZAxis: {
        if (selftest::phaseAxis(pZAxis, Config_ZAxis, true))
            return;
        break;
    }
    case stsMoveZup:
#ifndef Z_AXIS_DO_NOT_TEST_MOVE_DOWN
        queue.enqueue_one_now("G0 Z100"); // move to 100 mm
#endif
        break;
    case stsWait_axes:
        if (phaseWait())
            return;
        break;
    case stsHeaters_noz_ena:
        selftest::phaseHeaters_noz_ena(pNozzles, Config_HeaterNozzle);
        break;
    case stsHeaters_bed_ena:
        selftest::phaseHeaters_bed_ena(pBed, Config_HeaterBed);
        break;
    case stsHeaters:
        if (selftest::phaseHeaters(pNozzles, &pBed))
            return;
        break;
    case stsWait_heaters:
        if (phaseWait())
            return;
        break;
    case stsFSensor_calibration:
        if ((ret = selftest::phaseFSensor(tool_mask, pFSensor, Config_FSensor)))
            return;
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
    case stsEpilogue_nok:
        if (SelftestResult_Failed(m_result)) {
            FSM_CHANGE__LOGGING(Selftest, PhasesSelftest::WizardEpilogue_nok);
        }
        break;
    case stsEpilogue_nok_wait_user:
        if (SelftestResult_Failed(m_result)) {
            if (phaseWaitUser(PhasesSelftest::WizardEpilogue_nok))
                return;
        }
        break;
    case stsShow_result:
        phaseShowResult();
        break;
    case stsResult_wait_user:
        if (phaseWaitUser(PhasesSelftest::Result))
            return;
        break;
    case stsEpilogue_ok:
        if (SelftestResult_Passed(m_result)) {
            FSM_CHANGE__LOGGING(Selftest, PhasesSelftest::WizardEpilogue_ok);
        }
        break;
    case stsEpilogue_ok_wait_user:
        if (SelftestResult_Passed(m_result)) {
            if (phaseWaitUser(PhasesSelftest::WizardEpilogue_ok))
                return;
        }
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
    FSM_CHANGE_WITH_DATA__LOGGING(Selftest, PhasesSelftest::Result, FsmSelftestResult().Serialize());
}

void CSelftest::phaseDidSelftestPass() {
    m_result = config_store().selftest_result.get();
    SelftestResult_Log(m_result);

    // dont run wizard again
    if (SelftestResult_Passed(m_result)) {
        config_store().run_selftest.set(false);    // clear selftest flag
        config_store().run_xyz_calib.set(false);   // clear XYZ calib flag
        config_store().run_first_layer.set(false); // clear first layer flag
    }
}

bool CSelftest::phaseWaitUser(PhasesSelftest phase) {
    const Response response = marlin_server::ClientResponseHandler::GetResponseFromPhase(phase);
    if (response == Response::Abort || response == Response::Cancel)
        Abort();
    if (response == Response::Ignore) {
        config_store().run_selftest.set(false);    // clear selftest flag
        config_store().run_xyz_calib.set(false);   // clear XYZ calib flag
        config_store().run_first_layer.set(false); // clear first layer flag
        Abort();
    }
    return response == Response::_none;
}

bool CSelftest::Abort() {
    if (!IsInProgress())
        return false;
    for (auto &pFan : pFans)
        abort_part(&pFan);
    abort_part((selftest::IPartHandler **)&pXAxis);
    abort_part((selftest::IPartHandler **)&pYAxis);
    abort_part((selftest::IPartHandler **)&pZAxis);
    for (auto &pNozzle : pNozzles)
        abort_part(&pNozzle);
    for (auto &loadcell : m_pLoadcell)
        abort_part(&loadcell);
    abort_part((selftest::IPartHandler **)&pFSensor);
    for (auto &dock : pDocks) {
        abort_part(&dock);
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
            m_result.tools[e].printFan = TestResult_Unknown;
            m_result.tools[e].heatBreakFan = TestResult_Unknown;
            m_result.tools[e].fansSwitched = TestResult_Unknown;
        }
    }
    if (m_Mask & stmXAxis)
        m_result.xaxis = TestResult_Unknown;
    if (m_Mask & stmYAxis)
        m_result.yaxis = TestResult_Unknown;
    if (m_Mask & stmZAxis)
        m_result.zaxis = TestResult_Unknown;
    if (m_Mask & stmZcalib)
        m_result.zalign = TestResult_Unknown;
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
    Fans::print(0).ExitSelftestMode();
    Fans::heat_break(0).ExitSelftestMode();

    thermalManager.disable_all_heaters();

    // Keep steppers enabled, so the next test doesn't have to rehome
    // Reset stepper timeout
    marlin_server::enqueue_gcode_printf("M18 S%d", DEFAULT_STEPPER_DEACTIVE_TIME);
}

void CSelftest::next() {
    if ((m_State == stsFinished) || (m_State == stsAborted))
        return;
    int state = m_State + 1;
    while ((((uint64_t(1) << state) & m_Mask) == 0) && (state < stsFinish))
        state++;
    m_State = (SelftestState_t)state;
}

const char *CSelftest::get_log_suffix() {
    const char *suffix = "";
    if (m_Mask & stmFans)
        suffix = _suffix[0];
    else if (m_Mask & stmXYAxis)
        suffix = _suffix[1];
    else if (m_Mask & stmXYZAxis)
        suffix = _suffix[1];
    else if (m_Mask & stmHeaters)
        suffix = _suffix[2];
    return suffix;
}

// declared in parent source file
ISelftest &SelftestInstance() {
    static CSelftest ret = CSelftest();
    return ret;
}

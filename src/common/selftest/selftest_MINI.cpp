// selftest.cpp

#include <fcntl.h>
#include <unistd.h>

#include "selftest_MINI.h"
#include "selftest_fan.h"
#include "selftest_axis.h"
#include "selftest_heater.h"
#include "stdarg.h"
#include "app.h"
#include "otp.h"
#include "hwio.h"
#include "marlin_server.hpp"
#include "wizard_config.hpp"
#include "../../Marlin/src/module/stepper.h"
#include "../../Marlin/src/module/temperature.h"
#include "eeprom.h"
#include "selftest_fans_type.hpp"
#include "selftest_axis_type.hpp"
#include "selftest_heaters_type.hpp"

static_assert(sizeof(SelftestResultEEprom_t) == 4, "Invalid size of SelftestResultEEprom_t (!= 4).");

#define HOMING_TIME 15000 // ~15s when X and Y axes are at opposite side to home position

static const char *_suffix[] = { "_fan", "_xyz", "_heaters" };
/// These speeds create major chord
/// https://en.wikipedia.org/wiki/Just_intonation
static const float XYfr_table[] = { 50, 62.5f, 75, 100 };

static const float Zfr_table[] = { 10 };

static const uint16_t Fan0min_rpm_table[] = { 10, 10, 10, 10, 10 };

static const uint16_t Fan0max_rpm_table[] = { 10000, 10000, 10000, 10000, 10000 };

static const uint16_t Fan1min_rpm_table[] = { 10, 10, 10, 10, 10 };

static const uint16_t Fan1max_rpm_table[] = { 10000, 10000, 10000, 10000, 10000 };

//use this?
/*
static const uint16_t Fan0min_rpm_table[] = { 150, 1250, 3250, 3250, 3850 };

static const uint16_t Fan0max_rpm_table[] = { 1950, 3950, 5050, 5950, 6650 };

static const uint16_t Fan1min_rpm_table[] = { 2350, 4750, 5950, 6850, 7650 };

static const uint16_t Fan1max_rpm_table[] = { 3750, 5850, 7050, 8050, 8950 };
*/

static const selftest_fan_config_t Config_Fan0 = { .partname = "Fan0", .fanctl = fanctl0, .pwm_start = 10, .pwm_step = 10, .rpm_min_table = Fan0min_rpm_table, .rpm_max_table = Fan0max_rpm_table, .steps = 5 };

static const selftest_fan_config_t Config_Fan1 = { .partname = "Fan1", .fanctl = fanctl1, .pwm_start = 10, .pwm_step = 10, .rpm_min_table = Fan1min_rpm_table, .rpm_max_table = Fan1max_rpm_table, .steps = 5 };

static const selftest_axis_config_t Config_XAxis = { .partname = "X-Axis", .length = 186, .fr_table = XYfr_table, .length_min = 178, .length_max = 188, .axis = X_AXIS, .steps = 4, .dir = -1 };

static const selftest_axis_config_t Config_YAxis = { .partname = "Y-Axis", .length = 185, .fr_table = XYfr_table, .length_min = 179, .length_max = 189, .axis = Y_AXIS, .steps = 4, .dir = 1 };

static const selftest_axis_config_t Config_ZAxis = { .partname = "Z-Axis", .length = 185, .fr_table = Zfr_table, .length_min = 181, .length_max = 191, .axis = Z_AXIS, .steps = 1, .dir = 1 };

static const selftest_heater_config_t Config_HeaterNozzle = { .partname = "Nozzle", .heat_time_ms = 42000, .start_temp = 40, .undercool_temp = 37, .target_temp = 290, .heat_min_temp = 130, .heat_max_temp = 190, .heater = 0 };

static const selftest_heater_config_t Config_HeaterBed = { .partname = "Bed", .heat_time_ms = 60000, .start_temp = 40, .undercool_temp = 39, .target_temp = 110, .heat_min_temp = 50, .heat_max_temp = 65, .heater = 0xff };

static const selftest_fan_config_t Config_Fan0_fine = { .partname = "Fan0", .fanctl = fanctl0, .pwm_start = 4, .pwm_step = 2, .rpm_min_table = nullptr, .rpm_max_table = nullptr, .steps = 24 };

static const selftest_fan_config_t Config_Fan1_fine = { .partname = "Fan1", .fanctl = fanctl1, .pwm_start = 4, .pwm_step = 2, .rpm_min_table = nullptr, .rpm_max_table = nullptr, .steps = 24 };

CSelftest::CSelftest()
    : m_State(stsIdle)
    , m_Mask(stmNone)
    , m_Time(0)
    , m_pFan0(nullptr)
    , m_pFan1(nullptr)
    , m_pXAxis(nullptr)
    , m_pYAxis(nullptr)
    , m_pZAxis(nullptr)
    , m_pHeater_Nozzle(nullptr)
    , m_pHeater_Bed(nullptr)
    , m_pFSM(nullptr)
    , m_filIsValid(false) {
}

bool CSelftest::IsInProgress() const {
    return ((m_State != stsIdle) && (m_State != stsFinished) && (m_State != stsAborted));
}

bool CSelftest::Start(SelftestMask_t mask) {
    m_Mask = mask;
    if (m_Mask & stmFans)
        m_Mask = (SelftestMask_t)(m_Mask | stmWait_fans);
    if (m_Mask & stmXYZAxis)
        m_Mask = (SelftestMask_t)(m_Mask | stmHome | stmWait_axes);
    if (m_Mask & stmHeaters)
        m_Mask = (SelftestMask_t)(m_Mask | stmWait_heaters);
    m_State = stsStart;
    return true;
}

void CSelftest::Loop() {
    uint32_t time = HAL_GetTick();
    if ((time - m_Time) < SELFTEST_LOOP_PERIODE)
        return;
    m_Time = time;
    switch (m_State) {
    case stsIdle:
        return;
    case stsStart:
        phaseStart();
        break;
    case stsFans:
        if (phaseFans(Config_Fan0, Config_Fan1))
            return;
        break;
    case stsWait_fans:
        if (phaseWait())
            return;
        break;
    case stsHome:
        if (phaseHome())
            return;
        break;
    case stsXAxis: {
        if (phaseAxis(Config_XAxis, &m_pXAxis))
            return;
        SelftestResultEEprom_t eeres;
        eeres.ui32 = variant8_get_ui32(eeprom_get_var(EEVAR_SELFTEST_RESULT));
        if (eeres.xaxis == SelftestResult_Failed) {
            m_State = stsWait_axes;
            return;
        }
        break;
    }
    case stsYAxis:
        if (phaseAxis(Config_YAxis, &m_pYAxis))
            return;
        break;
    case stsZAxis:
        if (phaseAxis(Config_ZAxis, &m_pZAxis))
            return;
        break;
    case stsWait_axes:
        if (phaseWait())
            return;
        break;
    case stsHeaters:
        if (phaseHeaters(Config_HeaterNozzle, Config_HeaterBed, fanctl0, fanctl1))
            return;
        break;
    case stsWait_heaters:
        if (phaseWait())
            return;
        break;
    case stsFans_fine:
        if (phaseFans(Config_Fan0_fine, Config_Fan1_fine))
            return;
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

bool CSelftest::Abort() {
    if (!IsInProgress())
        return false;
    abort_part((CSelftestPart **)&m_pFan0);
    abort_part((CSelftestPart **)&m_pFan1);
    abort_part((CSelftestPart **)&m_pXAxis);
    abort_part((CSelftestPart **)&m_pYAxis);
    abort_part((CSelftestPart **)&m_pZAxis);
    m_State = stsAborted;
    return true;
}

void CSelftest::phaseStart() {
    marlin_server_set_exclusive_mode(1);
    hwio_fan_control_disable();
    m_HomeState = sthsNone;
    if (m_Mask & stmHeaters) {
        thermalManager.setTargetHotend(40, 0);
        thermalManager.setTargetBed(40);
    }
    //log_open();
    SelftestResultEEprom_t eeres; // read previous result
    eeres.ui32 = variant8_get_ui32(eeprom_get_var(EEVAR_SELFTEST_RESULT));

    if (m_Mask & stmFans) {
        eeres.fan0 = 0;
        eeres.fan1 = 0;
    }
    if (m_Mask & stmXAxis)
        eeres.xaxis = 0;
    if (m_Mask & stmYAxis)
        eeres.yaxis = 0;
    if (m_Mask & stmZAxis)
        eeres.zaxis = 0;
    if (m_Mask & stmHeaters) {
        eeres.nozzle = 0;
        eeres.bed = 0;
    }
    eeprom_set_var(EEVAR_SELFTEST_RESULT, variant8_ui32(eeres.ui32)); // reset status for all selftest parts in eeprom
}

bool CSelftest::phaseFans(const selftest_fan_config_t &config_fan0, const selftest_fan_config_t &config_fan1) {
    m_pFSM = m_pFSM ? m_pFSM : new FSM_Holder(ClientFSM::SelftestFans, 0);
    m_pFan0 = m_pFan0 ? m_pFan0 : new CSelftestPart_Fan(config_fan0);
    m_pFan1 = m_pFan1 ? m_pFan1 : new CSelftestPart_Fan(config_fan1);
    m_pFan0->Loop();
    m_pFan1->Loop();
    if (m_pFan0->IsInProgress() || m_pFan1->IsInProgress()) {
        SelftestFans_t result(m_pFan0->GetProgress(), m_pFan1->GetProgress(), std::min(m_pFan0->GetProgress(), m_pFan1->GetProgress()), SelftestSubtestState_t(m_pFan0->getFSMState()), SelftestSubtestState_t(m_pFan1->getFSMState()));
        fsm_change(ClientFSM::SelftestFans, PhasesSelftestFans::measure, result.Serialize());
        return true;
    }
    SelftestFans_t result(100, 100, 100, SelftestSubtestState_t(m_pFan0->getFSMState()), SelftestSubtestState_t(m_pFan1->getFSMState()));
    fsm_change(ClientFSM::SelftestFans, PhasesSelftestFans::measure, result.Serialize());

    SelftestResultEEprom_t eeres;
    eeres.ui32 = variant8_get_ui32(eeprom_get_var(EEVAR_SELFTEST_RESULT));
    eeres.fan0 = m_pFan0->GetResult();
    eeres.fan1 = m_pFan1->GetResult();
    eeprom_set_var(EEVAR_SELFTEST_RESULT, variant8_ui32(eeres.ui32));
    delete m_pFan0;
    m_pFan0 = nullptr;
    delete m_pFan1;
    m_pFan1 = nullptr;
    return false;
}

bool CSelftest::phaseHome() {
    switch (m_HomeState) {
    case sthsNone:
        queue.enqueue_one_now("G28");
        m_HomeState = sthsHommingInProgress;
        return true;
    case sthsHommingInProgress:
        if (planner.movesplanned() || queue.length)
            return true;
        m_HomeState = sthsHommingFinished;
        break;
    case sthsHommingFinished:
        break;
    }
    return false;
}

SelftestSubtestState_t is_eeres_ok(uint8_t axis_eeres) {
    return axis_eeres == SelftestResult_Passed ? SelftestSubtestState_t::ok : SelftestSubtestState_t::not_good;
}

void send_axis_result(SelftestResultEEprom_t eeres, AxisEnum axis, uint8_t progress) {
    SelftestAxis_t result;

    //selftest does one axis at the time, bud dialog does not need to know that (this behavior can change)
    switch (axis) {
    case X_AXIS:
        result.x_progress = progress;
        result.x_state = SelftestSubtestState_t::running;
        break;
    case Y_AXIS: {
        result.x_progress = 100;
        result.x_state = is_eeres_ok(eeres.xaxis);

        result.y_progress = progress;
        result.y_state = SelftestSubtestState_t::running;
    } break;
    case Z_AXIS: {
        result.x_progress = 100;
        result.x_state = is_eeres_ok(eeres.xaxis);

        result.y_progress = 100;
        result.y_state = is_eeres_ok(eeres.yaxis);

        result.z_progress = progress;
        result.z_state = SelftestSubtestState_t::running;
    } break;
    default:
        break;
    }

    fsm_change(ClientFSM::SelftestAxis, PhasesSelftestAxis::measure, result.Serialize());
}

bool CSelftest::phaseAxis(const selftest_axis_config_t &config_axis, CSelftestPart_Axis **ppaxis) {
    m_pFSM = m_pFSM ? m_pFSM : new FSM_Holder(ClientFSM::SelftestAxis, 0);
    *ppaxis = *ppaxis ? *ppaxis : new CSelftestPart_Axis(config_axis);

    SelftestResultEEprom_t eeres;
    eeres.ui32 = variant8_get_ui32(eeprom_get_var(EEVAR_SELFTEST_RESULT));

    if ((*ppaxis)->Loop()) {
        send_axis_result(eeres, AxisEnum(config_axis.axis), (*ppaxis)->GetProgress());
        return true;
    }

    switch (config_axis.axis) {
    case X_AXIS:
        eeres.xaxis = (*ppaxis)->GetResult();
        break;
    case Y_AXIS:
        eeres.yaxis = (*ppaxis)->GetResult();
        break;
    case Z_AXIS:
        eeres.zaxis = (*ppaxis)->GetResult();
        break;
    }
    eeprom_set_var(EEVAR_SELFTEST_RESULT, variant8_ui32(eeres.ui32));

    SelftestAxis_t result(100, 100, 100, is_eeres_ok(eeres.xaxis), is_eeres_ok(eeres.yaxis), is_eeres_ok(eeres.zaxis));
    fsm_change(ClientFSM::SelftestAxis, PhasesSelftestAxis::measure, result.Serialize());

    delete *ppaxis;
    *ppaxis = nullptr;
    return false;
}

bool CSelftest::phaseHeaters(const selftest_heater_config_t &config_nozzle, const selftest_heater_config_t &config_bedconst, CFanCtl &fan0, CFanCtl &fan1) {
    m_pFSM = m_pFSM ? m_pFSM : new FSM_Holder(ClientFSM::SelftestHeat, 0);
    m_pHeater_Nozzle = m_pHeater_Nozzle ? m_pHeater_Nozzle : new CSelftestPart_HeaterHotend(Config_HeaterNozzle, Temperature::temp_hotend[0].pid, fan0, fan1);
    m_pHeater_Bed = m_pHeater_Bed ? m_pHeater_Bed : new CSelftestPart_Heater(Config_HeaterBed, Temperature::temp_bed.pid);
    m_pHeater_Nozzle->Loop();
    m_pHeater_Bed->Loop();
    if (m_pHeater_Nozzle->IsInProgress() || m_pHeater_Bed->IsInProgress()) {
        SelftestHeaters_t result(m_pHeater_Nozzle->GetProgress(), m_pHeater_Bed->GetProgress(),
            SelftestSubtestState_t(m_pHeater_Nozzle->getFSMState_prepare()), SelftestSubtestState_t(m_pHeater_Nozzle->getFSMState_heat()),
            SelftestSubtestState_t(m_pHeater_Bed->getFSMState_prepare()), SelftestSubtestState_t(m_pHeater_Bed->getFSMState_heat()));
        fsm_change(ClientFSM::SelftestHeat, PhasesSelftestHeat::measure, result.Serialize());

        return true;
    }
    SelftestHeaters_t result(100, 100,
        SelftestSubtestState_t(m_pHeater_Nozzle->getFSMState_prepare()), SelftestSubtestState_t(m_pHeater_Nozzle->getFSMState_heat()),
        SelftestSubtestState_t(m_pHeater_Bed->getFSMState_prepare()), SelftestSubtestState_t(m_pHeater_Bed->getFSMState_heat()));
    fsm_change(ClientFSM::SelftestHeat, PhasesSelftestHeat::measure, result.Serialize());

    SelftestResultEEprom_t eeres;
    eeres.ui32 = variant8_get_ui32(eeprom_get_var(EEVAR_SELFTEST_RESULT));
    eeres.nozzle = m_pHeater_Nozzle->GetResult();
    eeres.bed = m_pHeater_Bed->GetResult();
    eeprom_set_var(EEVAR_SELFTEST_RESULT, variant8_ui32(eeres.ui32));
    delete m_pHeater_Nozzle;
    m_pHeater_Nozzle = nullptr;
    delete m_pHeater_Bed;
    m_pHeater_Bed = nullptr;
    return false;
}

void CSelftest::phaseFinish() {
    log_close();
    hwio_fan_control_enable();
    marlin_server_set_exclusive_mode(0);
    thermalManager.disable_all_heaters();
    disable_all_steppers();
}

bool CSelftest::phaseWait() {
    static uint32_t tick = 0;
    if (tick == 0) {
        tick = m_Time;
        return true;
    } else if ((m_Time - tick) < 2000)
        return true;
    tick = 0;
    delete m_pFSM;
    m_pFSM = nullptr;
    return false;
}

void CSelftest::next() {
    if ((m_State == stsFinished) || (m_State == stsAborted))
        return;
    int state = m_State + 1;
    while ((((1 << state) & m_Mask) == 0) && (state < stsFinish))
        state++;
    m_State = (SelftestState_t)state;
}

void CSelftest::log_open() {
    char *serial_otp = (char *)OTP_SERIAL_NUMBER_ADDR;
    if (*serial_otp == 0xff) {
        serial_otp = 0;
    }
    char serial[32] = "unknown";
    const char *suffix = "";
    if (m_Mask & stmFans)
        suffix = _suffix[0];
    else if (m_Mask & stmXYAxis)
        suffix = _suffix[1];
    else if (m_Mask & stmXYZAxis)
        suffix = _suffix[1];
    else if (m_Mask & stmHeaters)
        suffix = _suffix[2];

    char fname[64];
    snprintf(fname, sizeof(fname), "test_unknown%s.txt", suffix);
    if (serial_otp) {
        snprintf(serial, sizeof(serial), "CZPX%.15s", serial_otp);
        snprintf(fname, sizeof(fname), "test_CZPX%.15s%s.txt", serial_otp, suffix);
    }
    m_fd = open(fname, O_WRONLY | O_CREAT);
    if (m_fd >= 0) {
        m_filIsValid = true;
        log_printf("SELFTEST START\n");
        log_printf("printer serial: %s\n\n", serial);
    } else
        m_filIsValid = false;
}

void CSelftest::log_close() {
    if (m_filIsValid) {
        log_printf("SELFTEST END\n");
        close(m_fd);
        m_filIsValid = false;
    }
}

int CSelftest::log_printf(const char *fmt, ...) {
    char line[SELFTEST_MAX_LOG_PRINTF];
    va_list va;
    va_start(va, fmt);
    int len = vsnprintf(line, SELFTEST_MAX_LOG_PRINTF, fmt, va);
    va_end(va);
    if (m_filIsValid) {
        write(m_fd, line, len);
        fsync(m_fd);
    }
    return len;
}

bool CSelftest::abort_part(CSelftestPart **pppart) {
    if (pppart && (*pppart)) {
        (*pppart)->Abort();
        delete *pppart;
        *pppart = nullptr;
        return true;
    }
    return false;
}

CSelftestPart::CSelftestPart()
    : m_State(0)
    , m_StartTime(0)
    , m_EndTime(UINT_MAX)
    , m_Result(sprUnknown) {
}

CSelftestPart::~CSelftestPart() {
}

float CSelftestPart::GetProgress() {
    float progress = 100.0F * (Selftest.m_Time - m_StartTime) / (m_EndTime - m_StartTime);
    if (progress < 0)
        progress = 0;
    if (progress > 100.0F)
        progress = 100.0F;
    return progress;
}

bool CSelftestPart::next() {
    if (!IsInProgress())
        return false;
    m_State = m_State + 1;
    return IsInProgress();
}

CSelftest Selftest = CSelftest();

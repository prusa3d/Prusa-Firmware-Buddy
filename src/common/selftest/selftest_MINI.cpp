// selftest.cpp

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
#include "ff.h"
#include "../../Marlin/src/module/stepper.h"
#include "../../Marlin/src/module/temperature.h"

#define HOMING_TIME 15000 // ~15s when X and Y axes are at oposite side to home position
#define _PF         2.40F // progres factor (because the value is in pixels! :) TODO fix this

static const float XYfr_table[] = { 50, 60, 75, 100 };

static const float Zfr_table[] = { 20 };

static const selftest_fan_config_t Config_Fan0 = { .partname = "Fan0", .pfanctl = &fanctl0, .steps = 5, .pwm_start = 10, .pwm_step = 10 };

static const selftest_fan_config_t Config_Fan1 = { .partname = "Fan1", .pfanctl = &fanctl1, .steps = 5, .pwm_start = 10, .pwm_step = 10 };

static const selftest_axis_config_t Config_XAxis = { .partname = "X-Axis", .axis = X_AXIS, .steps = 4, .dir = -1, .length = 186, .fr_table = XYfr_table };

static const selftest_axis_config_t Config_YAxis = { .partname = "Y-Axis", .axis = Y_AXIS, .steps = 4, .dir = 1, .length = 185, .fr_table = XYfr_table };

static const selftest_axis_config_t Config_ZAxis = { .partname = "Z-Axis", .axis = Z_AXIS, .steps = 1, .dir = 1, .length = 185, .fr_table = Zfr_table };

static const selftest_heater_config_t Config_HeaterNozzle = { .partname = "Nozzle", .heater = 0, .start_temp = 40, .max_temp = 290 };

static const selftest_heater_config_t Config_HeaterBed = { .partname = "Bed", .heater = 0xff, .start_temp = 40, .max_temp = 110 };

static const selftest_fan_config_t Config_Fan0_fine = { .partname = "Fan0", .pfanctl = &fanctl0, .steps = 24, .pwm_start = 4, .pwm_step = 2 };

static const selftest_fan_config_t Config_Fan1_fine = { .partname = "Fan1", .pfanctl = &fanctl1, .steps = 24, .pwm_start = 4, .pwm_step = 2 };

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
    m_Mask = (SelftestMask_t)(m_Mask & ~stmZAxis); // temporarily disable ZAxis test
    if (m_Mask & stmXYZAxis)
        m_Mask = (SelftestMask_t)(m_Mask | stmHome);
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
        if (phaseFans(&Config_Fan0, &Config_Fan1))
            return;
        break;
    case stsHome:
        if (phaseHome())
            return;
        break;
    case stsXAxis:
        if (phaseAxis(&Config_XAxis, &m_pXAxis, (uint16_t)PhasesSelftestAxis::Xaxis))
            return;
        break;
    case stsYAxis:
        if (phaseAxis(&Config_YAxis, &m_pYAxis, (uint16_t)PhasesSelftestAxis::Yaxis))
            return;
        break;
    case stsZAxis:
        //    	if (phaseAxis(&Config_ZAxis, &m_pZAxis, (uint16_t)PhasesSelftestAxis::Zaxis))
        //    		return;
        break;
    case stsHeaters:
        if (phaseHeaters(&Config_HeaterNozzle, &Config_HeaterBed))
            return;
        break;
    case stsFans_fine:
        if (phaseFans(&Config_Fan0_fine, &Config_Fan1_fine))
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
    log_open();
}

bool CSelftest::phaseFans(const selftest_fan_config_t *pconfig_fan0, const selftest_fan_config_t *pconfig_fan1) {
    m_pFSM = m_pFSM ? m_pFSM : new FSM_Holder(ClientFSM::SelftestFans, 0);
    m_pFan0 = m_pFan0 ? m_pFan0 : new CSelftestPart_Fan(pconfig_fan0);
    m_pFan1 = m_pFan1 ? m_pFan1 : new CSelftestPart_Fan(pconfig_fan1);
    m_pFan0->Loop();
    m_pFan1->Loop();
    if (m_pFan0->IsInProgress() || m_pFan1->IsInProgress()) {
        int p0 = m_pFan0->GetProgress();
        int p1 = m_pFan1->GetProgress();
        int p = _PF * ((p1 > p0) ? p0 : p1);
        fsm_change(ClientFSM::SelftestFans, PhasesSelftestFans::TestFan0, p, uint8_t(SelftestSubtestState_t::running));
        if (m_pFan1->IsInProgress())
            fsm_change(ClientFSM::SelftestFans, PhasesSelftestFans::TestFan1, p, uint8_t(SelftestSubtestState_t::running));
        else
            fsm_change(ClientFSM::SelftestFans, PhasesSelftestFans::TestFan1, p, uint8_t(SelftestSubtestState_t::ok));
        return true;
    }
    fsm_change(ClientFSM::SelftestFans, PhasesSelftestFans::TestFan0, _PF, uint8_t(SelftestSubtestState_t::ok));
    fsm_change(ClientFSM::SelftestFans, PhasesSelftestFans::TestFan1, _PF, uint8_t(SelftestSubtestState_t::ok));
    delete m_pFan0;
    m_pFan0 = nullptr;
    delete m_pFan1;
    m_pFan1 = nullptr;
    delete m_pFSM;
    m_pFSM = nullptr;
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

bool CSelftest::phaseAxis(const selftest_axis_config_t *pconfig_axis, CSelftestPart_Axis **ppaxis, uint16_t fsm_phase) {
    m_pFSM = m_pFSM ? m_pFSM : new FSM_Holder(ClientFSM::SelftestAxis, 0);
    *ppaxis = *ppaxis ? *ppaxis : new CSelftestPart_Axis(pconfig_axis);
    if ((*ppaxis)->Loop()) {
        int p = _PF * ((*ppaxis)->GetProgress());
        fsm_change(ClientFSM::SelftestAxis, (PhasesSelftestAxis)fsm_phase, p, uint8_t(SelftestSubtestState_t::running));
        return true;
    }
    fsm_change(ClientFSM::SelftestAxis, (PhasesSelftestAxis)fsm_phase, _PF, uint8_t(SelftestSubtestState_t::ok));
    delete *ppaxis;
    *ppaxis = nullptr;
    if (((m_State == stsXAxis) && ((m_Mask & (stmYAxis | stmZAxis)) == 0)) || ((m_State == stsYAxis) && ((m_Mask & stmZAxis) == 0)) || (m_State == stsZAxis)) {
        delete m_pFSM;
        m_pFSM = nullptr;
    }
    return false;
}

bool CSelftest::phaseHeaters(const selftest_heater_config_t *pconfig_nozzle, const selftest_heater_config_t *pconfig_bed) {
    m_pFSM = m_pFSM ? m_pFSM : new FSM_Holder(ClientFSM::SelftestHeat, 0);
    m_pHeater_Nozzle = m_pHeater_Nozzle ? m_pHeater_Nozzle : new CSelftestPart_Heater(&Config_HeaterNozzle);
    m_pHeater_Bed = m_pHeater_Bed ? m_pHeater_Bed : new CSelftestPart_Heater(&Config_HeaterBed);
    m_pHeater_Nozzle->Loop();
    m_pHeater_Bed->Loop();
    if (m_pHeater_Nozzle->IsInProgress() || m_pHeater_Bed->IsInProgress()) {
        int p0 = m_pHeater_Nozzle->GetProgress();
        int p1 = m_pHeater_Bed->GetProgress();
        int p = _PF * ((p1 > p0) ? p0 : p1);
        fsm_change(ClientFSM::SelftestHeat, PhasesSelftestHeat::noz_cool, p, uint8_t(SelftestSubtestState_t::running));
        fsm_change(ClientFSM::SelftestHeat, PhasesSelftestHeat::bed_cool, p, uint8_t(SelftestSubtestState_t::running));
        return true;
    }
    delete m_pHeater_Nozzle;
    m_pHeater_Nozzle = nullptr;
    delete m_pHeater_Bed;
    m_pHeater_Bed = nullptr;
    delete m_pFSM;
    m_pFSM = nullptr;
    return false;
}

void CSelftest::phaseFinish() {
    log_close();
    hwio_fan_control_enable();
    marlin_server_set_exclusive_mode(0);
    thermalManager.disable_all_heaters();
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
    switch (m_Mask) {
    case stmFans:
        suffix = "_fans";
        break;
    case stmXYAxis:
    case stmHome_XYAxis:
        suffix = "_xyz";
        break;
    case stmXYZAxis:
    case stmHome_XYZAxis:
        suffix = "_xyz";
        break;
    case stmHeaters:
        suffix = "_heaters";
        break;
    default:
        break;
    }
    char fname[64];
    snprintf(fname, sizeof(fname), "test_unknown%s.txt", suffix);
    if (serial_otp) {
        snprintf(serial, sizeof(serial), "CZPX%.15s", serial_otp);
        snprintf(fname, sizeof(fname), "test_CZPX%.15s%s.txt", serial_otp, suffix);
    }
    if (f_open(&m_fil, fname, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
        m_filIsValid = true;
        log_printf("SELFTEST START\n");
        log_printf("printer serial: %s\n\n", serial);
    } else
        m_filIsValid = false;
}

void CSelftest::log_close() {
    if (m_filIsValid) {
        log_printf("SELFTEST END\n");
        f_close(&m_fil);
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
        f_puts(line, &m_fil);
        f_sync(&m_fil);
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
    : m_Result(sprUnknown) {
}

CSelftestPart::~CSelftestPart() {
}

float CSelftestPart::GetProgress() const {
    float progress = 100.0F * (Selftest.m_Time - m_StartTime) / (m_EndTime - m_StartTime);
    if (progress < 0)
        progress = 0;
    if (progress > 100.0F)
        progress = 100.0F;
    return progress;
}

CSelftest Selftest = CSelftest();

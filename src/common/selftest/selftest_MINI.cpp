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

static const selftest_fan_config_t Config_Fan0 = { .partname = "Fan0", .pfanctl = &fanctl0, .pwm_start = 10, .pwm_step = 5, .pwm_steps = 10 };

static const selftest_fan_config_t Config_Fan1 = { .partname = "Fan1", .pfanctl = &fanctl1, .pwm_start = 10, .pwm_step = 10, .pwm_steps = 5 };

static const selftest_axis_config_t Config_XAxis = { .partname = "X-Axis", .axis = X_AXIS };

static const selftest_axis_config_t Config_YAxis = { .partname = "Y-Axis", .axis = Y_AXIS };

static const selftest_axis_config_t Config_ZAxis = { .partname = "Z-Axis", .axis = Z_AXIS };

static const selftest_heater_config_t Config_HeaterNozzle = { .partname = "Nozzle", .heater = 0 };

static const selftest_heater_config_t Config_HeaterBed = { .partname = "Bed", .heater = 0xff };

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
        marlin_server_set_exclusive_mode(1);
        hwio_fan_control_disable();
        CSelftestPart_Axis::ResetHome();
        log_open();
        break;
    case stsFans: {
        m_pFSM = m_pFSM ? m_pFSM : new FSM_Holder(ClientFSM::SelftestFans, 0);
        m_pFan0 = m_pFan0 ? m_pFan0 : new CSelftestPart_Fan(&Config_Fan0);
        m_pFan1 = m_pFan1 ? m_pFan1 : new CSelftestPart_Fan(&Config_Fan1);
        m_pFan0->Loop();
        m_pFan1->Loop();
        if (m_pFan0->IsInProgress() || m_pFan1->IsInProgress()) {
            int p0 = m_pFan0->GetProgress();
            int p1 = m_pFan1->GetProgress();
            fsm_change(ClientFSM::SelftestFans, PhasesSelftestFans::TestFan0, p0 + p1, uint8_t(SelftestSubtestState_t::running));
            if (m_pFan1->IsInProgress())
                fsm_change(ClientFSM::SelftestFans, PhasesSelftestFans::TestFan1, p0 + p1, uint8_t(SelftestSubtestState_t::running));
            else
                fsm_change(ClientFSM::SelftestFans, PhasesSelftestFans::TestFan1, p0 + 100, uint8_t(SelftestSubtestState_t::ok));
            return;
        }
        fsm_change(ClientFSM::SelftestFans, PhasesSelftestFans::TestFan0, 200, uint8_t(SelftestSubtestState_t::ok));
        fsm_change(ClientFSM::SelftestFans, PhasesSelftestFans::TestFan1, 200, uint8_t(SelftestSubtestState_t::ok));
        delete m_pFan0;
        m_pFan0 = nullptr;
        delete m_pFan1;
        m_pFan1 = nullptr;
        delete m_pFSM;
        m_pFSM = nullptr;
    } break;
    case stsXAxis:
        m_pFSM = m_pFSM ? m_pFSM : new FSM_Holder(ClientFSM::SelftestAxis, 0);
        m_pXAxis = m_pXAxis ? m_pXAxis : new CSelftestPart_Axis(&Config_XAxis);
        if (m_pXAxis->Loop()) {
            int p = m_pXAxis->GetProgress();
            fsm_change(ClientFSM::SelftestAxis, PhasesSelftestAxis::Xaxis, p / 3, uint8_t(SelftestSubtestState_t::running));
            return;
        }
        fsm_change(ClientFSM::SelftestAxis, PhasesSelftestAxis::Xaxis, 33, uint8_t(SelftestSubtestState_t::ok));
        delete m_pXAxis;
        m_pXAxis = nullptr;
        if ((m_Mask & (stmYAxis | stmZAxis)) == 0) {
            delete m_pFSM;
            m_pFSM = nullptr;
        }
        break;
    case stsYAxis:
        m_pFSM = m_pFSM ? m_pFSM : new FSM_Holder(ClientFSM::SelftestAxis, 0);
        m_pYAxis = m_pYAxis ? m_pYAxis : new CSelftestPart_Axis(&Config_YAxis);
        if (m_pYAxis->Loop()) {
            int p = m_pYAxis->GetProgress();
            fsm_change(ClientFSM::SelftestAxis, PhasesSelftestAxis::Yaxis, 33 + p / 3, uint8_t(SelftestSubtestState_t::running));
            return;
        }
        fsm_change(ClientFSM::SelftestAxis, PhasesSelftestAxis::Yaxis, 66, uint8_t(SelftestSubtestState_t::ok));
        delete m_pYAxis;
        m_pYAxis = nullptr;
        if ((m_Mask & stmZAxis) == 0) {
            delete m_pFSM;
            m_pFSM = nullptr;
        }
        break;
    case stsZAxis:
        m_pFSM = m_pFSM ? m_pFSM : new FSM_Holder(ClientFSM::SelftestAxis, 0);
        m_pZAxis = m_pZAxis ? m_pZAxis : new CSelftestPart_Axis(&Config_ZAxis);
        if (m_pZAxis->Loop()) {
            int p = m_pYAxis->GetProgress();
            fsm_change(ClientFSM::SelftestAxis, PhasesSelftestAxis::Zaxis, 66 + p / 3, uint8_t(SelftestSubtestState_t::running));
            return;
        }
        fsm_change(ClientFSM::SelftestAxis, PhasesSelftestAxis::Zaxis, 100, uint8_t(SelftestSubtestState_t::ok));
        delete m_pZAxis;
        m_pZAxis = nullptr;
        delete m_pFSM;
        m_pFSM = nullptr;
        break;
    case stsHeaters:
        m_pHeater_Nozzle = m_pHeater_Nozzle ? m_pHeater_Nozzle : new CSelftestPart_Heater(&Config_HeaterNozzle);
        m_pHeater_Bed = m_pHeater_Bed ? m_pHeater_Bed : new CSelftestPart_Heater(&Config_HeaterBed);
        m_pHeater_Nozzle->Loop();
        m_pHeater_Bed->Loop();
        if (m_pHeater_Nozzle->IsInProgress() || m_pFan1->IsInProgress()) {
            //int p0 = m_pHeater_Nozzle->GetProgress();
            //int p1 = m_pHeater_Bed->GetProgress();
            return;
        }
        delete m_pHeater_Nozzle;
        m_pHeater_Nozzle = nullptr;
        delete m_pHeater_Bed;
        m_pHeater_Bed = nullptr;
        break;
    case stsFinish:
        log_close();
        hwio_fan_control_enable();
        marlin_server_set_exclusive_mode(0);
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
    char fname[32] = "test_unknown.txt";
    if (serial_otp) {
        sprintf(serial, "CZPX%.15s", serial_otp);
        sprintf(fname, "test_CZPX%.15s.txt", serial_otp);
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

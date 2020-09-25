// selftest.cpp

#include "selftest_MINI.h"
#include "selftest_fan.h"
#include "selftest_axis.h"
#include "selftest_heater.h"
#include "stdarg.h"
#include "app.h"
#include "otp.h"
#include "hwio.h"
#include "marlin_server.h"
#include "ff.h"
#include "../../Marlin/src/module/stepper.h"

CSelftest::CSelftest()
    : m_State(stsIdle)
    , m_Mask(stmNone)
    , m_pFan0(nullptr)
    , m_pFan1(nullptr)
    , m_pXAxis(nullptr)
    , m_pYAxis(nullptr)
    , m_pZAxis(nullptr)
    , m_pHeater_Nozzle(nullptr)
    , m_pHeater_Bed(nullptr)
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
    static uint32_t last_time = 0;
    uint32_t time = HAL_GetTick();
    if ((time - last_time) < SELFTEST_LOOP_PERIODE)
        return;
    last_time = time;
    switch (m_State) {
    case stsIdle:
        return;
    case stsStart:
        marlin_server_set_exclusive_mode(1);
        hwio_fan_control_disable();
        log_open();
        break;
    case stsFans:
        //		if (m_pFan0 == nullptr)
        //			m_pFan0 = new CSelftestPart_Fan("FAN0", &fanctl0, 10, 10, 5);
        //		if (m_pFan1 == nullptr)
        //			m_pFan1 = new CSelftestPart_Fan("FAN1", &fanctl1, 10, 10, 5);
        if (m_pFan0 == nullptr)
            m_pFan0 = new CSelftestPart_Fan("FAN0", &fanctl0, 4, 2, 23);
        if (m_pFan1 == nullptr)
            m_pFan1 = new CSelftestPart_Fan("FAN1", &fanctl1, 4, 2, 23);
        m_pFan0->Loop();
        m_pFan1->Loop();
        if (m_pFan0->IsInProgress() || m_pFan1->IsInProgress())
            return;
        delete m_pFan0;
        m_pFan0 = nullptr;
        delete m_pFan1;
        m_pFan1 = nullptr;
        break;
    case stsXAxis:
        if (m_pXAxis == nullptr)
            m_pXAxis = new CSelftestPart_Axis(X_AXIS);
        if (m_pXAxis->Loop())
            return;
        delete m_pXAxis;
        m_pXAxis = nullptr;
        break;
    case stsYAxis:
        m_pYAxis = m_pYAxis ? m_pYAxis : new CSelftestPart_Axis(Y_AXIS);
        if (m_pYAxis->Loop())
            return;
        delete m_pYAxis;
        m_pYAxis = nullptr;
        break;
    case stsZAxis:
        if (m_pZAxis->Loop())
            return;
        delete m_pZAxis;
        m_pZAxis = nullptr;
        break;
    case stsHeaters:
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

CSelftestPart::~CSelftestPart() {
}

CSelftest Selftest = CSelftest();

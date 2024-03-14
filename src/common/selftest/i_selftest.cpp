// selftest.cpp

#include "i_selftest.hpp"
#include "feature/prusa/crash_recovery.hpp"
#include "stdarg.h"
#include "log.h"
#include "app.h"
#include "otp.hpp"
#include "hwio.h"
#include "marlin_server.hpp"
#include "wizard_config.hpp"
#include "filament_sensors_handler.hpp"

LOG_COMPONENT_DEF(Selftest, LOG_SEVERITY_DEBUG);

ISelftest::ISelftest()
    : m_Time(0) {
}

void ISelftest::phaseStart() {
    FSensors_instance().IncEvLock(); // block autoload and M600
    marlin_server::set_exclusive_mode(1);
#if ENABLED(CRASH_RECOVERY)
    crash_s.set_state(Crash_s::SELFTEST);
#endif
    FSM_CREATE__LOGGING(Selftest); // TODO data 0/1 selftest/wizard
}

void ISelftest::phaseFinish() {
    FSM_DESTROY__LOGGING(Selftest);
    marlin_server::set_exclusive_mode(0);
#if ENABLED(CRASH_RECOVERY)
    crash_s.set_state(Crash_s::IDLE);
#endif
    FSensors_instance().DecEvLock(); // stop blocking autoload and M600
}

bool ISelftest::phaseWait() {
    static uint32_t tick = 0;
    if (tick == 0) {
        tick = m_Time;
        return true;
    } else if ((m_Time - tick) < 2000) {
        return true;
    }
    tick = 0;
    return false;
}

bool ISelftest::abort_part(selftest::IPartHandler **pppart) {
    if (pppart && (*pppart)) {
        (*pppart)->Abort();
        delete *pppart;
        *pppart = nullptr;
        return true;
    }
    return false;
}

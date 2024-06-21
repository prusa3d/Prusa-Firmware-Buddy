// selftest.cpp

#include "i_selftest.hpp"
#include "feature/prusa/crash_recovery.hpp"
#include <logging/log.hpp>
#include "marlin_server.hpp"
#include "filament_sensors_handler.hpp"

LOG_COMPONENT_DEF(Selftest, logging::Severity::debug);

ISelftest::ISelftest()
    : m_Time(0) {
}

void ISelftest::phaseStart() {
    FSensors_instance().IncEvLock(); // block autoload and M600
    marlin_server::set_exclusive_mode(1);
#if HAS_PHASE_STEPPING()
    phstep_restorer.set_state(false);
#endif
#if ENABLED(CRASH_RECOVERY)
    crash_s.set_state(Crash_s::SELFTEST);
#endif
    marlin_server::fsm_create(PhasesSelftest::_none);
}

void ISelftest::phaseFinish() {
    marlin_server::fsm_destroy(ClientFSM::Selftest);
    marlin_server::set_exclusive_mode(0);
#if HAS_PHASE_STEPPING()
    phstep_restorer.release();
#endif
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

#include "marlin_vars.hpp"
#include <common/random.h>

#include <cassert>

void marlin_vars_t::lock() {

    if (current_mutex_owner == osThreadGetId()) {
        bsod("Mutex recursive lock");
    }

    [[maybe_unused]] auto res = osMutexWait(mutex_id, osWaitForever);
    assert(res == osOK);
    current_mutex_owner = osThreadGetId();
}

void marlin_vars_t::unlock() {
    current_mutex_owner = nullptr;
    [[maybe_unused]] auto res = osMutexRelease(mutex_id);
    assert(res == osOK);
}

MarlinVarsLockGuard::MarlinVarsLockGuard() {
    marlin_vars()->lock();
}

MarlinVarsLockGuard::~MarlinVarsLockGuard() {
    marlin_vars()->unlock();
}

void marlin_vars_t::init() {
    mutex_id = osMutexCreate(osMutex(mutex));
}

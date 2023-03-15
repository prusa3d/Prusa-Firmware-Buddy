#include "marlin_vars.hpp"

marlin_vars_t marlin_vars_instance;

void marlin_msg_to_str(const marlin_msg_t id, char *str) {
    str[0] = '!';
    str[1] = (char)id;
    str[2] = 0;
}

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
    marlin_vars_instance.lock();
}

MarlinVarsLockGuard::~MarlinVarsLockGuard() {
    marlin_vars_instance.unlock();
}

void marlin_vars_t::init() {
    mutex_id = osMutexCreate(osMutex(mutex));
}

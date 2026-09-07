#include "marlin_vars.hpp"
#include <random/random.h>

void marlin_vars_t::lock() {
    mutex.lock();
}

void marlin_vars_t::unlock() {
    mutex.unlock();
}

MarlinVarsLockGuard::MarlinVarsLockGuard() {
    marlin_vars().lock();
}

MarlinVarsLockGuard::~MarlinVarsLockGuard() {
    marlin_vars().unlock();
}

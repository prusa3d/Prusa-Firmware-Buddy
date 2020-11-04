#include "TMCStepper.h"

bool __attribute__((weak)) tmc_serial_lock_acquire() {
	return true;
}

void __attribute__((weak)) tmc_serial_lock_release() {}

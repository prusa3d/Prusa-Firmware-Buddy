#pragma once

#include "cmsis_os.h"

#include "puppies/PuppyModbus.hpp"
#include "puppies/PuppyBootstrap.hpp"

namespace buddy::puppies {

extern osThreadId puppy_task_handle;

/// Initialize and start task taking care of the puppies via Modbus
extern void start_puppy_task();

/// Get progress of the bootstrap stage (if currently active)
extern std::optional<PuppyBootstrap::Progress> get_bootstrap_progress();

}

#pragma once

#include <common/variant8.h>
#include <common/freertos_queue.hpp>

namespace marlin_client {

using ClientQueue = freertos::Queue<variant8_t, 16>;

}

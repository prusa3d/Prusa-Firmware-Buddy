#pragma once

#include <common/marlin_events.h>
#include <freertos/queue.hpp>

namespace marlin_client {

using ClientQueue = freertos::Queue<marlin_server::Event, 16>;

} // namespace marlin_client

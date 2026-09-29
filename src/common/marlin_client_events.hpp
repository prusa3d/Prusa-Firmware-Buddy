#pragma once

#include "config.h"
#include <common/marlin_events.h>
#include <freertos/event_group.hpp>
#include <type_traits>

namespace marlin_client {

static_assert(std::is_same_v<marlin_server::EventMask, freertos::EventGroup::Bits>);

/// Events for each client, set by the server and consumed by the client.
extern freertos::EventGroup marlin_client_events[MARLIN_MAX_CLIENTS];

} // namespace marlin_client

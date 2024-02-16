#pragma once

#include <common/marlin_events.h>
#include <common/freertos_queue.hpp>

namespace marlin_client {

struct ClientEvent {
    marlin_server::Event event;
    uint8_t unused;
    uint16_t usr16;
    union {
        char *message; // Event::Message
        uint32_t usr32; // other events
    };
};
static_assert(sizeof(ClientEvent) == 8);
static_assert(std::is_trivial_v<ClientEvent>);

using ClientQueue = freertos::Queue<ClientEvent, 16>;

} // namespace marlin_client

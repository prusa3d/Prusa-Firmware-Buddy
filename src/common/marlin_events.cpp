// marlin_events.c

#include "marlin_events.h"
#include "utility_extensions.hpp"
#include <iterator>

namespace marlin_server {

// event name constants (dbg)
const char *__evt_name[] = {
    "Startup",
    "PrinterKilled",
    "MediaInserted",
    "MediaError",
    "MediaRemoved",
    "PlayTone",
    "PrintTimerStarted",
    "PrintTimerPaused",
    "PrintTimerStopped",
    "FilamentRunout",
    "UserConfirmRequired",
    "StatusChanged",
    "FactoryReset",
    "LoadSettings",
    "StoreSettings",
    "MeshUpdate",
    "StartProcessing",
    "StopProcessing",
    "Error",
    "CommandBegin",
    "CommandEnd",
    "SafetyTimerExpired",
    "Message",
    "Reheat",
    "DialogOpenCloseChange",
    "Acknowledge",
    "Not Acknowledge",
};

static_assert(std::size(__evt_name) == ftrstd::to_underlying(Event::_count), "Incorrect number of event names");

// returns event name (dbg)
const char *marlin_events_get_name(Event evt_id) {
    if (evt_id <= Event::_last) {
        return __evt_name[ftrstd::to_underlying(evt_id)];
    }
    return "";
}

} // namespace marlin_server

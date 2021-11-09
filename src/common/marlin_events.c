// marlin_events.c

#include "marlin_events.h"
#include <stdio.h>

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
    "Warning",
    "Reheat",
    "DialogOpenCloseChange",
    "Acknowledge",
};

_Static_assert((sizeof(__evt_name) / sizeof(__evt_name[0])) == (MARLIN_EVT_MAX + 1), "Incorrect number of event names");

// returns event name (dbg)
const char *marlin_events_get_name(MARLIN_EVT_t evt_id) {
    if (evt_id <= MARLIN_EVT_MAX)
        return __evt_name[evt_id];
    return "";
}

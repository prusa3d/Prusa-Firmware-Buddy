// marlin_events.c

#include "marlin_events.h"
#include <stdio.h>

// event name constants (dbg)
const char *__evt_name[] = {
    "Startup",
    "Idle",
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
    "HostPrompt",
    "StartProcessing",
    "StopProcessing",
    "Busy",
    "Ready",
    "Error",
    "CommandBegin",
    "CommandEnd",
    "SafetyTimerExpired",
    "Message",
    "Reheat",
    "DialogCreation",
    "Acknowledge",
};

// returns event name (dbg)
const char *marlin_events_get_name(uint8_t evt_id) {
    if (evt_id <= MARLIN_EVT_MAX)
        return __evt_name[evt_id];
    return "";
}

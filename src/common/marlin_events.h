// marlin_events.h
#pragma once

#include "variant8.h"

typedef enum {
    // Marlin events - UIAPI
    MARLIN_EVT_Startup,             // onStartup()
    MARLIN_EVT_PrinterKilled,       // onPrinterKilled(PGM_P const msg)
    MARLIN_EVT_MediaInserted,       // onMediaInserted();
    MARLIN_EVT_MediaError,          // onMediaError();
    MARLIN_EVT_MediaRemoved,        // onMediaRemoved();
    MARLIN_EVT_FSM,                 // create/destroy finite state machine or change phase/state/progress in client
    MARLIN_EVT_PlayTone,            // onPlayTone(const uint16_t frequency, const uint16_t duration)
    MARLIN_EVT_PrintTimerStarted,   // onPrintTimerStarted()
    MARLIN_EVT_PrintTimerPaused,    // onPrintTimerPaused()
    MARLIN_EVT_PrintTimerStopped,   // onPrintTimerStopped()
    MARLIN_EVT_FilamentRunout,      // onFilamentRunout()
    MARLIN_EVT_UserConfirmRequired, // onUserConfirmRequired(const char * const msg)
    MARLIN_EVT_StatusChanged,       // onStatusChanged(const char * const msg)
    MARLIN_EVT_FactoryReset,        // onFactoryReset()
    MARLIN_EVT_LoadSettings,        // onLoadSettings()
    MARLIN_EVT_StoreSettings,       // onStoreSettings()
    MARLIN_EVT_MeshUpdate,          // onMeshUpdate(const uint8_t xpos, const uint8_t ypos, const float zval)
                                    // Marlin events - other
    MARLIN_EVT_StartProcessing,     // sent from marlin_server_start_processing
    MARLIN_EVT_StopProcessing,      // sent from marlin_server_stop_processing
    MARLIN_EVT_Error,               // sent onStatusChanged etc.
    MARLIN_EVT_CommandBegin,        //
    MARLIN_EVT_CommandEnd,          //
    MARLIN_EVT_SafetyTimerExpired,  // host action from marlin, hotends and bed turned off
    MARLIN_EVT_Message,             //
    MARLIN_EVT_Warning,             // important messages like fan error or heater timeout
    MARLIN_EVT_Reheat,              //
    MARLIN_EVT_Acknowledge,         // onAcknowledge - lowest priority

    MARLIN_EVT_MAX = MARLIN_EVT_Acknowledge
} MARLIN_EVT_t;

// event masks
#define MARLIN_EVT_MSK(e_id) ((uint64_t)1 << (e_id))
#define MARLIN_EVT_MSK_ALL   ((MARLIN_EVT_MSK(MARLIN_EVT_MAX + 1) - (uint64_t)1))

static const uint64_t MARLIN_EVT_MSK_DEF = MARLIN_EVT_MSK_ALL - (MARLIN_EVT_MSK(MARLIN_EVT_PrinterKilled));
static const uint64_t MARLIN_EVT_MSK_FSM = MARLIN_EVT_MSK(MARLIN_EVT_FSM);

// commands
enum {
    MARLIN_CMD_NONE = 0,
    MARLIN_CMD_G = (uint32_t)'G' << 16,
    MARLIN_CMD_M = (uint32_t)'M' << 16,
    MARLIN_CMD_G28 = MARLIN_CMD_G + 28,
    MARLIN_CMD_G29 = MARLIN_CMD_G + 29,
    MARLIN_CMD_M109 = MARLIN_CMD_M + 109,
    MARLIN_CMD_M190 = MARLIN_CMD_M + 190,
    MARLIN_CMD_M303 = MARLIN_CMD_M + 303,
    MARLIN_CMD_M600 = MARLIN_CMD_M + 600,
    MARLIN_CMD_M701 = MARLIN_CMD_M + 701,
    MARLIN_CMD_M702 = MARLIN_CMD_M + 702,
    MARLIN_CMD_M876 = MARLIN_CMD_M + 876,

    MARLIN_MAX_MESH_POINTS = (4 * 4),
};

typedef struct _marlin_mesh_t {
    float z[MARLIN_MAX_MESH_POINTS];
    uint8_t xc;
    uint8_t yc;
} marlin_mesh_t;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const char *marlin_events_get_name(MARLIN_EVT_t evt_id);

#ifdef __cplusplus
}
#endif //__cplusplus

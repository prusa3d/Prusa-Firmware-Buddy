// marlin_events.h
#ifndef _MARLIN_EVENTS_H
#define _MARLIN_EVENTS_H

#include "variant8.h"

// Marlin events - UIAPI
#define MARLIN_EVT_Startup 0x00 // onStartup()
#define MARLIN_EVT_Idle 0x01 // onIdle()
#define MARLIN_EVT_PrinterKilled 0x02 // onPrinterKilled(PGM_P const msg)
#define MARLIN_EVT_MediaInserted 0x03 // onMediaInserted();
#define MARLIN_EVT_MediaError 0x04 // onMediaError();
#define MARLIN_EVT_MediaRemoved 0x05 // onMediaRemoved();
#define MARLIN_EVT_PlayTone 0x06 // onPlayTone(const uint16_t frequency, const uint16_t duration)
#define MARLIN_EVT_PrintTimerStarted 0x07 // onPrintTimerStarted()
#define MARLIN_EVT_PrintTimerPaused 0x08 // onPrintTimerPaused()
#define MARLIN_EVT_PrintTimerStopped 0x09 // onPrintTimerStopped()
#define MARLIN_EVT_FilamentRunout 0x0a // onFilamentRunout()
#define MARLIN_EVT_UserConfirmRequired 0x0b // onUserConfirmRequired(const char * const msg)
#define MARLIN_EVT_StatusChanged 0x0c // onStatusChanged(const char * const msg)
#define MARLIN_EVT_FactoryReset 0x0d // onFactoryReset()
#define MARLIN_EVT_LoadSettings 0x0e // onLoadSettings()
#define MARLIN_EVT_StoreSettings 0x0f // onStoreSettings()
#define MARLIN_EVT_MeshUpdate 0x10 // onMeshUpdate(const uint8_t xpos, const uint8_t ypos, const float zval)
// Marlin events - host actions
#define MARLIN_EVT_HostPrompt 0x11 // host_action_prompt
// Marlin events - other
#define MARLIN_EVT_StartProcessing 0x12 // sent from marlin_server_start_processing
#define MARLIN_EVT_StopProcessing 0x13 // sent from marlin_server_stop_processing
#define MARLIN_EVT_Busy 0x14 // sent from marlin_server_idle
#define MARLIN_EVT_Ready 0x15 // sent from marlin_server_loop
#define MARLIN_EVT_Error 0x16 // sent onStatusChanged etc.
#define MARLIN_EVT_CommandBegin 0x17 //
#define MARLIN_EVT_CommandEnd 0x18 //
#define MARLIN_EVT_SafetyTimerExpired 0x19 // host action from marlin, hotends and bed turned off
#define MARLIN_EVT_Message 0x1a //
#define MARLIN_EVT_Reheat 0x1b //
#define MARLIN_EVT_DialogCreation 0x1c //
#define MARLIN_EVT_Acknowledge 0x1d // onAcknowledge - lowest priority
#define MARLIN_EVT_MAX MARLIN_EVT_Acknowledge

// event masks
#define MARLIN_EVT_MSK(e_id) ((uint64_t)1 << (e_id))
#define MARLIN_EVT_MSK_DEF ( \
    MARLIN_EVT_MSK(MARLIN_EVT_Startup) | MARLIN_EVT_MSK(MARLIN_EVT_MediaInserted) | MARLIN_EVT_MSK(MARLIN_EVT_MediaError) | MARLIN_EVT_MSK(MARLIN_EVT_MediaRemoved) | MARLIN_EVT_MSK(MARLIN_EVT_PlayTone) | MARLIN_EVT_MSK(MARLIN_EVT_PrintTimerStarted) | MARLIN_EVT_MSK(MARLIN_EVT_PrintTimerPaused) | MARLIN_EVT_MSK(MARLIN_EVT_PrintTimerStopped) | MARLIN_EVT_MSK(MARLIN_EVT_FilamentRunout) | MARLIN_EVT_MSK(MARLIN_EVT_UserConfirmRequired) | MARLIN_EVT_MSK(MARLIN_EVT_StatusChanged) | MARLIN_EVT_MSK(MARLIN_EVT_FactoryReset) | MARLIN_EVT_MSK(MARLIN_EVT_LoadSettings) | MARLIN_EVT_MSK(MARLIN_EVT_StoreSettings) | MARLIN_EVT_MSK(MARLIN_EVT_MeshUpdate) | MARLIN_EVT_MSK(MARLIN_EVT_HostPrompt) | MARLIN_EVT_MSK(MARLIN_EVT_StartProcessing) | MARLIN_EVT_MSK(MARLIN_EVT_StopProcessing) | MARLIN_EVT_MSK(MARLIN_EVT_Busy) | MARLIN_EVT_MSK(MARLIN_EVT_Ready) | MARLIN_EVT_MSK(MARLIN_EVT_Error) | MARLIN_EVT_MSK(MARLIN_EVT_CommandBegin) | MARLIN_EVT_MSK(MARLIN_EVT_CommandEnd) | MARLIN_EVT_MSK(MARLIN_EVT_SafetyTimerExpired) | MARLIN_EVT_MSK(MARLIN_EVT_Message) | MARLIN_EVT_MSK(MARLIN_EVT_Reheat) | MARLIN_EVT_MSK(MARLIN_EVT_Acknowledge))

#define MARLIN_EVT_MSK_ALL ( \
    MARLIN_EVT_MSK(MARLIN_EVT_Startup) | MARLIN_EVT_MSK(MARLIN_EVT_Idle) | MARLIN_EVT_MSK(MARLIN_EVT_PrinterKilled) | MARLIN_EVT_MSK(MARLIN_EVT_MediaInserted) | MARLIN_EVT_MSK(MARLIN_EVT_MediaError) | MARLIN_EVT_MSK(MARLIN_EVT_MediaRemoved) | MARLIN_EVT_MSK(MARLIN_EVT_PlayTone) | MARLIN_EVT_MSK(MARLIN_EVT_PrintTimerStarted) | MARLIN_EVT_MSK(MARLIN_EVT_PrintTimerPaused) | MARLIN_EVT_MSK(MARLIN_EVT_PrintTimerStopped) | MARLIN_EVT_MSK(MARLIN_EVT_FilamentRunout) | MARLIN_EVT_MSK(MARLIN_EVT_UserConfirmRequired) | MARLIN_EVT_MSK(MARLIN_EVT_StatusChanged) | MARLIN_EVT_MSK(MARLIN_EVT_FactoryReset) | MARLIN_EVT_MSK(MARLIN_EVT_LoadSettings) | MARLIN_EVT_MSK(MARLIN_EVT_StoreSettings) | MARLIN_EVT_MSK(MARLIN_EVT_MeshUpdate) | MARLIN_EVT_MSK(MARLIN_EVT_HostPrompt) | MARLIN_EVT_MSK(MARLIN_EVT_StartProcessing) | MARLIN_EVT_MSK(MARLIN_EVT_StopProcessing) | MARLIN_EVT_MSK(MARLIN_EVT_Busy) | MARLIN_EVT_MSK(MARLIN_EVT_Ready) | MARLIN_EVT_MSK(MARLIN_EVT_Error) | MARLIN_EVT_MSK(MARLIN_EVT_CommandBegin) | MARLIN_EVT_MSK(MARLIN_EVT_CommandEnd) | MARLIN_EVT_MSK(MARLIN_EVT_SafetyTimerExpired) | MARLIN_EVT_MSK(MARLIN_EVT_Message) | MARLIN_EVT_MSK(MARLIN_EVT_Reheat) | MARLIN_EVT_MSK(MARLIN_EVT_Acknowledge))

// commands
#define MARLIN_CMD_NONE 0
#define MARLIN_CMD_G (((uint32_t)'G') << 16)
#define MARLIN_CMD_M (((uint32_t)'M') << 16)
#define MARLIN_CMD_G28 (MARLIN_CMD_G + 28)
#define MARLIN_CMD_G29 (MARLIN_CMD_G + 29)
#define MARLIN_CMD_M109 (MARLIN_CMD_M + 109)
#define MARLIN_CMD_M190 (MARLIN_CMD_M + 190)
#define MARLIN_CMD_M600 (MARLIN_CMD_M + 600)
#define MARLIN_CMD_M701 (MARLIN_CMD_M + 701)
#define MARLIN_CMD_M702 (MARLIN_CMD_M + 702)
#define MARLIN_CMD_M876 (MARLIN_CMD_M + 876)
#define MARLIN_MAX_MESH_POINTS (4 * 4)

#pragma pack(push)
#pragma pack(1)

typedef union _marlin_events_t {
    uint64_t evt;
    struct
    {
        uint8_t evt_Startup : 1;
        uint8_t evt_Idle : 1;
        uint8_t evt_PrinterKilled : 1;
        uint8_t evt_MediaInserted : 1;
        uint8_t evt_MediaError : 1;
        uint8_t evt_MediaRemoved : 1;
        uint8_t evt_PlayTone : 1;
        uint8_t evt_PrintTimerStarted : 1;
        uint8_t evt_PrintTimerPaused : 1;
        uint8_t evt_PrintTimerStopped : 1;
        uint8_t evt_FilamentRunout : 1;
        uint8_t evt_UserConfirmRequired : 1;
        uint8_t evt_StatusChanged : 1;
        uint8_t evt_FactoryReset : 1;
        uint8_t evt_LoadSettings : 1;
        uint8_t evt_StoreSettings : 1;
        uint8_t evt_MeshUpdate : 1;
        uint8_t evt_HostPrompt : 1;
        uint8_t evt_StartProcessing : 1;
        uint8_t evt_StopProcessing : 1;
        uint8_t evt_Busy : 1;
        uint8_t evt_Ready : 1;
        uint8_t evt_Error : 1;
        uint8_t evt_CommandBegin : 1;
        uint8_t evt_CommandEnd : 1;
        uint8_t evt_SafetyTimerExpired : 1;
        uint8_t evt_Message : 1;
        uint8_t evt_Acknowledge : 1;
        uint64_t evt_reserved : 38;
    };
} marlin_events_t;

typedef struct _marlin_mesh_t {
    float z[MARLIN_MAX_MESH_POINTS];
    uint8_t xc;
    uint8_t yc;
} marlin_mesh_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const char *marlin_events_get_name(uint8_t evt_id);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_MARLIN_EVENTS_H

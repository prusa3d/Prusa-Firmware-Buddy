// dump_marlinapi.h
#ifndef _DUMP_MARLINAPI_H
#define _DUMP_MARLINAPI_H

#include "dump.h"

//marlin api config
#define MARLIN_MAX_CLIENTS  3   // maximum number of clients registered in same time
#define MARLIN_MAX_REQUEST  100 // maximum request length in chars
#define MARLIN_SERVER_QUEUE 128 // size of marlin server input character queue (number of characters)
#define MARLIN_CLIENT_QUEUE 16  // size of marlin client input message queue (number of messages)

#define MARLIN_MAX_MESH_POINTS (4 * 4)
/*
typedef enum {
    mpsIdle = 0,
    mpsPrinting,
    mpsPausing_Begin,
    mpsPausing_WaitIdle,
    mpsPausing_ParkHead,
    mpsPaused,
    mpsResuming_Begin,
    mpsResuming_Reheating,
    mpsResuming_UnparkHead,
    mpsAborting_Begin,
    mpsAborting_WaitIdle,
    mpsAborting_ParkHead,
    mpsAborted,
    mpsFinishing_WaitIdle,
    mpsFinishing_ParkHead,
    mpsFinished,
} marlin_print_state_t;*/

#pragma pack(push)
#pragma pack(1)

typedef uint8_t marlin_print_state_t;

typedef struct _marlin_vars_t {
    uint8_t motion;                   // motion (bit0-X, bit1-Y, bit2-Z, bit3-E)
    uint8_t gqueue;                   // number of commands in gcode queue
    uint8_t pqueue;                   // number of commands in planner queue
    int32_t ipos[4];                  // integer position XYZE [steps]
    float pos[4];                     // position XYZE [mm]
    float temp_nozzle;                // nozzle temperature [C]
    float temp_bed;                   // bed temperature [C]
    float target_nozzle;              // nozzle target temperature [C]
    float target_bed;                 // bed target temperature [C]
    float z_offset;                   // probe z-offset [mm]
    uint8_t print_fan_speed;          // print fan speed [0..255]
    uint16_t print_speed;             // printing speed factor [%]
    uint16_t flow_factor;             // flow factor [%]
    uint8_t wait_heat;                // wait_for_heatup
    uint8_t wait_user;                // wait_for_user
    uint8_t sd_printing;              // card.flag.sdprinting
    uint8_t sd_percent_done;          // card.percentDone() [%]
    uint32_t print_duration;          // print_job_timer.duration() [ms]
    uint8_t media_inserted;           // media_is_inserted()
    marlin_print_state_t print_state; // marlin_server.print_state
    char *media_LFN;                  // Long-File-Name of the currently selected file - a pointer to a global static buffer
    char *media_SFN_path;             // Short-File-Name path to currently selected file - a pointer to a global static buffer
    float display_nozzle;             // nozzle temperature to display [C]
    uint32_t time_to_end;             // oProgressData.oTime2End.mGetValue() [s]
    //    uint16_t fan0_rpm;                // fanctl0.getActualRPM() [1/min]
    //    uint16_t fan1_rpm;                // fanctl1.getActualRPM() [1/min]
} marlin_vars_t;

typedef struct _marlin_mesh_t {
    float z[MARLIN_MAX_MESH_POINTS];
    uint8_t xc;
    uint8_t yc;
} marlin_mesh_t;

typedef struct _marlin_server_t {
    uint16_t flags;                              // server flags (MARLIN_SFLG)
    uint64_t notify_events[MARLIN_MAX_CLIENTS];  // event notification mask
    uint64_t notify_changes[MARLIN_MAX_CLIENTS]; // variable change notification mask
    marlin_vars_t vars;                          // cached variables
    char request[MARLIN_MAX_REQUEST];
    int request_len;
    uint64_t client_events[MARLIN_MAX_CLIENTS];      // client event mask
    uint64_t client_changes[MARLIN_MAX_CLIENTS];     // client variable change mask
    uint32_t last_update;                            // last update tick count
    uint8_t idle_cnt;                                // idle call counter
    uint8_t pqueue_head;                             // copy of planner.block_buffer_head
    uint8_t pqueue_tail;                             // copy of planner.block_buffer_tail
    uint8_t pqueue;                                  // calculated number of records in planner queue
    uint8_t gqueue;                                  // copy of queue.length - number of commands in gcode queue
    uint32_t command;                                // actually running command
    uint32_t command_begin;                          // variable for notification
    uint32_t command_end;                            // variable for notification
    marlin_mesh_t mesh;                              // meshbed leveling
    uint64_t mesh_point_notsent[MARLIN_MAX_CLIENTS]; // mesh point mask (points that are not sent)
    uint64_t update_vars;                            // variable update mask
    marlin_print_state_t print_state;                // printing state (printing, paused, ...)
    float resume_pos[4];                             // resume position for unpark_head
    float resume_nozzle_temp;                        // resume nozzle temperature
    uint8_t resume_fan_speed;                        // resume fan speed
    uint32_t paused_ticks;                           // tick count in moment when printing paused
} marlin_server_t;

typedef struct _marlin_client_t {
    uint8_t id;          // client id (0..MARLIN_MAX_CLIENTS-1)
    uint16_t flags;      // client flags (MARLIN_CFLG_xxx)
    uint64_t events;     // event mask
    uint64_t changes;    // variable change mask
    marlin_vars_t vars;  // cached variables
    uint32_t ack;        // cached ack value from last Acknowledge event
    uint16_t last_count; // number of messages received in last client loop
    uint64_t errors;
    marlin_mesh_t mesh; // meshbed leveling
    uint32_t command;   // processed command (G28,G29,M701,M702,M600)
    uint8_t reheating;  // reheating in progress
    uint32_t fsm_cb;    // to register callback for screen creation (M876), callback ensures M876 is processed asap, so there is no need for queue
} marlin_client_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern void dump_marlinapi_print(dump_t *pd, mapfile_t *pm);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_DUMP_MARLINAPI_H

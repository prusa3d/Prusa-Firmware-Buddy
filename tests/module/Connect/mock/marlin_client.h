#include <cstdint>

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
} marlin_print_state_t;

typedef struct _marlin_vars_t {
    // 4B base types
    float pos[4];                     // position XYZE [mm]
    int32_t ipos[4];                  // integer position XYZE [steps]
    float curr_pos[4];                // current position XYZE according to G-code [mm]
    float temp_nozzle;                // nozzle temperature [C]
    float temp_bed;                   // bed temperature [C]
    float target_nozzle;              // nozzle target temperature [C]
    float target_bed;                 // bed target temperature [C]
    float z_offset;                   // probe z-offset [mm]
    float display_nozzle;             // nozzle temperature to display [C]
    float travel_acceleration;        // travel acceleration from planner
    uint32_t print_duration;          // print_job_timer.duration() [ms]
    uint32_t time_to_end;             // oProgressData.oTime2End.mGetValue() [s]
    char *media_LFN;                  // Long-File-Name of the currently selected file - a pointer to a global static buffer
    char *media_SFN_path;             // Short-File-Name path to currently selected file - a pointer to a global static buffer
    marlin_print_state_t print_state; // marlin_server.print_state

    // 2B base types
    uint16_t print_speed;       // printing speed factor [%]
    uint16_t flow_factor;       // flow factor [%]
    uint16_t print_fan_rpm;     // fanCtlPrint.getActualRPM() [1/min]
    uint16_t heatbreak_fan_rpm; // fanCtlHeatBreak.getActualRPM() [1/min]

    // 1B base types
    uint8_t motion;              // motion (bit0-X, bit1-Y, bit2-Z, bit3-E)
    uint8_t gqueue;              // number of commands in gcode queue
    uint8_t pqueue;              // number of commands in planner queue
    uint8_t print_fan_speed;     // print fan speed [0..255]
    uint8_t wait_heat;           // wait_for_heatup
    uint8_t wait_user;           // wait_for_user
    uint8_t sd_printing;         // card.flag.sdprinting
    uint8_t sd_percent_done;     // card.percentDone() [%]
    uint8_t media_inserted;      // media_is_inserted()
    uint8_t fan_check_enabled;   // fan_check [on/off]
    uint8_t fs_autoload_enabled; // fs_autoload [on/off]
} marlin_vars_t;

#pragma once
#include <stdint.h>

static constexpr float MMU2_EXTRUDER_HEATBREAK_LENGTH = 67.F;
static constexpr float MMU2_EXTRUDER_NOZZLE_LENGTH = 20.F;
static constexpr float MMU2_VERIFY_LOAD_TO_NOZZLE_FEED_RATE = 50.F;
// mm used to shorten/lenghten (negative number -> shorten) the distange of verify load to nozzle
static constexpr float MMU2_VERIFY_LOAD_TO_NOZZLE_TWEAK = -10.F - 25.F;

#define DEFAULT_SAFETYTIMER_TIME_MINS 30

///////////////////////////////////////////

// MMU Error pause position
#define MMU_ERR_X_PAUSE_POS  125
#define MMU_ERR_Y_PAUSE_POS  0
#define MMU_ERR_Z_PAUSE_LIFT 20

// As discussed with our PrusaSlicer profile specialist
// - ToolChange shall not try to push filament into the very tip of the nozzle
// to have some space for additional G-code to tune the extruded filament length
// in the profile
// Beware - this value is used to initialize the MMU logic layer - it will be sent to the MMU upon line up (written into its 8bit register 0x0b)
// However - in the G-code we can get a request to set the extra load distance at runtime to something else (M708 A0xb Xsomething).
// The printer intercepts such a call and sets its extra load distance to match the new value as well.
// The same applies to Pulley slow feed rate (register 0x14)
static constexpr uint8_t MMU2_TOOL_CHANGE_LOAD_LENGTH = 5; // mm
static constexpr float MMU2_LOAD_TO_NOZZLE_FEED_RATE = 20.0F; // mm/s

static constexpr float MMU2_UNLOAD_TO_FINDA_FEED_RATE = 120.0F; // mm/s

// The first the MMU does is initialise its axis. Meanwhile the E-motor will unload 20mm of filament in approx. 1 second.
static constexpr float MMU2_RETRY_UNLOAD_TO_FINDA_LENGTH = 20.0f; // mm
static constexpr float MMU2_RETRY_UNLOAD_TO_FINDA_FEED_RATE = 20.0f; // mm/s

static constexpr float MMU2_RETRY_UNLOAD_FINISH_LENGTH = -40.0f; // mm
static constexpr float MMU2_RETRY_UNLOAD_FINISH_FEED_RATE = 20.0f; // mm/s

// @@TODO remove this and enable it in the configuration files
// Settings for filament load / unload from the LCD menu.
// This is for Prusa MK3-style extruders. Customize for your hardware.
#define MMU2_FILAMENTCHANGE_EJECT_FEED 80.0F
#define MMU2_LOAD_TO_NOZZLE_SEQUENCE                                       \
    {                                                                      \
        { MMU2_EXTRUDER_HEATBREAK_LENGTH, MMU2_LOAD_TO_NOZZLE_FEED_RATE }, \
        { MMU2_EXTRUDER_NOZZLE_LENGTH, 5.F }                               \
    }

static constexpr uint8_t MMU2_NO_TOOL = 99;
static constexpr uint32_t MMU_BAUD = 115200;

typedef float feedRate_t;

struct E_Step {
    float extrude; ///< extrude distance in mm
    feedRate_t feedRate; ///< feed rate in mm/s
};

static constexpr E_Step ramming_sequence[] = FILAMENT_MMU2_RAMMING_SEQUENCE;
static constexpr E_Step load_to_nozzle_sequence[] = MMU2_LOAD_TO_NOZZLE_SEQUENCE;

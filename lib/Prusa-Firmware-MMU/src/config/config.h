/// @file config.h
#pragma once
#include <stdint.h>
#include "axis.h"

/// Define Debug mode to add additional serial output

//#define DEBUG_FINDA
/// Enable DEBUG_LOGIC to compile debugging and error messages (beware of code base size ;) ) for the logic layer
//#define DEBUG_LOGIC

/// Enable DEBUG_MODULES to compile debugging and error messages (beware of code base size ;) ) for the modules layer
//#define DEBUG_MODULES

/// Enable DEBUG_HAL to compile debugging and error messages (beware of code base size ;) ) for the HAL layer
//#define DEBUG_HAL

/// Wrangler for assorted compile-time configuration and constants.
namespace config {

/// Max number of extruders/tools/slots
/// Beware - if you change this, the EEPROM structure will become invalid and no migration procedures have been implemented.
/// So if you really have to change this, erase your EEPROM content then.
static constexpr const uint8_t toolCount = 5U;
static_assert(toolCount < 15, "Up to 14 valid slots (+1 parking) is supported in EEPROM storage");

// Printer's filament sensor setup
static constexpr const uint16_t fsensorDebounceMs = 2;

// LEDS
/// The complete period of LED's blinking (i.e. ON and OFF together)
/// Beware - in order to keep the blink periods "handle" millis overflow seamlessly
/// keep the period a power of 2 (i.e. 256, 512, 1024).
/// If you don't, one of the LED unit tests will fail.
static constexpr uint16_t ledBlinkPeriodMs = 1024U;
static_assert(ledBlinkPeriodMs == 256 || ledBlinkPeriodMs == 512 || ledBlinkPeriodMs == 1024 || ledBlinkPeriodMs == 2048, "LED blink period should be a power of 2");

// FINDA setup
static constexpr const uint16_t findaDebounceMs = 100;

// Buttons setup
static constexpr const uint8_t buttonCount = 3; ///< number of buttons currently supported
static constexpr const uint16_t buttonsDebounceMs = 20; ///< tuned with a pack of highly trained monkeys :)
static constexpr const uint16_t buttonADCLimits[buttonCount][2] = { { 0, 50 }, { 80, 100 }, { 160, 180 } };
static constexpr const uint16_t buttonADCMaxValue = 1023; ///< used in unit tests
static constexpr const uint8_t buttonsADCIndex = 5; ///< ADC index of buttons input

// VCC measurement setup
static constexpr const uint8_t VCCADCIndex = 30; ///< ADC index of scaled VCC input
static constexpr const uint16_t VCCADCThreshold = 274; ///< ADC value for triggering the UV_VCC error
/// We are measuring the bandgap voltage, Vb=1.1V.
/// To compute the threshold value: `VAL = 1125.3 / AVCC`
/// So for AVCC=4.1V, you get VAL=274.46
static constexpr const uint8_t VCCADCReadCnt = 10; ///< Number of ADC reads to perform, only the last one being used

// Motion and planning

/// Do not plan moves equal or shorter than the requested steps
static constexpr uint8_t dropSegments = 0;

/// Max step frequency 40KHz
static constexpr uint16_t maxStepFrequency = 40000;

/// Minimum stepping rate 120Hz
static constexpr uint16_t minStepRate = 120;

/// Size for the motion planner block buffer size
static constexpr uint8_t blockBufferSize = 4;

/// Step timer frequency divider (F = F_CPU / divider)
static constexpr uint8_t stepTimerFrequencyDivider = 8;

/// Smallest stepping ISR scheduling slice (T = 1 / (F_CPU / divider) * quantum)
/// 25us is the max frequency interval per maxStepFrequency attainable for a single axis
/// while accelerating: with 3 axes this yields a required minimum of 75us
static constexpr uint16_t stepTimerQuantum = 256; // 256 = 128us

/// Max retries of FeedToBondtech used in LoadFilament
static constexpr uint8_t feedToBondtechMaxRetries = 2;

/// Distances
static constexpr U_mm pulleyToCuttingEdge = 33.0_mm; /// 33.0_mm /// Pulley to cutting edge.
/// Case 1: FINDA working: This should be the max retraction after FINDA un-triggers.
/// Case 2: FINDA not working: calculate retraction from printer to this point.
static constexpr U_mm filamentMinLoadedToMMU = 20.0_mm; /// Limit of retraction.
static constexpr U_mm ejectFromCuttingEdge = 40.0_mm; /// Eject should ignore FilamentMinLoadedToMMU and retract
static constexpr U_mm cuttingEdgeRetract = 5.0_mm; /// Cutting retraction distance (filament should be flush with outlet)
static constexpr U_mm cuttingEdgeToFinda = 18.5_mm; /// Cutting edge to FINDA MMU side -1mm tolerance should be ~18.5. FINDA shouldn't trigger here.
static constexpr U_mm findaTriggerDistance = 4.5_mm; /// FINDA trigger distance +1.0_mm tolerance.
static constexpr U_mm cuttingEdgeToFindaMidpoint = 22.85_mm; /// Cutting edge to Midpoint of FINDA should be 22.85_mm.
static constexpr U_mm findaToCoupler = 12.0_mm; /// FINDA Coupler side to coupler screw.
static constexpr U_mm couplerToBowden = 3.5_mm; /// FINDA Coupler screw to bowden mmu side (in coupling).

// Min, max and default bowden length setup
static constexpr U_mm defaultBowdenLength = 360.0_mm; /// ~360.0_mm - Default Bowden length.
static constexpr U_mm minimumBowdenLength = 341.0_mm; /// ~341.0_mm - Minimum bowden length.
static constexpr U_mm maximumBowdenLength = 1000.0_mm; /// ~1000.0_mm - Maximum bowden length.
static_assert(minimumBowdenLength.v <= defaultBowdenLength.v);
static_assert(maximumBowdenLength.v > defaultBowdenLength.v);

static constexpr U_mm feedToFinda = cuttingEdgeToFindaMidpoint + filamentMinLoadedToMMU;
static constexpr U_mm maximumFeedToFinda = feedToFinda + 20.0_mm; ///< allow for some safety margin to load to FINDA
static constexpr U_mm pulleyHelperMove = 10.0_mm; ///< Helper move for Load/Unload error states - when the MMU should slowly move the filament a bit
static constexpr U_mm cutLength = 8.0_mm;
static constexpr U_mm fsensorToNozzle = 30.0_mm; ///< ~20mm from MK4's filament sensor through extruder gears into nozzle
static constexpr U_mm fsensorToNozzleAvoidGrind = 5.0_mm;
/// Check the state of FSensor after this amount of filament got (hopefully) pulled out while unloading.
static constexpr U_mm fsensorUnloadCheckDistance = 40.0_mm;

/// Begin: Pulley axis configuration
static constexpr AxisConfig pulley = {
    .dirOn = false,
    .mRes = MRes_8,
    .iRun = 13, /// 230mA
    .iHold = 0, /// 17mA in SpreadCycle, freewheel in StealthChop
    .stealth = false,
    .stepsPerUnit = (200 * 8 / 19.147274),
    .sg_thrs = 8,
};

/// Pulley motion limits
static constexpr PulleyLimits pulleyLimits = {
    .lenght = 1000.0_mm, // TODO
    .jerk = 4.0_mm_s,
    .accel = 800.0_mm_s2,
};

static constexpr U_mm_s pulleyUnloadFeedrate = 95._mm_s;
/// 120mm_s is too much, the printer cannot send the status of fsensor that fast
/// and false fsensor_not_triggered errors start to occur
static constexpr U_mm_s pulleyLoadFeedrate = 95._mm_s;
static constexpr U_mm_s pulleySlowFeedrate = 20._mm_s;
/// End: Pulley axis configuration

/// Begin: Selector configuration
static constexpr AxisConfig selector = {
    .dirOn = true,
    .mRes = MRes_8,
    .iRun = 31, /// 530mA
    .iHold = 0, /// 17mA in SpreadCycle, freewheel in StealthChop
    .stealth = false,
    .stepsPerUnit = (200 * 8 / 8.),
    .sg_thrs = 3,
};

static constexpr uint8_t selectorCutIRun = 40; ///< 660mA

/// Selector motion limits
static constexpr SelectorLimits selectorLimits = {
    .lenght = 75.0_mm, // TODO how does this relate to SelectorOffsetFromMin?
    .jerk = 1.0_mm_s,
    .accel = 200.0_mm_s2,
};

static constexpr U_mm SelectorSlotDistance = 14.0_mm; /// Selector distance between two slots
static constexpr U_mm SelectorOffsetFromMax = 1.0_mm; /// Selector offset from home max to slot 0
static constexpr U_mm SelectorOffsetFromMin = 75.5_mm; /// Selector offset from home min to slot 0

/// slots 0-4 are the real ones, the 5th is the farthest parking positions
/// selector.dirOn = true = Home at max: selector hits left side of the MMU body
/// selector.dirOn = false = Home at min: selector POM nut hit the selector motor
static constexpr U_mm selectorSlotPositions[toolCount + 1] = {

    ///selector max positions
    SelectorOffsetFromMax + 0 * SelectorSlotDistance, ///1.0_mm + 0 * 14.0_mm = 1.0_mm
    SelectorOffsetFromMax + 1 * SelectorSlotDistance, ///1.0_mm + 1 * 14.0_mm = 15.0_mm
    SelectorOffsetFromMax + 2 * SelectorSlotDistance, ///1.0_mm + 2 * 14.0_mm = 29.0_mm
    SelectorOffsetFromMax + 3 * SelectorSlotDistance, ///1.0_mm + 3 * 14.0_mm = 43.0_mm
    SelectorOffsetFromMax + 4 * SelectorSlotDistance, ///1.0_mm + 4 * 14.0_mm = 57.0_mm
    SelectorOffsetFromMax + 5 * SelectorSlotDistance ///1.0_mm + 5 * 14.0_mm = 71.0_mm

    ///selector min positions
    //    SelectorOffsetFromMin - 1.0_mm - 0 * SelectorSlotDistance, ///75.5_mm - 1.0_mm - 0 * 14.0_mm = 74.5_mm
    //    SelectorOffsetFromMin - 1.0_mm - 1 * SelectorSlotDistance, ///75.5_mm - 1.0_mm - 1 * 14.0_mm = 60.5_mm
    //    SelectorOffsetFromMin - 1.0_mm - 2 * SelectorSlotDistance, ///75.5_mm - 1.0_mm - 2 * 14.0_mm = 46.5_mm
    //    SelectorOffsetFromMin - 1.0_mm - 3 * SelectorSlotDistance, ///75.5_mm - 1.0_mm - 3 * 14.0_mm = 32.5_mm
    //    SelectorOffsetFromMin - 1.0_mm - 4 * SelectorSlotDistance, ///75.5_mm - 1.0_mm - 4 * 14.0_mm = 18.5_mm
    //    SelectorOffsetFromMin - 1.0_mm - 5 * SelectorSlotDistance ///75.5_mm - 1.0_mm - 5 * 14.0_mm = 4.5_mm
};

static constexpr U_mm_s selectorFeedrate = 45._mm_s;
static constexpr U_mm_s selectorHomingFeedrate = 30._mm_s;
/// End: Selector configuration

/// Begin: Idler configuration
static constexpr AxisConfig idler = {
    .dirOn = true,
    .mRes = MRes_16,
    .iRun = 31, /// 530mA
    .iHold = 5, /// 99mA - parked current
    .stealth = false,
    .stepsPerUnit = (200 * 16 / 360.),
    .sg_thrs = 7,
};

/// Idler motion limits
static constexpr IdlerLimits idlerLimits = {
    .lenght = 225.0_deg,
    .jerk = 0.1_deg_s,
    .accel = 500.0_deg_s2,
};

static constexpr U_deg IdlerSlotDistance = 40.0_deg; /// Idler distance between two slots
static constexpr U_deg IdlerOffsetFromHome = 18.0_deg; /// Idler offset from home to slots

/// Absolute positions for Idler's slots: 0-4 are the real ones, the 5th index is the idle position
/// Home ccw with 5th idler bearing facing selector
static constexpr U_deg idlerSlotPositions[toolCount + 1] = {
    IdlerOffsetFromHome + 5 * IdlerSlotDistance, /// Slot 0 at 218 degree after homing ///18.0_deg + 5 * 40.0_deg = 218.0_deg
    IdlerOffsetFromHome + 4 * IdlerSlotDistance, /// Slot 1 at 178 degree after homing ///18.0_deg + 4 * 40.0_deg = 178.0_deg
    IdlerOffsetFromHome + 3 * IdlerSlotDistance, /// Slot 2 at 138 degree after homing ///18.0_deg + 3 * 40.0_deg = 138.0_deg
    IdlerOffsetFromHome + 2 * IdlerSlotDistance, /// Slot 3 at 98 degree after homing ///18.0_deg + 2 * 40.0_deg = 98.0_deg
    IdlerOffsetFromHome + 1 * IdlerSlotDistance, /// Slot 4 at 58 degree after homing ///18.0_deg + 1 * 40.0_deg = 58.0_deg
    IdlerOffsetFromHome ///18.0_deg Fully disengaged all slots
};

/// Intermediate positions for Idler's slots: 0-4 are the real ones, the 5th index is the idle position
static constexpr U_deg idlerIntermediateSlotPositions[toolCount + 1] = {
    IdlerOffsetFromHome + 4.75F * IdlerSlotDistance,
    IdlerOffsetFromHome + 3.75F * IdlerSlotDistance,
    IdlerOffsetFromHome + 2.75F * IdlerSlotDistance,
    IdlerOffsetFromHome + 1.75F * IdlerSlotDistance,
    IdlerOffsetFromHome + 0.75F * IdlerSlotDistance,
    IdlerOffsetFromHome ///18.0_deg Fully disengaged all slots
};

static constexpr U_deg idlerParkPositionDelta = -IdlerSlotDistance + 5.0_deg / 2; // TODO verify

static constexpr U_deg_s idlerFeedrate = 300._deg_s;
static constexpr U_deg_s idlerHomingFeedrate = 265._deg_s;
/// End: Idler configuration

// TMC2130 setup

// static constexpr int8_t tmc2130_sg_thrs = 3;
// static_assert(tmc2130_sg_thrs >= -64 && tmc2130_sg_thrs <= 63, "tmc2130_sg_thrs out of range");

static constexpr uint32_t tmc2130_coolStepThreshold = 450; ///< step-based 20bit uint
static_assert(tmc2130_coolStepThreshold <= 0xfffff, "tmc2130_coolStepThreshold out of range");

static constexpr uint32_t tmc2130_PWM_AMPL = 240;
static_assert(tmc2130_PWM_AMPL <= 255, "tmc2130_PWM_AMPL out of range");

static constexpr uint32_t tmc2130_PWM_GRAD = 4;
static_assert(tmc2130_PWM_GRAD <= 255, "tmc2130_PWM_GRAD out of range");

static constexpr uint32_t tmc2130_PWM_FREQ = 0;
static_assert(tmc2130_PWM_FREQ <= 3, "tmc2130_PWM_GRAD out of range");

static constexpr uint32_t tmc2130_PWM_AUTOSCALE = 1;
static_assert(tmc2130_PWM_AUTOSCALE <= 1, "tmc2130_PWM_AUTOSCALE out of range");

/// Freewheel options for standstill:
/// 0: Normal operation (IHOLD is supplied to the motor at standstill)
/// 1: Freewheeling (as if the driver was disabled, no braking except for detent torque)
/// 2: Coil shorted using LS drivers (stronger passive braking)
/// 3: Coil shorted using HS drivers (weaker passive braking)
static constexpr uint32_t tmc2130_freewheel = 1;
static_assert(tmc2130_PWM_AUTOSCALE <= 3, "tmc2130_freewheel out of range");

} // namespace config

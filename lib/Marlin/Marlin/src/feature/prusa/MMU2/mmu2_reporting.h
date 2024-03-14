/// @file mmu2_reporting.h

#pragma once
#include <stdint.h>
#ifdef __AVR__
    #include "mmu2/error_codes.h"
    #include "mmu2/progress_codes.h"
#else
    #include "../../../../../../Prusa-Firmware-MMU/src/logic/error_codes.h"
    #include "../../../../../../Prusa-Firmware-MMU/src/logic/progress_codes.h"
#endif

namespace MMU2 {

enum CommandInProgress : uint8_t {
    NoCommand = 0,
    CutFilament = 'K',
    EjectFilament = 'E',
    Homing = 'H',
    LoadFilament = 'L',
    Reset = 'X',
    ToolChange = 'T',
    UnloadFilament = 'U',
    TestLoad = 't'
};

/// Called at the begin of every MMU operation
void BeginReport(CommandInProgress cip, ProgressCode ec);

/// Called at the end of every MMU operation
void EndReport(CommandInProgress cip, ProgressCode ec);

/// Checks for error screen user input, if the error screen is open
void CheckErrorScreenUserInput();

/// Return true if the error screen is sleeping in the background
/// Error screen sleeps when the firmware is rendering complementary
/// UI to resolve the error screen, for example tuning Idler Stallguard Threshold
bool TuneMenuEntered();

/// @brief Called when the MMU or MK3S sends operation error (even repeatedly).
/// Render MMU error screen on the LCD. This must be non-blocking
/// and allow the MMU and printer to communicate with each other.
/// @param[in] ec error code
/// @param[in] es error source
void ReportErrorHook(CommandInProgress cip, ErrorCode ec, uint8_t es);

/// Called when the MMU sends operation progress update
void ReportProgressHook(CommandInProgress cip, ProgressCode ec);

struct TryLoadUnloadReporter {
    TryLoadUnloadReporter(float delta_mm);
    void Progress(bool sensorState);
    void DumpToSerial();

private:
    /// @brief Add one block to the progress bar
    /// @param col pixel position on the LCD status line, should range from 0 to (LCD_WIDTH - 1)
    /// @param sensorState if true, filament is not present, else filament is present. This controls which character to render
    void Render(uint8_t col, bool sensorState);

    uint8_t dpixel1;
    uint8_t dpixel0;
    // The total length is twice delta_mm. Divide that length by number of pixels
    // available to get length per pixel.
    // Note: Below is the reciprocal of (2 * delta_mm) / LCD_WIDTH [mm/pixel]
    float pixel_per_mm;
    uint8_t lcd_cursor_col;

    // Beware: needs to be a union to optimize for the 8bit better
    union __attribute__((packed)) PU {
        uint32_t dw;
        uint8_t bytes[4];
        constexpr PU()
            : dw(0) {}
        constexpr PU(uint32_t dw)
            : dw(dw) {}
    } progress;
    static_assert(sizeof(PU) == 4);
};

/// Remders the sensor status line. Also used by the "resume temperature" screen.
void ReportErrorHookDynamicRender();

/// Renders the static part of the sensor state line. Also used by "resuming temperature screen"
void ReportErrorHookSensorLineRender();

/// @returns true if the MMU is communicating and available
/// can change at runtime
bool MMUAvailable();

/// Global Enable/Disable use MMU (to be stored in EEPROM)
bool UseMMU();

/// Disables MMU in EEPROM
void DisableMMUInSettings();

/// Increments EEPROM cell - number of failed loads into the nozzle
/// Note: technically, this is not an MMU error but an error of the printer.
void IncrementLoadFails();

/// Increments EEPROM cell - number of MMU errors
void IncrementMMUFails();

/// @returns true when Cutter is enabled in the menus
bool cutter_enabled();

// Beware: enum values intentionally chosen to match the 8bit FW to save code size
enum SoundType {
    Prompt = 2,
    Confirm = 3
};

void MakeSound(SoundType s);

void FullScreenMsgCut(uint8_t slot);
void FullScreenMsgEject(uint8_t slot);
void FullScreenMsgTest(uint8_t slot);
void FullScreenMsgLoad(uint8_t slot);
void FullScreenMsgRestoringTemperature();

void ScreenUpdateEnable();
void ScreenClear();

void tuneIdlerStallguardThreshold();

} // namespace MMU2

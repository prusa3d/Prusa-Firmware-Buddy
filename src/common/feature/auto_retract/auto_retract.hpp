#pragma once

namespace buddy {

/// Class for managing automatic retraction after print or load, so that the printer keeps the nozzle empty for MBL and non-printing to prevent oozing.
/// Only to be managed from the marlin thread
class AutoRetract {
    friend AutoRetract &auto_retract();

public:
    /// \returns whether the specified hotend is auto-retracted (and will need de-retracting on print)
    bool is_retracted(uint8_t hotend) const;

    bool is_retracted() const;

    /// How much is the filament retracted from the nozzle (mm)
    float retracted_distance() const;

    /// Marks the specified \param hotend as \param retracted
    void mark_as_retracted(uint8_t hotend, bool retracted);

    /// If !is_retracted, executes the retraction process and marks the currently active hotend as retracted
    void maybe_retract_from_nozzle();

    /// If is_retracted, executes the deretraction process and marks the currently active hotend as not retracted
    void maybe_deretract_to_nozzle();

private:
    AutoRetract();

    /// Common checks for retract & deretract
    bool can_perform_action() const;

    bool is_checking_deretract_ = false;

    /// Shadows the config_store variable to reduce mutex locking
    std::bitset<HOTENDS> retracted_hotends_bitset_ = 0;
};

AutoRetract &auto_retract();

} // namespace buddy

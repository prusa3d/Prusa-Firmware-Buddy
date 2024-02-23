#pragma once
#include "inc/MarlinConfigPre.h"

#if ENABLED(CRASH_RECOVERY)

    #include <array>
    #include <stddef.h>

    #include "utility_extensions.hpp"

struct Crash_s_Counters {

public:
    enum class Counter : uint8_t {
        axis_crash_x,
        axis_crash_y,
        power_panic,
        _cnt
    };
    static constexpr size_t value_count = ftrstd::to_underlying(Counter::_cnt);

public:
    using Value = uint16_t;
    using Data = std::array<Value, value_count>;

public:
    /// \returns the local \p counter value since the reset
    /// (reset happens on the beginning of the prints)
    inline Value get(Counter counter) const {
        return data_[ftrstd::to_underlying(counter)];
    }

    void increment(Counter counter);

public:
    /// Resets the counters completely.
    /// This function is supposed to be called only on the beginning of a print
    void reset();

    /// \returns data for backup - used for restoring the counters after power panic
    const Data &backup_data();

    /// Restores previous backup of counters data stored in \p data
    void restore_data(const Data &data);

    /// Saves the counters to eeprom (if they have not been saved yet).
    /// Does NOT RESET the counters (but marks them as saved and they will be reset on next increase possibly)
    void save_to_eeprom();

private:
    Data data_ {};

    /// When set to true, denotes that the current counter values have bene saved to eeprom (and thus should not be saved there again).
    /// The counters should not be increased when this is set to true
    bool saved_to_eeprom_ = false;
};

#endif

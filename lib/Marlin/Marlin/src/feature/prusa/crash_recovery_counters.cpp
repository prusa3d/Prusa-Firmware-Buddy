#include "crash_recovery_counters.hpp"

#if ENABLED(CRASH_RECOVERY)

    #include "metric.h"
    #include "config_store/store_instance.hpp"

void Crash_s_Counters::reset() {
    saved_to_eeprom_ = false;
    data_ = {};
}

const Crash_s_Counters::Data &Crash_s_Counters::backup_data() {
    // Make sure we're providing valid data
    if (saved_to_eeprom_) {
        reset();
    }

    return data_;
}

void Crash_s_Counters::restore_data(const std::array<Value, value_count> &data) {
    saved_to_eeprom_ = false;
    data_ = data;
}

void Crash_s_Counters::increment(Counter counter) {
    // New value, but we've stored the previous values in the eeprom -> reset the counters
    if (saved_to_eeprom_) {
        reset();
    }

    data_[ftrstd::to_underlying(counter)]++;
}

void Crash_s_Counters::save_to_eeprom() {
    if (saved_to_eeprom_) {
        return;
    }

    saved_to_eeprom_ = true;

    // Save & report axis crashes
    for (int i = 0; i < 2; i++) {
        const auto counter_var = data_[i + ftrstd::to_underlying(Counter::axis_crash_x)];
        if (counter_var == 0) {
            return;
        }

        const AxisEnum axis = AxisEnum(i + int(X_AXIS));

        Value total = counter_var;
        if (axis == X_AXIS) {
            total += config_store().crash_count_x.get();
            config_store().crash_count_x.set(total);
        } else {
            total += config_store().crash_count_y.get();
            config_store().crash_count_y.set(total);
        }

        METRIC_DEF(crash_stat, "crash_stat", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
        metric_record_custom(&crash_stat, ",axis=%c last=%ui,total=%ui", axis_codes[axis], counter_var, total);
    }

    // Save power panics
    config_store().power_panics_count.set(config_store().power_panics_count.get() + get(Counter::power_panic));
}

#endif

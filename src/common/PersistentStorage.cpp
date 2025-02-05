/**
 * @file
 */

#include "PersistentStorage.h"
#include <stdint.h>
#include <stddef.h>

void PersistentStorage::pushHomeSample(uint16_t mscnt, uint8_t axis) {
    static constexpr auto cnt = config_store_ns::CurrentStore::precise_homing_axis_sample_count;
    const auto index = config_store().precise_homing_sample_history_index.get(axis);
    config_store().precise_homing_sample_history.set(axis * cnt + index, mscnt);
    config_store().precise_homing_sample_history_index.set(axis, (index + 1) % cnt);
}

bool PersistentStorage::isCalibratedHome(uint16_t (&mscnt)[homeSamplesCount], uint8_t axis) {
    static constexpr auto cnt = config_store_ns::CurrentStore::precise_homing_axis_sample_count;
    const auto offset = axis * cnt;

    bool is_calibrated = true;
    auto &store_array = config_store().precise_homing_sample_history;

    for (uint_fast8_t i = 0; i < homeSamplesCount; ++i) {
        auto sample = store_array.get(i + offset);

        if (sample == store_array.get_default_val(i + offset)) {
            is_calibrated = false;
            sample = 0;
        }

        mscnt[i] = sample;
    }
    return is_calibrated;
}

void PersistentStorage::erase_axis(uint8_t axis) {
    config_store().precise_homing_sample_history_index.set_to_default(axis);

    static constexpr auto cnt = config_store_ns::CurrentStore::precise_homing_axis_sample_count;
    for (int i = cnt * axis, e = cnt * axis + cnt; i < e; i++) {
        config_store().precise_homing_sample_history.set_to_default(i);
    }
}

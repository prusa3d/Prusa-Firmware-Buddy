/**
 * @file
 */

#include "PersistentStorage.h"
#include "crc32.h"
#include "st25dv64k.h"
#include <stdint.h>
#include <stddef.h>
#include <array>
#include <config_store/backend_instance.hpp>

namespace {

constexpr size_t axisCount = 2;

struct HomeSample {
    uint8_t run : 4;
    uint16_t mscnt : 12;
    uint8_t board_temp;
    uint8_t crc8;
};

static_assert(sizeof(HomeSample) == 4, "Doesn't fit 4 bytes.");

struct Data {
    uint8_t reserved_for_future_use[8];
    HomeSample homeSamples[axisCount][PersistentStorage::homeSamplesCount];
};
static_assert(sizeof(Data) < config_store_ns::start_address, "Data collides with space used by config_store");

const Data *data = reinterpret_cast<Data *>(0x0);

struct IndexRun {
    uint_fast8_t index;
    uint_fast8_t run;
    bool error;
};

} // end anonymous namespace

static decltype(HomeSample::crc8) calcCrc(HomeSample homeSample) {
    return crc32_calc(reinterpret_cast<uint8_t *>(&homeSample), (sizeof(homeSample) - sizeof(homeSample.crc8)));
}

static bool isValid(HomeSample homeSample) {
    const bool validRun = ((0 == homeSample.run) || (1 == homeSample.run));
    const decltype(homeSample.crc8) calculatedCrc = calcCrc(homeSample);
    const bool validCrc = (calculatedCrc == homeSample.crc8);
    return (validRun && validCrc);
}

static void readHomeSamples(HomeSample (&homeSamples)[PersistentStorage::homeSamplesCount], uint_fast8_t axis) {

    st25dv64k_user_read_bytes(reinterpret_cast<uint32_t>(data->homeSamples[axis]), &homeSamples, sizeof(homeSamples));
}

static IndexRun getNextHomeSampleIndexRun(uint_fast8_t axis) {
    if (axis >= axisCount) {
        return { 0, 0, true };
    }

    HomeSample homeSamplesRead[PersistentStorage::homeSamplesCount];
    readHomeSamples(homeSamplesRead, axis);

    /// return first invalid sample, assume run 0.
    for (uint_fast8_t i = 0; i < PersistentStorage::homeSamplesCount; ++i) {
        if (!isValid(homeSamplesRead[i])) {
            return { i, 0, false };
        }
    }

    /// return position to rewrite - first sample of previous run
    if (0 == homeSamplesRead[0].run) { // run 0. already started
        for (uint_fast8_t i = 1; i < PersistentStorage::homeSamplesCount; ++i) {
            if (1 == homeSamplesRead[i].run) {
                return { i, 0, false }; // return position of first sample from previous run
            }
        }
        // run 0. finished, start 1. run
        return { 0, 1, false };
    } else { // run 1. already started
        for (uint_fast8_t i = 1; i < PersistentStorage::homeSamplesCount; ++i) {
            if (0 == homeSamplesRead[i].run) {
                return { i, 1, false }; // return position of first sample from previous run
            }
        }
        // run 1. finished, start 0. run
        return { 0, 0, false };
    }
}

void PersistentStorage::pushHomeSample(uint16_t mscnt, uint8_t board_temp, uint8_t axis) {
    const IndexRun indexRun = getNextHomeSampleIndexRun(axis);
    if (indexRun.error) {
        return;
    }

    HomeSample homeSample;
    homeSample.run = indexRun.run;
    homeSample.mscnt = mscnt;
    homeSample.board_temp = board_temp;
    homeSample.crc8 = calcCrc(homeSample);

    st25dv64k_user_write_bytes(reinterpret_cast<uint32_t>(&(data->homeSamples[axis][indexRun.index])), &homeSample, sizeof(homeSample));
}

bool PersistentStorage::isCalibratedHome(uint16_t (&mscnt)[homeSamplesCount], uint8_t axis) {
    HomeSample homeSamplesRead[PersistentStorage::homeSamplesCount];
    readHomeSamples(homeSamplesRead, axis);
    bool isCalibrated = true;

    for (uint_fast8_t i = 0; i < PersistentStorage::homeSamplesCount; ++i) {
        if (!isValid(homeSamplesRead[i])) {
            isCalibrated = false;
            mscnt[i] = 0;
        } else {
            mscnt[i] = homeSamplesRead[i].mscnt;
        }
    }
    return isCalibrated;
}

template <size_t N>
static constexpr std::array<uint8_t, N> generate_eeprom_erase_data() {
    std::array<uint8_t, N> ret;
    ret.fill(0xFF);
    return ret;
}

void PersistentStorage::erase_axis(uint8_t axis) {
    static constexpr auto empty_arr = generate_eeprom_erase_data<sizeof(Data::homeSamples[0])>();

    st25dv64k_user_unverified_write_bytes(reinterpret_cast<uint32_t>(&(data->homeSamples[axis])), empty_arr.begin(), empty_arr.size());
}

void PersistentStorage::erase() {
    erase_axis(0);
    erase_axis(1);
}

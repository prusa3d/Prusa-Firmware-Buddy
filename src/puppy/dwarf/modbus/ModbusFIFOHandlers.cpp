#include "ModbusFIFOHandlers.hpp"

#include "logging/log_dest_bufflog.h"
#include "puppies/fifo_encoder.hpp"
#include "../loadcell.hpp"
#include "accelerometer.hpp"
#include "Marlin/src/module/prusa/accelerometer_utils.h"
#include "bsod.h"

using namespace common::puppies::fifo;

LOG_COMPONENT_DEF(ModbusFIFOHandlers, LOG_SEVERITY_DEBUG);

/**
 * @brief Holds number of sent accelerometer samples; used for rate-limiting of
 * frequency report
 */
static int acc_sample_counter = 0;

/**
 * @brief Failed to encode
 *
 * to be used to handle impossible cases
 * where we already checked that it can_encode()
 * our type and actual encoding failed.
 *
 * This might catch programming error like
 * can_encode was invoked with different type
 * then we actually try to encode.
 *
 */
static void failed_to_encode() {
    bsod("Failed to encode.");
}

/**
 * @brief Pick accelerometer sample and encode it
 *
 * @param encoder
 * @param encoded Sets to true if sample encoded, left untouched otherwise
 *                to allow daisy chaining operations. Do not pass uninitialized
 *                value.
 */
static void pickup_accelerometer_sample(Encoder &encoder, bool &encoded) {
    dwarf::accelerometer::AccelerometerRecord record;
    if (encoder.can_encode<AccelerometerFastData>() && (dwarf::accelerometer::get_num_samples() >= std::tuple_size<AccelerometerFastData>::value)) {
        AccelerometerFastData accelerometer_data;
        for (auto &xyzSample : accelerometer_data) {
            if (!dwarf::accelerometer::accelerometer_get_sample(record)) {
                // This can never happen as we already checked
                // there are enough samples before.
                bsod("Get accelerometer sample failed.");
            }
            xyzSample = AccelerometerUtils::pack_record(record);
        }
        if (encoder.encode(accelerometer_data)) {
            encoded = true;
        } else {
            failed_to_encode();
        }
    }
}

/**
 * @brief FIFO handler for encoded data stream
 *
 * All possible data sources are polled until either
 * no source has more data or fifo is full.
 *
 * @param fifo
 * @return number of registers written
 */
size_t handle_encoded_fifo(std::array<uint16_t, MODBUS_FIFO_LEN> &fifo) {
    Encoder encoder(fifo);

    bool encoded = true;
    while (encoded) {
        encoded = false;

        pickup_accelerometer_sample(encoder, encoded);
        if (encoded) {
            acc_sample_counter++;
            continue; // Continue packing accelerometer samples - the highest priority
        }

        if (acc_sample_counter >= 100) {
            acc_sample_counter = 0;
            if (encoder.can_encode<AccelerometerSamplingRate>()) {
                AccelerometerSamplingRate sample = { .frequency = dwarf::accelerometer::measured_sampling_rate() };
                if (encoder.encode(sample)) {
                    encoded = true;
                } else {
                    failed_to_encode();
                }
            }
        }

        // Pickup loadcell sample
        if (encoder.can_encode<LoadcellRecord>()) {
            LoadcellRecord sample;
            if (dwarf::loadcell::get_loadcell_sample(sample)) {
                if (encoder.encode(sample)) {
                    encoded = true;
                    continue; // Continue packing loadcell samples - the second highest priority
                } else {
                    failed_to_encode();
                }
            }
        }

        // Pickup log data
        if (encoder.can_encode<LogData>()) {
            LogData log_fragment;
            log_fragment.fill(0);
            size_t num_log_bytes = bufflog_pickup(log_fragment.data(), log_fragment.size());
            if (num_log_bytes) {
                if (encoder.encode(log_fragment)) {
                    encoded = true;
                } else {
                    failed_to_encode();
                }
            }
        }
    }

    encoder.padd();

    return encoder.position();
}

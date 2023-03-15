#include "ModbusFIFOHandlers.hpp"

#include "logging/log_dest_bufflog.h"
#include "puppies/fifo_encoder.hpp"
#include "loadcell.hpp"

using namespace common::puppies::fifo;

LOG_COMPONENT_DEF(ModbusFIFOHandlers, LOG_SEVERITY_DEBUG);

size_t handle_encoded_fifo(std::array<uint16_t, MODBUS_FIFO_LEN> &fifo) {
    Encoder encoder(fifo);

    bool encoded = true;
    while (encoded) {
        encoded = false;

        // Pickup loadcell sample
        if (encoder.can_encode<LoadCellData_t>()) {
            LoadcellRecord sample;
            if (dwarf::loadcell::get_loadcell_sample(sample)) {
                if (encoder.encode(sample.timestamp, sample.loadcell_raw_value)) {
                    encoded = true;
                }
            }
        }

        // Pickup log data
        if (encoder.can_encode<LogData_t>()) {
            LogData_t log_fragment;
            log_fragment.fill(0);
            size_t num_log_bytes = bufflog_pickup(log_fragment.data(), log_fragment.size());
            if (num_log_bytes) {
                if (encoder.encode(42, log_fragment)) {
                    encoded = true;
                }
            }
        }
    }

    encoder.padd();

    return encoder.position();
}

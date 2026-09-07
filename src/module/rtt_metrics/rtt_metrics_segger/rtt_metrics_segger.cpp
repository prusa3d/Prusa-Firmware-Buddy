///@file
#include "include/rtt_metrics_segger/rtt_metrics_segger.hpp"
#include <cassert>
#include <cstring>
#include <cobs/cobs.hpp>
#include <utils/byte_utils.hpp>

#include <SEGGER_RTT.h>

using namespace rtt_metrics;

namespace {

constexpr unsigned rtt_buffer_index = 2;

std::array<std::byte, 8192> rtt_buffer_data;

/// Check whether an RTT consumer (debugger) is actively draining the buffer.
///
/// Compares the current RdOff with a snapshot taken after the previous write.
/// - If RdOff advanced → consumer is alive, safe to do a blocking write.
/// - If RdOff is stagnant AND the buffer is full → no consumer, skip the write.
/// - If RdOff is stagnant but buffer has space → proceed (consumer may be slow).
///
/// The static snapshot is safe because log_metric() (its only caller) runs from
/// a single consumer context (the rtt_metrics task).
bool has_active_consumer() {
    static unsigned prev_rd_off = 0;
    static bool consumer_present = true;

    const unsigned rd_off = _SEGGER_RTT.aUp[rtt_buffer_index].RdOff;

    if (rd_off != prev_rd_off) {
        prev_rd_off = rd_off;
        consumer_present = true;
        return true;
    }

    if (SEGGER_RTT_GetAvailWriteSpace(rtt_buffer_index) == 0) {
        consumer_present = false;
        return false;
    }

    return consumer_present;
}

} // anonymous namespace

void rtt_metrics::init_rtt_metrics() {
    SEGGER_RTT_Init();
    SEGGER_RTT_ConfigUpBuffer(rtt_buffer_index, "metrics", &rtt_buffer_data[0], sizeof(rtt_buffer_data), SEGGER_RTT_MODE_NO_BLOCK_SKIP);
}

void rtt_metrics::log_metric(Bytes buffer) {
    const size_t encoded_buffer_size = cobs::max_encoded_frame_size(buffer.size());
    std::byte encoded[encoded_buffer_size];
    WritableBytes encoded_buffer_span(encoded, encoded_buffer_size);

    Bytes input_buffer(buffer.data(), buffer.size());
    auto encoded_ret = cobs::encode(input_buffer, encoded_buffer_span);
    if (has_active_consumer()) {
        debug_assert(encoded_ret.has_value());
        SEGGER_RTT_Write(rtt_buffer_index, encoded, encoded_ret.value());
    }
}

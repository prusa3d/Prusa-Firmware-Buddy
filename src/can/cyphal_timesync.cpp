
#include "cyphal_timesync.hpp"

#include <option/has_cyphal_metrics.h>
#include <bsod/bsod.h>
#if HAS_CYPHAL_METRICS()
    // #error dead code found by automatic analyses (see BFW-5461)
    #include <metric.h>
    #include <cinttypes>

    #define ENABLE_ALL_TIMESYNC_METRICS METRIC_DISABLED

METRIC_DEF(metric_raw_offset_us, "timesync_raw_offset_us", METRIC_VALUE_CUSTOM, 0, ENABLE_ALL_TIMESYNC_METRICS);
METRIC_DEF(metric_raw_drift, "timesync_raw_drift", METRIC_VALUE_CUSTOM, 0, ENABLE_ALL_TIMESYNC_METRICS);
METRIC_DEF(metric_offset_err_us2, "timesync_offset_err_us2", METRIC_VALUE_CUSTOM, 0, ENABLE_ALL_TIMESYNC_METRICS);
METRIC_DEF(metric_offset_us, "timesync_offset_us", METRIC_VALUE_CUSTOM, 0, ENABLE_ALL_TIMESYNC_METRICS);
METRIC_DEF(metric_drift_err, "timesync_drift_err", METRIC_VALUE_CUSTOM, 0, ENABLE_ALL_TIMESYNC_METRICS);
METRIC_DEF(metric_drift, "timesync_drift", METRIC_VALUE_CUSTOM, 0, ENABLE_ALL_TIMESYNC_METRICS);
#endif

namespace can::cyphal {

namespace {
    /// Main clock timer resolution under 1 us. Convert this from enum to something useful.
    constexpr int32_t TIM_BASE_CLK_MHZ_ = TIM_BASE_CLK_MHZ;
} // namespace

TimeSync::TimeSync(uint32_t filter_index_)
    : filter_index(filter_index_)
    , sync_suber(uavcan_time_Synchronization_1_0_deserialize_, uavcan_time_Synchronization_1_0_FIXED_PORT_ID_,
          [this](const uavcan_time_Synchronization_1_0 &data, const ProtoSuber::Meta &meta) {
              if (meta.remote_node_id == CANARD_NODE_ID_UNSET) {
                  return; // Anonymous message would mess the following if
              }

              if (sync_lock.load()) {
                  if (master_node_id == meta.remote_node_id) { // From the locked master
                      if (ProtoSender::increment_transfer_id(last_transfer_id) == meta.transfer_id // Second message in a row
                          && last_sync_valid.load() == false // Previous set was already processed
                          && data.previous_transmission_timestamp_microsecond != 0) { // 0 is used on first message or if master lost the timestamp
                          // Just store the time, no heavy computing in callback
                          last_sync = { last_rx_local, static_cast<int64_t>(data.previous_transmission_timestamp_microsecond) };
                          last_sync_valid = true;
                      }
                      last_transfer_id = meta.transfer_id;
                      last_rx_local = meta.timestamp;
                  }
              } else { // First message from a new master
                  last_transfer_id = meta.transfer_id;
                  last_rx_local = meta.timestamp;
                  master_node_id = meta.remote_node_id;
                  sync_lock = true;
              }
          }) {
    debug_assert(time_mutex != nullptr);
}

bool TimeSync::loop(int64_t timestamp) {
    bool ret = false; ///< Return value

    if (sync_lock.load()) { // We have locked a timesync master
        if (last_sync_valid.load() == true) { // New set of local-remote obtained
            if (Task::RAIILock lock(time_mutex, 0); lock.is_locked()) { // Try to get mutex
                if (previous_sync_valid) {
                    // Compute raw offset info
                    const int64_t new_offset_us = last_sync.remote - last_sync.local;
                    const int64_t previous_offset_us = previous_sync.remote - previous_sync.local;
                    const double puppy_drift = static_cast<double>(new_offset_us - previous_offset_us)
                        / static_cast<double>(last_sync.local - previous_sync.local);

                    // Compute filtered offset and drift
                    offset_filter.filter(new_offset_us, offset_filter.value() + drift_filter.value() * (last_sync.local - previous_sync.local));
                    drift_filter.filter(puppy_drift, drift_filter.value());

                    // Metrics
#if HAS_CYPHAL_METRICS()
                    // #error dead code found by automatic analyses (see BFW-5461)
                    uint32_t ticks_us_now = ticks_us();
                    metric_record_custom_at_time(&metric_raw_offset_us, ticks_us_now, " v=%" PRIi64, new_offset_us);
                    metric_record_custom_at_time(&metric_raw_drift, ticks_us_now, " v=%.6f", puppy_drift);
                    metric_record_custom_at_time(&metric_offset_err_us2, ticks_us_now, " v=%f", offset_filter.error());
                    metric_record_custom_at_time(&metric_offset_us, ticks_us_now, " v=%.10f", offset_filter.value());
                    metric_record_custom_at_time(&metric_drift_err, ticks_us_now, " v=%e", drift_filter.error());
                    metric_record_custom_at_time(&metric_drift, ticks_us_now, " v=%.3e", drift_filter.value());
#endif

                    // Set remote data copy for ISR access
                    bool valid_remote = remote_data_valid.load();
                    RemoteData &rd = remote_data[!valid_remote]; // Get data that are not currently valid
                    rd.offset_us = offset_filter.value();
                    rd.average_drift = drift_filter.value();
                    rd.previous_sync_local = previous_sync.local;
                    remote_data_valid = !valid_remote; // Atomically switch which data are valid

                    // set the status of the sync
                    sync_valid = true;

                    ret = is_precise(); // We have new synchronization data and it is precise
                } else {
                    // First value, reset filters
                    offset_filter.reset(OFFSET_INITIAL_ERROR, last_sync.remote - last_sync.local);
                    drift_filter.reset(DRIFT_INITIAL_ERROR, 0);
                }

                previous_sync = last_sync;
                previous_sync_valid = true;
                last_sync_valid = false;
                last_calculation_done = timestamp;
            }
        } else if (timestamp - last_rx_local > SYNC_LOCK_TIMEOUT_US) { // Synchronization master is not sending messages
            sync_lock = false;
            previous_sync_valid = false;
        }
    }

    if (sync_valid && timestamp - last_calculation_done > SYNC_LOST_TIMEOUT_US) {
        sync_valid = false; // Too long without synchronization
    }

    return ret; // Return true if we have new synchronization and it is precise
}

int64_t TimeSync::get_local(int64_t remote_timestamp, TickType_t timeout) {
    Task::RAIILock lock(time_mutex, timeout); // Wait for mutex
    if (lock.is_locked() == false) {
        return -2; // Timeout
    }

    if (sync_valid == false) {
        return -1; // We don't know
    }

    int64_t offset = static_cast<int64_t>(offset_filter.value() + drift_filter.value() * (remote_timestamp - previous_sync.remote) + 0.5);
    return remote_timestamp - offset;
}

TimeSync::SubtickTimestamp TimeSync::get_local_subtick(int64_t remote_timestamp, TickType_t timeout) {
    Task::RAIILock lock(time_mutex, timeout); // Wait for mutex
    if (lock.is_locked() == false) {
        return { .timestamp = -2, .subticks = 0 }; // Timeout
    }

    if (sync_valid == false) {
        return { .timestamp = -1, .subticks = 0 }; // We don't know
    }

    int64_t offset_subticks
        = static_cast<int64_t>(
              ((offset_filter.value() + drift_filter.value() * (remote_timestamp - previous_sync.remote)) // Raw offset
                  * TIM_BASE_CLK_MHZ_) // Multiply by TIM_BASE_CLK_MHZ_
              + 0.5) // Round
        + (TIM_BASE_CLK_MHZ_ - 1); // Add TIM_BASE_CLK_MHZ_ - 1 to have positive subticks
    return { .timestamp = remote_timestamp - offset_subticks / TIM_BASE_CLK_MHZ_, // Timestamp [us]
        .subticks = TIM_BASE_CLK_MHZ_ - 1 - static_cast<uint32_t>(offset_subticks % TIM_BASE_CLK_MHZ_) }; // Subticks [1/TIM_BASE_CLK_MHZ us]
}

int64_t TimeSync::get_remote(int64_t local_timestamp, TickType_t timeout) {
    Task::RAIILock lock(time_mutex, timeout); // Wait for mutex
    if (lock.is_locked() == false) {
        return -2; // Timeout
    }

    if (sync_valid == false) {
        return -1; // We don't know
    }

    int64_t offset = static_cast<int64_t>(offset_filter.value() + drift_filter.value() * (local_timestamp - previous_sync.local) + 0.5);
    return local_timestamp + offset;
}

int64_t TimeSync::get_remote_isr(int64_t local_timestamp) {
    if (sync_valid.load() == false) {
        return -1; // We don't know
    }

    const RemoteData &rd = remote_data_valid.load() ? remote_data[1] : remote_data[0]; // Which remote data are valid

    int64_t offset = static_cast<int64_t>(rd.offset_us + rd.average_drift * (local_timestamp - rd.previous_sync_local) + 0.5);
    return local_timestamp + offset;
}

} // namespace can::cyphal

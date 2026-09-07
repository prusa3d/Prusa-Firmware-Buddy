
#include "cyphal_record.hpp"
#include "logging/log_dest_shared.hpp"
#include <bsod/bsod.h>

namespace can::cyphal {

Record *Record::instance = nullptr; ///< Singleton instance

Record::Record()
    : log_sender(CanardTransferKindMessage,
        CANARD_NODE_ID_UNSET, ProtoSender::send_timeout_default,
        CanardPrioritySlow) {
    debug_assert(instance == nullptr); // Allow only one instance
    instance = this;

    debug_assert(access_mutex != nullptr); // Check if mutex was created
}

Record::~Record() {
    instance = nullptr;
}

void Record::add_character(char character, [[maybe_unused]] void *arg) {
    auto record = std::launder(reinterpret_cast<uavcan_diagnostic_Record_1_1 *>(arg));

    if (record->text.count < uavcan_diagnostic_Record_1_1_text_ARRAY_CAPACITY_) {
        record->text.elements[record->text.count++] = character;
    }
}

void Record::extern_log_event(logging::FormattedEvent *event) {
    if (instance == nullptr) {
        return; // Not inited yet, ignore
    }
    instance->member_log_event(event);
}

void Record::member_log_event(logging::FormattedEvent *event) {
    // Find any free buffer
    for (auto &buffer : buffers) {
        if (buffer.blocked.exchange(true) == false) { // Found and blocked a free buffer
            const bool in_isr = xPortIsInsideInterrupt() == pdTRUE; ///< Extra care if logging from ISR

            // Convert timestamp to network time
            CanardMicrosecond timestamp = static_cast<int64_t>(event->timestamp.sec) * 1000000 + event->timestamp.us;
            if (time_sync != nullptr) {
                if (in_isr) {
                    timestamp = std::max<int64_t>(time_sync->get_remote_isr(timestamp), 0);
                } else {
                    timestamp = std::max<int64_t>(time_sync->get_remote(timestamp), 0);
                }
            }
            buffer.record.timestamp.microsecond = timestamp;

            // Convert severity to Cyphal value
            switch (event->severity) {
            case logging::Severity::debug:
                buffer.record.severity.value = uavcan_diagnostic_Severity_1_0_DEBUG;
                break;
            case logging::Severity::info:
                buffer.record.severity.value = uavcan_diagnostic_Severity_1_0_INFO;
                break;
            case logging::Severity::warning:
                buffer.record.severity.value = uavcan_diagnostic_Severity_1_0_WARNING;
                break;
            case logging::Severity::error:
                buffer.record.severity.value = uavcan_diagnostic_Severity_1_0_ERROR;
                break;
            case logging::Severity::critical:
                buffer.record.severity.value = uavcan_diagnostic_Severity_1_0_CRITICAL;
                break;
            default:
                buffer.record.severity.value = uavcan_diagnostic_Severity_1_0_TRACE;
                break;
            }

            // Store message into buffer
            buffer.record.text.count = 0;
            log_format_honeybee_node(event, add_character, &buffer.record);

            // Mark as finished
            buffer.filled = true;

            // Try to send the message only if not in interrupt and not in callback
            if (in_isr == false
                && cyphal_task.is_in_callback() == false) {
                try_send();
            }

            return;
        }
    }

    // No free buffer, increment lost counter
    /// @note Not ISR safe, don't care.
    lost_counter++;
}

void Record::try_send() {
    Task::RAIILock lock(access_mutex, 0); // Try
    if (lock.is_locked() == false) {
        return; // Probably used from member_log_event(), try again later
    }

    // Find the oldest filled buffer
    BufferLine *to_send = nullptr;
    for (auto &buffer : buffers) {
        if (buffer.filled.load()) {
            if (to_send == nullptr // First filled buffer
                || buffer.record.timestamp.microsecond < to_send->record.timestamp.microsecond) { // Earlier timestamp
                to_send = &buffer;
            }
        }
    }

    // Try to send the message
    if (to_send != nullptr) {
        if (log_sender.send_data(to_send->record, std::nullopt, SEND_TIMEOUT)) {
            to_send->filled = false;
            to_send->blocked = false;
        }
    }
}

} // namespace can::cyphal

/// Wrapper to be used from C code
void cyphal_log_event(logging::FormattedEvent *event) {
    can::cyphal::Record::extern_log_event(event);
}


#include "cyphal_can_stats.hpp"
#include "canard.h"
#include "cyphal_proto_sender.hpp"
#include "cyphal_proto_suber.hpp"
#include "timing.h"
#include <prusa3d/common/PortIds_0_1.h>
#include <logging/log.hpp>
#include <bsod/bsod.h>

LOG_COMPONENT_REF(can);

namespace can::cyphal {

CanStats::CanStats()
    : stat_suber(
        [this](const RawPacketTraits::Type &data, const ProtoSuber::Meta &meta) {
            if (data.serialized_size == command_len) {
                on_command(data, meta.remote_node_id, meta.transfer_id);
            } else {
                on_data(data, meta.remote_node_id, meta.transfer_id);
            }
        },
        CanardTransferKindMessage, ProtoSuber::multipart_timeout_short)
    , stat_sender(
          CanardTransferKindMessage, CANARD_NODE_ID_UNSET,
          ProtoSender::send_timeout_default, CanardPriorityOptional) {
    debug_assert(tx_semaphore != nullptr);
}

void CanStats::on_command(const RawPacketTraits::Type &message, CanardNodeID remote_node_id, CanardTransferID transfer_id) {
    if (memcmp(message.serialized_data.data(), command_start.serialized_data.data(), command_len) == 0) {
        // Start command
        if (rx_node_id != remote_node_id || rx_counter != (counter_start - 1UL)) {
            log_info(can, "Stats start: node=%hu", remote_node_id);
        }
        rx_node_id = remote_node_id;
        rx_counter = counter_start - 1UL;
        rx_duplicates = 0;
        rx_missing = 0;
        rx_err = 0;
        rx_transfer_id_mismatch = 0;
        rx_transfer_id = transfer_id;

        // Sample CAN error log counter
        can_error_log_start = cyphal_task.get_error_log();
    } else if (memcmp(message.serialized_data.data(), command_end.serialized_data.data(), command_len) == 0) {
        // End command
        if (rx_node_id != remote_node_id) {
            return; // Ignore if not the same node
        }
        rx_node_id = CANARD_NODE_ID_UNSET; // Reset node ID

        // Sample CAN error log counter
        can_error_log_end = cyphal_task.get_error_log();

        // Print stats, one print now and the rest from task
        print_stats.store(print_stats_n_times);
        poll_finished_stats(get_timestamp_us());
    } else {
        log_error(can, "Stats unknown command: 0x%02x", message.serialized_data[0]);
    }
}

void CanStats::on_data(const RawPacketTraits::Type &message, CanardNodeID remote_node_id, CanardTransferID transfer_id) {
    if (rx_node_id != remote_node_id) {
        return; // Ignore if not the same node
    }

    // Check length
    if (message.serialized_size != RawPacketTraits::data_size) {
        rx_err++;
        return; // Invalid message
    }

    // Check message and get its counter
    uint32_t counter;
    if (auto valid = validate_message(message); !valid.has_value()) {
        rx_err++;
        return; // Invalid message
    } else {
        counter = valid.value();
    }

    // Check how many messages we missed
    int32_t diff = counter - rx_counter;
    if (diff < 0) {
        rx_err++;
        return; // Invalid message
    } else if (diff == 0) {
        rx_duplicates++;
    } else {
        rx_missing += diff - 1UL;
    }
    rx_counter = counter;

    // Check transfer ID (not on first message)
    if (diff < static_cast<int32_t>(CANARD_TRANSFER_ID_MAX)
        && counter != counter_start
        && (rx_transfer_id + diff) % (CANARD_TRANSFER_ID_MAX + 1) != transfer_id) {
        rx_transfer_id_mismatch++;
    }
    rx_transfer_id = transfer_id;
}

void CanStats::loop() {
    xSemaphoreTake(tx_semaphore, portMAX_DELAY); // Wait for task to start
    uint32_t data_messages_copy = data_messages.load(); // Get number of messages to send

    /// Send start command
    for (size_t i = 0; i < command_repeats; i++) {
        stat_sender.send_data(command_start);
    }

    /// Send data
    for (uint32_t counter = counter_start; counter < data_messages_copy; counter++) {
        stat_sender.send_data(create_message(counter));
    }

    /// Send end command
    for (size_t i = 0; i < command_repeats; i++) {
        stat_sender.send_data(command_end);
    }
}

uint32_t CanStats::prng(uint32_t previous) {
    uint64_t x = static_cast<uint64_t>(previous) * A; // x_new = x_old*A
    x = (x >> P_MERS) + (x & P); // x / w + x % w (almost mod P, P = w-1)
    if (x >= P) {
        return x - P; // The rest of mod P
    } else {
        return x;
    }
}

RawPacketTraits::Type CanStats::create_message(uint32_t counter) {
    RawPacketTraits::Type message;
    message.serialized_size = RawPacketTraits::data_size;

    // Counter is first 4 bytes
    memcpy(message.serialized_data.data(), &counter, sizeof(counter));

    // Fill the rest with pseudorandom numbers
    uint32_t random = counter; // Seed for the PRNG
    for (size_t i = sizeof(random); i < RawPacketTraits::data_size; i++) {
        random = prng(random);
        message.serialized_data[i] = (random & 0xff); // Use only the lowest byte
    }

    return message;
}

std::optional<uint32_t> CanStats::validate_message(const RawPacketTraits::Type &message) {
    if (message.serialized_size != RawPacketTraits::data_size) {
        return 0; // Invalid message
    }

    // Counter is first 4 bytes
    uint32_t counter = 0;
    memcpy(&counter, message.serialized_data.data(), sizeof(counter));

    // Check the rest with pseudorandom numbers
    uint32_t random = counter; // Seed for the PRNG
    for (size_t i = sizeof(random); i < RawPacketTraits::data_size; i++) {
        random = prng(random);
        if (message.serialized_data[i] != (random & 0xff)) {
            return std::nullopt; // Invalid message
        }
    }

    return counter;
}

bool CanStats::start_tx_session(size_t parameter_len, const uint8_t *parameter) {
    uint32_t data_messages_ = 0;
    for (uint32_t i = parameter_len, dek = 1; i > 0; i--, dek *= 10) {
        data_messages_ += dek * (parameter[i - 1] - '0');
    }
    if (data_messages_ <= 0 || data_messages_ >= P) {
        log_error(can, "Stats invalid number of messages. Needs to be between 1 and %lu", P);
        return false; // Invalid number of messages
    }
    log_info(can, "Stats start: cnt=%lu", data_messages_);
    data_messages.store(data_messages_); // Set number of messages to send
    xSemaphoreGive(tx_semaphore); // Notify waiting thread
    return true;
}

void CanStats::poll_finished_stats(int64_t now) {
    if (print_stats.load() > 0 && now - last_print_time > print_stats_interval) {
        log_info(can, "Stats: cnt=%lu, dups=%lu, miss=%lu, err=%lu, tid_miss=%lu, err_log=%lu",
            rx_counter, rx_duplicates, rx_missing, rx_err, rx_transfer_id_mismatch, can_error_log_end - can_error_log_start);
        last_print_time = now;
        print_stats--;
    }
}
} // namespace can::cyphal

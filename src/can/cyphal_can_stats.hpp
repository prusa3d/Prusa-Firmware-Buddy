
#pragma once

#include "canard.h"
#include <cstdint>
#include <cyphal_task.hpp>
#include <cyphal_suber_call.hpp>
#include <cyphal_sender_direct.hpp>
#include <cyphal_register.hpp>
#include <cyphal_port_list.hpp>
#include <cyphal_traits_utils.hpp>

#include <prusa3d/common/PortIds_0_1.h>

#include <option/cyphal_can_stats.h>
#include <bsod/bsod.h>
#if !CYPHAL_CAN_STATS()
    #error
#endif

namespace can::cyphal {

class CanStats {
    static constexpr size_t command_repeats = 3; ///< Number of times to repeat commands
    static constexpr size_t command_len = 1; ///< Length of the command
    static constexpr RawPacketTraits::Type command_start = { .serialized_data = { 0x00 }, .serialized_size = command_len }; ///< Start command
    static constexpr RawPacketTraits::Type command_end = { .serialized_data = { 0xff }, .serialized_size = command_len }; ///< End command

    static constexpr uint32_t counter_start = 1; ///< First counter to send (needs to be > 0 for the prng to work)

    static constexpr size_t print_stats_n_times = 3; ///< Print stats multiple times to avoid loss if diagnostic frame gets lost
    static constexpr int64_t print_stats_interval = 1'000'000; ///< Print stats this often [us]

    SuberCallTraited<RawPacketTraits, prusa3d_common_PortIds_0_1_MSG_COMMON_STATS> stat_suber; ///< Subscription for stats messages
    SenderDirectTraited<RawPacketTraits, prusa3d_common_PortIds_0_1_MSG_COMMON_STATS> stat_sender; ///< Sender for stats messages

    StaticSemaphore_t tx_semaphore_buffer; ///< Buffer for tx semaphore
    SemaphoreHandle_t tx_semaphore = xSemaphoreCreateBinaryStatic(&tx_semaphore_buffer); ///< Semaphore to pause the tx thread
    std::atomic<uint32_t> data_messages = 0; ///< Number of data messages to send

    CanardNodeID rx_node_id = CANARD_NODE_ID_UNSET; ///< Node-ID of the sender
    uint32_t rx_counter = 0; ///< Follows received messages
    uint32_t rx_duplicates = 0; ///< Count duplicate messages
    uint32_t rx_missing = 0; ///< Count missing messages
    uint32_t rx_err = 0; ///< Count errors in data (if message counts here, it might also count as missing)
    uint32_t rx_transfer_id_mismatch = 0; ///< Count transfer-ID mismatches
    CanardTransferID rx_transfer_id = 0; ///< Transfer ID of the last received message

    uint32_t can_error_log_start = 0; ///< CAN error log counter when the test started
    uint32_t can_error_log_end = 0; ///< CAN error log counter when the test ended
    std::atomic<uint32_t> print_stats = 0; ///< Print stats this many times
    int64_t last_print_time = 0; ///< Last time when stats were printed

    /**
     * @brief Task to send measurement messages.
     * This will be sleeping until start_tx_session().
     * Then it will be pushing messages as quickly as possible until all are sent.
     */
    void loop();

    /**
     * @brief Callback when message with command is received.
     * @param message message with a command
     * @param remote_node_id node-ID of the sender
     * @param transfer_id transfer-ID of the message
     */
    void on_command(const RawPacketTraits::Type &message, CanardNodeID remote_node_id, CanardTransferID transfer_id);

    /**
     * @brief Callback when message with data is received.
     * @param message message with data
     * @param remote_node_id node-ID of the sender
     * @param transfer_id transfer-ID of the message
     */
    void on_data(const RawPacketTraits::Type &message, CanardNodeID remote_node_id, CanardTransferID transfer_id);

    /**
     * @brief Tiny PRNG to fill the packet with reproducible data.
     * @param previous can be between 1 and P-1
     * @return next value
     */
    uint32_t prng(uint32_t previous);
    static constexpr const uint32_t P_MERS = (31); ///< Nth Mersenne prime
    static constexpr const uint32_t P = ((1ULL << P_MERS) - 1); ///< 2^N - 1 is often a prime
    static constexpr const uint64_t A = 62089911ULL; ///< Suggested by Knuth, found by Fishman and Moore

    /**
     * @brief Create a data message to send.
     * @param counter counter to put in the message
     * @return message with the given counter and pseudorandom fill
     */
    RawPacketTraits::Type create_message(uint32_t counter);

    /**
     * @brief Validate received data message.
     * @param message message to validate
     * @return counter if valid
     */
    std::optional<uint32_t> validate_message(const RawPacketTraits::Type &message);

public:
    /**
     * @brief Object to get stats of the CAN bus.
     */
    CanStats();

    /**
     * @brief Add itself to task and to portlist.
     * @note Does not add port name and type registers.
     * @param port_list node's object publishing used ports
     */
    void init(PortList &port_list) {
        stat_suber.add_to_task();
        port_list.add(stat_suber);
        port_list.add(stat_sender);
    }

    /**
     * @brief Add itself to task, to portlist and to registers.
     * @param port_list node's object publishing used ports
     * @param registers node's object handling registers
     */
    void init(PortList &port_list, RegisterMachineIface &registers) {
        init(port_list);
        registers.add_port_name_set("sub.can_stats", uavcan_primitive_Unstructured_1_0_FULL_NAME_AND_VERSION_, stat_suber.get_port_id());
        registers.add_port_name_set("pub.can_stats", uavcan_primitive_Unstructured_1_0_FULL_NAME_AND_VERSION_, stat_sender.get_port_id());
    }

    /**
     * @brief Task to send measurement messages.
     * This will be sleeping until start_tx_session().
     * Then it will be pushing messages as quickly as possible until all are sent.
     * @param self pointer to this object
     */
    [[noreturn]] static void task(void *self) {
        debug_assert(self != nullptr);
        while (1) {
            reinterpret_cast<CanStats *>(self)->loop();
        }
    }

    /**
     * @brief Start sending measurement messages.
     * @param parameter_len length of the string in parameter
     * @param parameter number of messages to send encoded in string
     *    This string is not null-terminated, but uses len.
     * @note The parameter numerical value should be high enough to detect problems.
     *    If the packet error rate would be around 10^-6, there should be at least 1e7 messages.
     * @return false if invalid number of messages
     */
    bool start_tx_session(size_t parameter_len, const uint8_t *parameter);

    /**
     * @brief Log the statistics after receive was finished.
     * @param now current time [us]
     */
    void poll_finished_stats(int64_t now);
};

} // namespace can::cyphal

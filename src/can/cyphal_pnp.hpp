
#pragma once

#include <timing.h>

#include "cyphal_task.hpp"
#include "cyphal_sender_direct.hpp"
#include "cyphal_suber_call.hpp"

#include <uavcan/pnp/NodeIDAllocationData_2_0.h>
#include <bsod/bsod.h>

namespace can::cyphal {

/**
 * @brief Plug `n` Play object that requests node ID from allocator.
 */
class PnP {
    CanardMicrosecond next_send = 0U; ///< Next time to send request

    CanardNodeID id_giver = CANARD_NODE_ID_UNSET; ///< Node ID of a node that given us our node ID

    /// @brief Request ID from allocator by anonymous message.
    SenderDirectTraited<uavcan_pnp_NodeIDAllocationData_2_0_Traits> id_request;
    uavcan_pnp_NodeIDAllocationData_2_0 request_data; ///< Request data

    /// @brief Receive ID from allocator.
    SuberCallTraited<uavcan_pnp_NodeIDAllocationData_2_0_Traits> id_response;

public:
    /**
     * @brief Plug `n` Play object.
     * @param request_id suggest this ID, we can get any other ID if this is not available
     * @param uid unique ID of this node
     */
    PnP(CanardNodeID request_id, const uint8_t uid[sizeof(uavcan_pnp_NodeIDAllocationData_2_0::unique_id)])
        : id_request(CanardTransferKindMessage, CANARD_NODE_ID_UNSET, ProtoSender::send_timeout_default, CanardPrioritySlow)
        , id_response(
              [this](const uavcan_pnp_NodeIDAllocationData_2_0 &data, [[maybe_unused]] const ProtoSuber::Meta &meta) {
                  if (memcmp(data.unique_id, request_data.unique_id, sizeof(data.unique_id))) { // This is not our ID
                      return;
                  }
                  id_giver = meta.remote_node_id; // Save giver ID
                  cyphal_task.set_node_id(data.node_id.value); // Set our ID
                  id_response.remove_from_task(); // Remove response subscription
              }) {
        request_data.node_id.value = request_id;
        debug_assert(uid != nullptr);
        memcpy(request_data.unique_id, uid, sizeof(request_data.unique_id));

        // If this ever starts failing, make sure to either use PRNG with larger state
        // or pad UID with some well-defined bytes. Zeroes are OK.
        // Technically, we should guard against using all-zero-state in xorshift based PRNGs.
        // In practice, we are using STM32H5 MCU UID which contains ASCII-encoded data about
        // the wafer which is never zero. But even if it were, there would only be one such
        // chip in the world and PRNG is only used to provide source of jitter for allocation
        // requests anyway.
        static_assert(sizeof(prng.state) == sizeof(request_data.unique_id));
        memcpy(prng.state, request_data.unique_id, sizeof(request_data.unique_id));
    }

    /**
     * @brief Manage sending requests for node ID.
     * Call this reasonably often to reduce risk of collision, ~100 Hz.
     */
    void loop_tx() {
        if (cyphal_task.is_anonymous()) { // Our ID is not set yet
            if (CanardMicrosecond timestamp = get_timestamp_us(); timestamp > next_send) { // Time to send request
                id_request.send_data(request_data); // Send request
                next_send = timestamp + 200 * 1000 + (prng.next() % (800 * 1000)); // Send every 200-1000 ms
            }
        }
    }

    /// @brief Start PnP process.
    void start() {
        id_giver = CANARD_NODE_ID_UNSET;
        cyphal_task.set_node_id(CANARD_NODE_ID_UNSET);
        cyphal_task.whitelist_pnp(&id_request, &id_response);
        id_response.add_to_task();
    }

    /**
     * @brief This node has given us the node ID.
     * @return node ID of the node that given us our node ID
     */
    CanardNodeID get_id_giver() const { return id_giver; }

private:
    /// xoshiro128** 1.1 by David Blackman and Sebastiano Vigna
    struct PRNG {
        /// The state must be seeded so that it is not everywhere zero.
        uint32_t state[4];

        /// Get next pseudo random number.
        uint32_t next() {
            const uint32_t result = rotl(state[1] * 5, 7) * 9;
            const uint32_t t = state[1] << 9;

            state[2] ^= state[0];
            state[3] ^= state[1];
            state[1] ^= state[2];
            state[0] ^= state[3];

            state[2] ^= t;

            state[3] = rotl(state[3], 11);

            return result;
        }

        static constexpr uint32_t rotl(const uint32_t x, int k) {
            return (x << k) | (x >> (32 - k));
        }
    };
    PRNG prng;
};

} // namespace can::cyphal

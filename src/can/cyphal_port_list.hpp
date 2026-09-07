#pragma once

#include "cyphal_sender_data.hpp"
#include "cyphal_proto_sender.hpp"
#include "cyphal_proto_suber.hpp"
#include "cyphal_client.hpp"
#include "cyphal_server.hpp"

#include <uavcan/node/port/List_1_0.h>
#include <uavcan/time/Synchronization_1_0.h>
#include <bsod/bsod.h>

namespace can::cyphal {

/**
 * @brief This provides port list with less than full memory footprint.
 * Full uavcan_node_port_List_1_0 is 2184 bytes and it needs buffers for 8466 bytes, though that may be a mistake.
 * SERIALIZATION_BUFFER_SIZE is calculated in List_SERIALIZATION_BUFFER_SIZE_BYTES_.
 */
class PortList {
    static constexpr size_t MAX_PUBLISH = 64; ///< Maximal number of publishers
    static constexpr size_t MAX_SUBSCRIBE = 64; ///< Maximal number of subscribers

    static_assert(MAX_PUBLISH <= uavcan_node_port_SubjectIDList_1_0_sparse_list_ARRAY_CAPACITY_, "At most this many publishers");
    static_assert(MAX_SUBSCRIBE <= uavcan_node_port_SubjectIDList_1_0_sparse_list_ARRAY_CAPACITY_, "At most this many subscribers");

    /// Size of the serialized data
    static constexpr size_t List_SERIALIZATION_BUFFER_SIZE_BYTES_ = 0
        + 4 // Delimiter for publishers
        + 1 // Tag to select union type
        + 1 // Number of items in sparse list
        + sizeof(uint16_t) * MAX_PUBLISH // Maximal length of the sparse list
        + 4 // Delimiter for subscribers
        + 1 // Tag to select union type
        + 1 // Number of items in sparse list
        + sizeof(uint16_t) * MAX_SUBSCRIBE // Maximal length of the sparse list
        + 4
        + uavcan_node_port_ServiceIDList_1_0_mask_ARRAY_CAPACITY_ / 8 // Mask for clients
        + 4
        + uavcan_node_port_ServiceIDList_1_0_mask_ARRAY_CAPACITY_ / 8; // Mask for servers

    struct List {
        ProtoPortList *publishers = nullptr; ///< List of ports that are published by this node
        ProtoPortList *subscribers = nullptr; ///< List of ports that are subscribed to by this node
        ProtoPortList *clients = nullptr; ///< List of clients that are used by this node
        ProtoPortList *servers = nullptr; ///< List of servers that are provided by this node
    };

    /// Custom serialize function
    static int8_t serialize(const List *const obj, uint8_t *const buffer, size_t *const inout_buffer_size_bytes) {
        debug_assert(obj != nullptr);
        debug_assert(buffer != nullptr);
        debug_assert(inout_buffer_size_bytes != nullptr);

        const size_t buffer_size = *inout_buffer_size_bytes; ///< Space we have available
        debug_assert(buffer_size >= List_SERIALIZATION_BUFFER_SIZE_BYTES_);

        size_t &used_size = *inout_buffer_size_bytes; ///< Space we have used
        used_size = 0;

        /// Add one byte to buffer
        auto add_byte = [buffer, buffer_size](uint8_t value, size_t &position) {
            debug_assert(position < buffer_size);
            buffer[position] = value;
            position++;
        };

        /// Add two bytes to buffer
        auto add_short = [add_byte](uint16_t value, size_t &position) {
            add_byte(value, position);
            add_byte(value >> 8, position);
        };

        /// Add four bytes to buffer
        auto add_word = [add_byte](uint32_t value, size_t &position) {
            add_byte(value, position);
            add_byte(value >> 8, position);
            add_byte(value >> 16, position);
            add_byte(value >> 24, position);
        };

        // Publishers
        {
            size_t header = used_size;
            used_size += 6; // Skip delimiter, tag and count

            size_t count = 0;
            for (ProtoPortList *item = obj->publishers; item != nullptr; item = item->portlist_next) {
                count++;
                debug_assert(count <= MAX_PUBLISH);
                add_short(item->port_id, used_size);
            }

            add_word(2 + sizeof(uint16_t) * count, header); // Delimiter
            add_byte(1, header); // Tag to select sparse list
            add_byte(count, header);
        }

        // Subscribers
        {
            size_t header = used_size;
            used_size += 6; // Skip delimiter, tag and count

            size_t count = 0;
            for (ProtoPortList *item = obj->subscribers; item != nullptr; item = item->portlist_next) {
                count++;
                debug_assert(count <= MAX_PUBLISH);
                add_short(item->port_id, used_size);
            }

            add_word(2 + sizeof(uint16_t) * count, header); // Delimiter
            add_byte(1, header); // Tag to select sparse list
            add_byte(count, header);
        }

        // Clients
        {
            add_word(uavcan_node_port_ServiceIDList_1_0_mask_ARRAY_CAPACITY_ / 8, used_size); // Delimiter
            uint8_t *client_mask = buffer + used_size;
            memset(client_mask, 0, uavcan_node_port_ServiceIDList_1_0_mask_ARRAY_CAPACITY_ / 8);
            for (ProtoPortList *item = obj->clients; item != nullptr; item = item->portlist_next) {
                debug_assert(item->port_id < uavcan_node_port_ServiceIDList_1_0_mask_ARRAY_CAPACITY_);
                client_mask[item->port_id / 8] |= 1 << (item->port_id % 8);
            }
            used_size += uavcan_node_port_ServiceIDList_1_0_mask_ARRAY_CAPACITY_ / 8;
        }

        // Servers
        {
            add_word(uavcan_node_port_ServiceIDList_1_0_mask_ARRAY_CAPACITY_ / 8, used_size); // Delimiter
            uint8_t *server_mask = buffer + used_size;
            memset(server_mask, 0, uavcan_node_port_ServiceIDList_1_0_mask_ARRAY_CAPACITY_ / 8);
            for (ProtoPortList *item = obj->servers; item != nullptr; item = item->portlist_next) {
                debug_assert(item->port_id < uavcan_node_port_ServiceIDList_1_0_mask_ARRAY_CAPACITY_);
                server_mask[item->port_id / 8] |= 1 << (item->port_id % 8);
            }
            used_size += uavcan_node_port_ServiceIDList_1_0_mask_ARRAY_CAPACITY_ / 8;
        }

        return 0;
    }

    /// Send list of ports used by this node
    SenderData<List, List_SERIALIZATION_BUFFER_SIZE_BYTES_> list_sender;

    /**
     * @brief Add one port to sublist.
     * @param list one of the ProtoPortList pointers
     * @param added ProtoPortList to be added
     */
    static void add_item(ProtoPortList *&list, ProtoPortList &added) {
        if (list == nullptr) {
            list = &added; // Add first item
            return;
        }

        // Find the end of the list
        ProtoPortList *end = list;
        while (end->portlist_next != nullptr) {
            end = end->portlist_next;
            if (end->portlist_next == &added) {
                return; // Already in the list
            }
        }

        // Add the item
        end->portlist_next = &added;
    }

    /**
     * @brief Remove one port from sublist.
     * @param list one of the ProtoPortList pointers
     * @param removed ProtoPortList to be removed
     */
    static void remove_item(ProtoPortList *&list, ProtoPortList &removed) {
        if (list == nullptr) {
            return; // Nothing in the list
        }

        if (list == &removed) {
            list = removed.portlist_next; // Remove first item
            return;
        }

        // Find the item
        ProtoPortList *previous = list;
        while (previous->portlist_next != &removed) {
            previous = previous->portlist_next;
            if (previous->portlist_next == nullptr) {
                return; // Not found
            }
        }

        // Remove the item
        previous->portlist_next = removed.portlist_next;
        return;
    }

    /// Port for time synchronization is extra, because it doesn't have full ProtoSender
    ProtoPortList timesync_port;

public:
    /**
     * @brief Periodically publish list of used ports.
     */
    PortList()
        : list_sender({}, serialize, uavcan_node_port_List_1_0_FIXED_PORT_ID_,
            uavcan_node_port_List_1_0_MAX_PUBLICATION_PERIOD * 1000000,
            ProtoSender::send_timeout_default, CanardPriorityOptional)
        , timesync_port(uavcan_time_Synchronization_1_0_FIXED_PORT_ID_) {
    }

    /// @brief Add publisher to the list.
    void add(ProtoSender &sender) {
        list_sender.transform_data(
            [&sender](List &list) -> TransformResult {
                add_item(list.publishers, sender);
                return { .success = true, .significant = false };
            });
    }

    /// @brief Remove publisher from the list.
    void remove(ProtoSender &sender) {
        list_sender.transform_data(
            [&sender](List &list) -> TransformResult {
                remove_item(list.publishers, sender);
                return { .success = true, .significant = false };
            });
    }

    /// @brief Add subscriber to the list.
    void add(ProtoSuber &suber) {
        list_sender.transform_data(
            [&suber](List &list) -> TransformResult {
                add_item(list.subscribers, suber);
                return { .success = true, .significant = false };
            });
    }

    /// @brief Remove subscriber from the list.
    void remove(ProtoSuber &suber) {
        list_sender.transform_data(
            [&suber](List &list) -> TransformResult {
                remove_item(list.subscribers, suber);
                return { .success = true, .significant = false };
            });
    }

    /// @brief Add client to the list.
    void add(ClientVoid &client) {
        ProtoPortList &portitem = client.get_protoportlist();
        list_sender.transform_data(
            [&portitem](List &list) -> TransformResult {
                add_item(list.clients, portitem);
                return { .success = true, .significant = false };
            });
    }

    /// @brief Remove client from the list.
    void remove(ClientVoid &client) {
        ProtoPortList &portitem = client.get_protoportlist();
        list_sender.transform_data(
            [&portitem](List &list) -> TransformResult {
                remove_item(list.clients, portitem);
                return { .success = true, .significant = false };
            });
    }

    /// @brief Add server to the list.
    void add(ServerVoid &server) {
        ProtoPortList &portitem = server.get_protoportlist();
        list_sender.transform_data(
            [&portitem](List &list) -> TransformResult {
                add_item(list.servers, portitem);
                return { .success = true, .significant = false };
            });
    }

    /// @brief Remove server from the list.
    void remove(ServerVoid &server) {
        ProtoPortList &portitem = server.get_protoportlist();
        list_sender.transform_data(
            [&portitem](List &list) -> TransformResult {
                remove_item(list.servers, portitem);
                return { .success = true, .significant = false };
            });
    }

    /// @brief Add timesync sender port which is done without ProtoSender.
    void add_timesync_sender() {
        list_sender.transform_data(
            [this](List &list) -> TransformResult {
                add_item(list.publishers, timesync_port);
                return { .success = true, .significant = false };
            });
    }

    /**
     * @brief Add itself to task and to portlist.
     * @note Does not add port name and type registers (not needed for uavcan types).
     */
    void init() {
        add(list_sender);
        list_sender.add_to_task();
    }

    /// @return Published port id.
    [[nodiscard]] CanardPortID get_publish_id() const {
        return list_sender.get_port_id();
    }
};
} // namespace can::cyphal

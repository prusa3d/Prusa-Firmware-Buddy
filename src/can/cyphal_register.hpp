#pragma once

#include "cyphal_server.hpp"
#include "cyphal_timesync.hpp"

#include <uavcan/_register/Access_1_0.h>
#include <uavcan/_register/List_1_0.h>

#include <string>
#include <bsod/bsod.h>

namespace can::cyphal {

/**
 * @brief This provides a Cyphal register interface.
 */
class RegisterMachineIface {
public:
    /**
     * @brief Set and get the value of the register.
     * @param write value to write, check uavcan_register_Value_1_0_is_empty_() before writing
     * @param[inout] read value to read, buffer provided by the caller
     */
    using SetGetCallback = std::function<void(const uavcan_register_Value_1_0 &write, uavcan_register_Value_1_0 &read)>;

    /// Registers stored in an array
    struct ProtoRegister {
        const char *name; ///< Name of the register or name of port_name and its type
        CanardPortID port_name_id; ///< If this is not INT16_MAX, then this is a set of two PORT_NAME registers
        bool is_persistent; ///< Is the register persistent
        bool is_mutable; ///< Is the register mutable
        SetGetCallback set_get; ///< Set and get the value of the register
    };

private:
    ServerTraited<uavcan_register_Access_1_0_Traits> server_access; /// Respond to register access requests
    ServerTraited<uavcan_register_List_1_0_Traits> server_list; ///< Respond to register list requests

    TimeSync *time_sync = nullptr; ///< Time synchronization object
    ProtoRegister *registers; ///< Storage for registers
    size_t max_registers; ///< Maximum number of registers
    size_t register_count = 0; ///< Number of registers added

    /**
     * @brief Response to register access requests.
     * @param data register write command
     */
    void access(const uavcan_register_Access_Request_1_0 &data) {
        uavcan_register_Access_Response_1_0 response; ///< Large response object on stack
        size_t i; ///< Index of found register
        for (i = 0; i < register_count; i++) { // Find the register
            if (registers[i].port_name_id == INT16_MAX) { // Set+Get register
                if (strncmp(registers[i].name, reinterpret_cast<const char *>(data.name.name.elements), data.name.name.count) == 0) {
                    // Found the Set+Get register
                    response._mutable = registers[i].is_mutable;
                    response.persistent = registers[i].is_persistent;
                    registers[i].set_get(data.value, response.value); // Write and Read the register
                    break;
                }
            } else { // Set of two PORT_NAME registers
                const char *parse_position = reinterpret_cast<const char *>(data.name.name.elements);
                size_t parse_remaining = data.name.name.count;

                {
                    // Check PORT_NAME register prefix "uavcan."
                    static constexpr const char *uavcan_prefix = "uavcan.";
                    static constexpr size_t parse_length = std::string::traits_type::length(uavcan_prefix);
                    if (parse_remaining > parse_length
                        && strncmp(uavcan_prefix, parse_position, parse_length) != 0) {
                        i++; // Skip the next which is a part of the set
                        continue;
                    }
                    parse_position += parse_length;
                    parse_remaining -= parse_length;
                }

                {
                    // Check PORT_NAME itself
                    const size_t parse_length = strlen(registers[i].name);
                    if (parse_remaining > parse_length
                        && strncmp(registers[i].name, parse_position, parse_length) != 0) {
                        i++; // Skip the next which is a part of the set
                        continue;
                    }
                    parse_position += parse_length;
                    parse_remaining -= parse_length;
                }

                {
                    // Check suffix ".id"
                    static constexpr const char *id_suffix = ".id";
                    static constexpr size_t parse_length = std::string::traits_type::length(id_suffix);
                    if (parse_remaining == parse_length
                        && strncmp(id_suffix, parse_position, parse_length) == 0) {
                        // Found the ID register
                        response._mutable = false;
                        response.persistent = true;
                        uavcan_register_Value_1_0_select_natural16_(&response.value);
                        response.value.natural16.value.elements[0] = registers[i].port_name_id;
                        response.value.natural16.value.count = 1;
                        break;
                    }
                }

                {
                    // Check suffix ".type"
                    static constexpr const char *type_suffix = ".type";
                    static constexpr size_t parse_length = std::string::traits_type::length(type_suffix);
                    if (parse_remaining == parse_length
                        && strncmp(type_suffix, parse_position, parse_length) == 0) {
                        // Found the type register
                        response._mutable = false;
                        response.persistent = true;
                        uavcan_register_Value_1_0_select_string_(&response.value);
                        strncpy(reinterpret_cast<char *>(response.value._string.value.elements), registers[i + 1].name, sizeof(response.value._string.value.elements));
                        response.value._string.value.count = strnlen(reinterpret_cast<const char *>(response.value._string.value.elements), sizeof(response.value._string.value.elements));
                        break;
                    }
                }
            }
        }
        if (i >= register_count) { // Register not found
            response._mutable = false;
            response.persistent = false;
            uavcan_register_Value_1_0_select_empty_(&response.value);
        }

        // Add timestamp
        if (time_sync != nullptr) {
            response.timestamp.microsecond = std::max<int64_t>(time_sync->get_remote(get_timestamp_us()), 0);
        } else {
            response.timestamp.microsecond = 0; // Needs to be network time, use 0 if not available
        }

        // Send the response
        server_access.send_response(response);
    }

    /**
     * @brief Response to register list requests.
     * @param data register index to describe
     */
    void list(const uavcan_register_List_Request_1_0 &data) {
        uavcan_register_List_Response_1_0 response; ///< Large response object on stack

        if (data.index >= register_count) {
            response.name.name.count = 0; // No more registers
        } else if (registers[data.index].port_name_id == INT16_MAX) {
            // Copy the Set+Get register name
            strncpy(reinterpret_cast<char *>(response.name.name.elements), registers[data.index].name, sizeof(response.name.name.elements));
            response.name.name.count = strnlen(reinterpret_cast<char *>(response.name.name.elements), sizeof(response.name.name.elements));
        } else {
            // Create the PORT_NAME register name
            // Start with "uavcan."
            bool type_not_id = data.index > 0 && registers[data.index - 1].port_name_id == registers[data.index].port_name_id;
            char *write_position = reinterpret_cast<char *>(response.name.name.elements);
            size_t part_length = strlen("uavcan.");
            strcpy(write_position, "uavcan.");
            write_position += part_length;

            // Add "PORT_NAME"
            const size_t port_name_index = type_not_id ? data.index - 1 : data.index; // Name is stored in the lower of the set
            const size_t suffix_length = type_not_id ? strlen(".type") : strlen(".id");
            part_length = std::min(strlen(registers[port_name_index].name), sizeof(response.name.name.elements) - part_length - suffix_length);
            strncpy(write_position, registers[port_name_index].name, part_length);
            write_position += part_length;

            // Add ".type" or ".id"
            if (type_not_id) {
                strcpy(write_position, ".type");
                write_position += suffix_length;
            } else {
                strcpy(write_position, ".id");
                write_position += suffix_length;
            }

            // Used length
            response.name.name.count = write_position - reinterpret_cast<char *>(response.name.name.elements);
        }

        // Send the response
        server_list.send_response(response);
    }

public:
    /**
     * @brief Interface for handling registers without its own storage.
     * @param max_registers_ maximum number of registers and size of the registers_ array
     * @param registers_ pointer to the array of registers, must be valid for the lifetime of this object
     */
    RegisterMachineIface(size_t max_registers_, ProtoRegister *registers_)
        : server_access(
            [this](const uavcan_register_Access_Request_1_0 &data, [[maybe_unused]] const ProtoSuber::Meta &meta) { access(data); },
            ProtoSender::send_timeout_default, ProtoSuber::multipart_timeout_default)
        , server_list(
              [this](const uavcan_register_List_Request_1_0 &data, [[maybe_unused]] const ProtoSuber::Meta &meta) { list(data); },
              ProtoSender::send_timeout_default, ProtoSuber::multipart_timeout_default)
        , registers(registers_)
        , max_registers(max_registers_) {
    }

    /**
     * @brief Add a register.
     * @param name name of the register, naming requirements are described in "uavcan/register/384.Access.1.0.dsdl"
     *      @warning Parameter name must stay valid and unchanged for the lifetime of this object. "constexpr char*" is a good choice.
     * @param set_get function to first set and then get the value of the register, be sure to check uavcan_register_Value_1_0_is_empty_() before writing
     * @param mutable reported to the client, true means that the value can be writen
     * @param persistent reported to the client, true means that the value should survive device reset
     */
    void add_register(const char *name, SetGetCallback set_get, bool is_mutable, bool is_persistent = false) {
        if (register_count + 1 <= max_registers) {
            registers[register_count++] = { .name = name, .port_name_id = INT16_MAX, .is_persistent = is_persistent, .is_mutable = is_mutable, .set_get = set_get };
        } else {
            debug_assert(false);
        }
    }

    /**
     * @brief Add a set of two registers, PORT_NAME.id and PORT_NAME.type.
     * @note It uses two register slots.
     * @param port_name name of the port, needs to be in format "typ.port_name" where "typ" is "pub", "sub", "cln" or "srv", as in: "pub.test_out"
     * @param data_type DSDL data type of the port as in: "uavcan.primitive.scalar.Real32.1.0"
     * @param port_id port-ID of the port
     * @note More info in "uavcan/register/384.Access.1.0.dsdl".
     */
    void add_port_name_set(const char *port_name, const char *data_type, CanardPortID port_id) {
        if (register_count + 2 <= max_registers) {
            registers[register_count++] = { .name = port_name, .port_name_id = port_id, .is_persistent = true, .is_mutable = false, .set_get = nullptr };
            registers[register_count++] = { .name = data_type, .port_name_id = port_id, .is_persistent = true, .is_mutable = false, .set_get = nullptr };
        } else {
            debug_assert(false);
        }
    }

    /// @note Registers cannot be removed for List to work, so no remove_register().

    /**
     * @brief Set the time synchronization object.
     * Log timestamps should be in network time, use time_sync to convert.
     * @param time_sync_ the time synchronization object
     */
    void set_time_sync(TimeSync *time_sync_ = nullptr) {
        time_sync = time_sync_;
    }

    /**
     * @brief Add itself to task and to portlist.
     * @note Does not add port name and type registers (not needed for uavcan types).
     * @param port_list node's object publishing used ports
     */
    void init(PortList &port_list) {
        server_list.add_to_task();
        server_access.add_to_task();

        port_list.add(server_list);
        port_list.add(server_access);
    }

    /// @brief Server port-IDs.
    struct RegisterIds {
        CanardPortID access; ///< uavcan.register.Access.1.0
        CanardPortID list; ///< uavcan.register.List.1.0
    };

    /// @brief Get server port-IDs.
    [[nodiscard]] RegisterIds get_server_ids() const {
        return { .access = server_access.get_server_id(), .list = server_list.get_server_id() };
    }

    /// @returns maximum supported register count
    [[nodiscard]] size_t get_max_registers() const {
        return max_registers;
    }

    /// @returns number of registers added so far
    [[nodiscard]] size_t get_register_count() const {
        return register_count;
    }
};

/// @brief Template that stores registers inside.
template <size_t MAX_REGISTERS_>
class RegisterMachine : public RegisterMachineIface {
    RegisterMachineIface::ProtoRegister registers_array[MAX_REGISTERS_];

public:
    static constexpr size_t MAX_REGISTERS = MAX_REGISTERS_;

    RegisterMachine()
        : RegisterMachineIface(MAX_REGISTERS, registers_array) {}
};

} // namespace can::cyphal

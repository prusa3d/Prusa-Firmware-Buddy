/// @file
#pragma once

#include "cyphal_application.hpp"
#include "cyphal_nfc_node.hpp"
#include "cyphal_presentation.hpp"
#include <anfc/modbus.hpp>
#include "hal_rng.hpp"
#include "master_activity.hpp"
#include "yet_another_circular_buffer.hpp"
#include <algorithm>
#include <cstddef>
#include <optional>
#include <string_view>
#include <string.h>
#include <utils/overloaded_visitor.hpp>
#include <variant>
#include <bsod/bsod.h>
#include <utils/byte_utils.hpp>

namespace {

constexpr auto heartbeat_max_publication_period = std::chrono::seconds { 1 };
constexpr auto heartbeat_offline_timeout = std::chrono::seconds { 3 };
constexpr auto execute_command_timeout = std::chrono::seconds { 1 };

// How long must there be silence / no activity around a transfer to give up on it.
//
// This explicitly deals with activity around the transfer, not heartbeats
// (they are required to be at least once every 3 seconds, if we wanted to
// give it a chance to miss one, we would need at least 6s, which seems
// already too high).
constexpr auto transfer_inactivity_timeout = std::chrono::seconds { 2 };

uint32_t get_random_salt() {
    return hal::rng::get();
}

// Note: We do not actually need to name the firmware file.
//       Each node will request just a single file while in bootloader.
//       We just invent some short string here and call it a day.
static const std::string_view dummy_parameter_sv = "/path/to/fw";
static const Bytes dummy_parameter = std::as_bytes(std::span { dummy_parameter_sv });

} // namespace

namespace cyphal {

/// For now, there is a one-to-one correspondence; this may be extended with
/// different hardware revisions of the same logical node in the future.
inline FirmwareFile get_firmware_file_for_node_name(NodeName node_name) {
    switch (node_name) {
    case NodeName::none:
        break;
    case NodeName::cz_prusa3d_honeybee_ac_controller:
        return FirmwareFile::firmware_ac_controller;
    case NodeName::cz_prusa3d_honeybee_nfc:
        return FirmwareFile::firmware_anfc;
    case NodeName::cz_prusa3d_honeybee_tool_offset_sensor:
        return FirmwareFile::firmware_tool_offset_sensor;
    }
    return FirmwareFile::none;
}

/// Application layer of the cyphal implementation.
class ApplicationImpl final : public Application {
private:
    TimePoint last_heartbeat;
    // Suspend heartbeats if the master shows no activity.
    uint16_t last_master_activity = 0;
    const char *last_warning;

    struct AcController;

    struct Filesystem {
        // The file being requested over modbus from the printer.
        FirmwareFile file;

        // The file transfer that's currently running is at the end - there's
        // whatever is sitting in the buffer right now and that's it, there
        // won't be more.
        bool eof;

        // We need to update the registers exposed over modbus at the nearest
        // time of our convenience.
        //
        // Note that this might be delayed further by eg. not having enough
        // space in the buffer.
        bool delayed_update = false;

        // The offset at which the current read end of the buffer is, relative
        // to the start of the transfered file.
        //
        // The write offset is read_offset + buffer.size();
        uint32_t read_offset = 0;

        // There's (possibly) a request in one of our nodes, waiting to be handled.
        //
        // Note that this can have false positives - we reset it to false only
        // when we don't find such node, to save us some searching next time.
        bool request_in_node = false;

        // A node that's currently being taken care of with the current transfer.
        std::optional<uint8_t> current_node;
        YetAnotherCircularBuffer<1024> buffer;

        // When did "something" happen around the transfer. If nothing is
        // happening on either side for too long, we abort the transfer and
        // move to another one, possibly.
        TimePoint last_activity;

        // The request on modbus.
        ModbusRequest modbus_request;

        FirmwareFile hash_request = FirmwareFile::none;
        FirmwareFile hash_response = FirmwareFile::none;
        uint32_t hash_salt = 0;
        std::array<std::byte, 32> hash_digest;
        xbuddy_extension::DigestStatus hash_status = xbuddy_extension::DigestStatus::unavailable;

        void update_modbus_request() {
            modbus_request.hash_request = hash_request;
            modbus_request.hash_salt = hash_salt;

            if (!delayed_update) {
                // Nothing has changed, nothing to update.
                return;
            }

            if (eof) {
                // If the printer sent us the whole file, we wait for the
                // buffer to completely drain before doing anything else (like
                // trying to request a different file for different device), to
                // make sure we don't mix anything up.
                if (buffer.size() > 0) {
                    return;
                }
            } else {
                // If we are somewhere in the middle of the file, we just need
                // to make sure we have space for at least one chunk, so we
                // don't have to throw it away once it arrives.
                if (buffer.available() < sizeof(xbuddy_extension::modbus::Chunk::data)) {
                    return;
                }
            }

            delayed_update = false;

            if (file != modbus_request.flash_request) {
                modbus_request.flash_request = file;
            }
            modbus_request.offset = read_offset + buffer.size();
        }

        void reset() {
            eof = false;
            delayed_update = true;
            buffer.clear();
            file = FirmwareFile::none;
            current_node = std::nullopt;
        }
    };
    Filesystem filesystem;

    struct AcController {
        ac_controller::Config desired;
        ac_controller::Config active;
        ac_controller::LedConfig led_desired;
        ac_controller::LedConfig led_active;
        ac_controller::Status status;
        bool seen_status = false;
    };
    AcController ac_controller;

    struct ToolOffsetSensor {
        tool_offset_sensor::Config desired = tool_offset_sensor::Config { .ch0_enabled = false, .ch1_enabled = false };
        tool_offset_sensor::Config active;
        tool_offset_sensor::Status status;
        bool seen_status = false;
    };
    ToolOffsetSensor tool_offset_sensor;

    CyphalBridgeQueue<512> bridge_queue_;

    using NfcNodes = std::array<std::pair<anfc::Device, NfcNode>, anfc::device_count>;
    NfcNodes nfc_nodes { {
        { anfc::Device::anfc0, {} },
        { anfc::Device::anfc1, {} },
    } };
    NfcNodes::iterator next_nfc_node = nfc_nodes.begin();

    /// Buffer for the latest log message; messages not fitting will be truncated.
    struct LogBuffer {
        size_t text_size = 0;
        uint16_t sequence = 0;
        std::array<std::byte, sizeof(xbuddy_extension::modbus::LogMessage::text_data)> text;
    } log_buffer;

    /// Represents all the info we have/need about the node.
    struct Node {
        UniqueId unique_id;
        TimePoint heartbeat_timestamp;
        Heartbeat heartbeat_data;

        struct NotAllocated {};
        struct InitialInfo {
            bool info_request_sent = false;
        };
        struct Verify {
            uint32_t salt;
            bool request_sent : 1 = false;
            bool received_from_device : 1 = false;
            bool received_from_source : 1 = false;
            bool matches : 1 = false;
            // Filled in by the first one we receive (either from source or device).
            std::array<std::byte, 32> digest;

            /// Received a hash
            ///
            /// Assumes the caller adjusts the received_form_* beforehand.
            void received(const Bytes &new_digest) {
                if (digest.size() != new_digest.size()) {
                    // TODO BFW-7918
                    // Or assert?
                    return;
                }

                if (finalized()) {
                    matches = memcmp(digest.data(), new_digest.data(), digest.size()) == 0;
                } else {
                    memcpy(digest.data(), new_digest.data(), digest.size());
                }
            }

            bool finalized() const {
                return received_from_device && received_from_source;
            }
        };
        struct Flash {
            // Node wants some data, this is the ID specified by the node.
            //
            // nullopt in case we are waiting for the node's request to arrive.
            std::optional<TransferId> transfer_id;
            // Offset into the "file" the node wants.
            //
            // Used to:
            // * Restart if we "jump"/rewind.
            // * Know where to start the original request.
            uint32_t req_offset;
        };
        struct AcControllerAlive {
            AcController *ac_controller = nullptr;
            ~AcControllerAlive() {
                ac_controller->seen_status = false;
            }
        };
        struct ToolOffsetSensorAlive {
            ToolOffsetSensor *tool_offset_sensor = nullptr;
            ~ToolOffsetSensorAlive() {
                tool_offset_sensor->seen_status = false;
            }
        };

        struct NfcAlive {
            anfc::Device device;
        };
        struct Inert {
        };
        /// A node can be in one of these states during its lifetime.
        ///
        /// * NotAllocated - the node is not yet allocated, free slot. Never returns to this state after leaving it.
        /// * InitialInfo - waiting for initial heartbeat and node info.
        /// * Verify - we want to verify it's firmware's hash.
        /// * Flash - depending on the verification result, we want to load a new firmware into the device. Or we can skip this phase.
        /// * AcControllerAlive - AC controller node is active and operational.
        /// * NfcAlive - NFC node is active and operational.
        /// * ToolOffsetSensorAlive - tool offset sensor node is active and operational.
        /// * Inert - "trash" state, for nodes of unknown state or otherwise weird situations we don't know what to do about.
        using State = std::variant<
            // No state, before the node is properly allocated and born.
            NotAllocated,
            InitialInfo,
            Verify,
            Flash,
            AcControllerAlive,
            NfcAlive,
            ToolOffsetSensorAlive,
            Inert>;
        State state;

        /// In order to conserve memory, we do not use std::optional but
        /// rather store these flags packed here.
        struct {
            bool allocation_response_sent : 1;
            bool heartbeat_valid : 1;
            bool last_command_valid : 1;
            // TODO BFW-7918
            // Who resets this?
            bool start_app_sent : 1;
        };
        struct {
            NodeName name;
        } info;
        struct {
            TimePoint timepoint;
            Command command;
        } last_command;

        /// If verified, move to Alive (depending on the node name).
        State try_activate(ApplicationImpl *application, const Verify &verify) {
            if (verify.finalized()) {
                if (verify.matches) {
                    switch (info.name) {
                    case NodeName::none:
                        break;
                    case NodeName::cz_prusa3d_honeybee_ac_controller:
                        return AcControllerAlive {
                            .ac_controller = &application->ac_controller,
                        };
                    case NodeName::cz_prusa3d_honeybee_nfc:
                        return application->allocate_nfc_device();
                    case NodeName::cz_prusa3d_honeybee_tool_offset_sensor:
                        // Activation is also executed when TOS board was reset
                        // This will ensure the actual configuration is re-applied after reset, if needed
                        application->tool_offset_sensor.active = tool_offset_sensor::Config {};

                        return ToolOffsetSensorAlive {
                            .tool_offset_sensor = &application->tool_offset_sensor,
                        };
                    }
                    // We passed verification, but don't know what to do about the node, so...
                    return Inert {};
                } else {
                    // We know the hash doesn't match -> reflash
                    return Flash {};
                }
            }
            // else -> waiting for more answers
            return verify;
        }

        bool execute_command(Presentation &presentation, TimePoint now, NodeId node_id, Command command, Bytes parameter) {
            if (last_command_valid && now < last_command.timepoint + execute_command_timeout) {
                // Waiting for response. Let other nodes make progress.
                return false;
            }

            presentation.transmit_node_execute_command_request(node_id, command, parameter);
            last_command.timepoint = now;
            last_command.command = command;
            last_command_valid = true;
            return true;
        }

        /// Step internal state machine for particular node.
        ///
        /// This handles plug-and-play node allocation, identification, timeouts,
        /// requests to start nodes, flash and verify their firmware and so on.
        ///
        /// Return true if the state machine made progress.
        bool step(Presentation &presentation, const TimePoint now, ApplicationImpl *application, NodeId node_id) {
            if (!allocated()) {
                return false;
            }

            if (!allocation_response_sent) {
                // Send the allocation response. Failure to send is not considered an error.
                // If it fails, we will respond to the next allocation request.
                presentation.transmit_pnp_allocation(unique_id, node_id);
                allocation_response_sent = true;
                return true;
            }

            if (heartbeat_valid && now > heartbeat_timestamp + heartbeat_offline_timeout && verified()) {
                // Node may have died. Let's forget everything we know about it.
                heartbeat_valid = false;
                state = InitialInfo {
                    .info_request_sent = false,
                };
                last_command_valid = false;
                start_app_sent = false;
                presentation.transmit_diagnostic_record(Severity::warning, "lost heartbeat");
                return true;
            }

            if (!heartbeat_valid) {
                // We may have lost the heartbeat, or we may have invalidated it (as
                // part of restart or start_app commands).
                return false;
            }

            return match(state, [&](auto &state) {
                return step(presentation, now, application, node_id, state);
            });
        }
        // Overloads for the internal substep based on current state.

        bool step(Presentation &, const TimePoint, ApplicationImpl *, NodeId, NotAllocated &) {
            return false;
        }

        bool step(Presentation &, const TimePoint, ApplicationImpl *, NodeId, Inert &) {
            return false;
        }

        bool step(Presentation &presentation, const TimePoint, ApplicationImpl *, NodeId node_id, InitialInfo &initial) {
            // At this point, allocation response has been sent.

            if (!initial.info_request_sent) {
                presentation.transmit_node_get_info_request(node_id);
                initial.info_request_sent = true;
                return true;
            }

            // Waiting for the info to arrive.
            return false;
        }

        bool step(Presentation &presentation, const TimePoint now, ApplicationImpl *application, NodeId node_id, Verify &verify) {
            Filesystem &filesystem = application->filesystem;
            switch (heartbeat_data.mode) {
            case Mode::software_update:
                // We want to ask for the hash, but for that, we need to get to the app mode first.
                switch (heartbeat_data.health) {
                case Health::nominal:
                    if (heartbeat_data.vendor_specific_status_code > 0) {
                        // AppUpdateInProgress => keep responding to file.Read requests.
                        // TODO BFW-7918
                        // This one is probably illegal/happens in
                        // Flash? Or, can it happen this one is still
                        // "finishing" after we have sent the last chunk
                        // and we are already in Verify, but the node isn't
                        // yet?
                    } else {
                        // BootDelay => wait for application to boot.
                    }
                    // Let other nodes make progress.
                    return false;
                case Health::advisory:
                    // BootCancelled
                    // We need to boot the app to verify the digest
                    if (start_app_sent) {
                        // We sent the request to start app, but didn't make any progress.
                        // So the app is botched and we reflash it.
                        state = Flash {};
                        return true;
                    } else {
                        return execute_command(presentation, now, node_id, Command::start_app, {});
                    }
                case Health::caution:
                    // Out of spec => do nothing
                    return false;
                case Health::warning:
                    // NoAppToBoot => send request to update software
                    state = Flash {};
                    return true;
                }
                return false;
            case Mode::initialization:
            case Mode::maintenance:
            case Mode::operational: {
                if (!verify.request_sent) {
                    // We haven't sent our requests yet.
                    if (filesystem.hash_request == FirmwareFile::none) {
                        // Slot is open, let's request hash from the parent system
                        const auto salt = get_random_salt();
                        verify.salt = salt;
                        // Request the hash from the parent system
                        filesystem.hash_request = get_firmware_file_for_node_name(info.name);
                        filesystem.hash_salt = salt;
                        presentation.transmit_diagnostic_record(Severity::notice, "hash requested");
                        // And also request it from the device at the same time.
                        const bool sent = execute_command(presentation, now, node_id, Command::get_app_salted_hash, trivial_as_bytes(salt));
                        if (sent) {
                            verify.request_sent = true;
                        }
                        return sent;
                    } else {
                        // Wait for slot to open.
                        return false;
                    }
                }

                if (!verify.received_from_source) {
                    // We are waiting for the digest from the board.
                    if (filesystem.hash_response == get_firmware_file_for_node_name(info.name) && filesystem.hash_salt == verify.salt) {
                        switch (filesystem.hash_status) {
                        case xbuddy_extension::DigestStatus::ok:
                            verify.received_from_source = true;
                            verify.received(filesystem.hash_digest);
                            state = try_activate(application, verify);
                            presentation.transmit_diagnostic_record(Severity::notice, "hash received: ok");
                            filesystem.hash_request = filesystem.hash_response = FirmwareFile::none;
                            break;
                        case xbuddy_extension::DigestStatus::unavailable:
                            state = Inert {};
                            presentation.transmit_diagnostic_record(Severity::warning, "hash received: unavailable");
                            filesystem.hash_request = filesystem.hash_response = FirmwareFile::none;
                            break;
                        case xbuddy_extension::DigestStatus::retry:
                            std::swap(filesystem.hash_request, filesystem.hash_response);
                            break;
                        }
                        return true;
                    }
                    return false;
                }
                // Other states are handled somewhere else.
                return false;
            }
            }
            return false;
        }

        bool step(Presentation &presentation, const TimePoint now, ApplicationImpl *, NodeId node_id, Flash &) {
            switch (heartbeat_data.mode) {
            case Mode::initialization:
            case Mode::operational:
                // We need reflashing (hash doesn't match), go to the bootloader again.
                //
                // Note: We don't get a response to restart, so we invalidate
                // heartbeat here already, not in the answer.
                heartbeat_valid = false;
                return execute_command(presentation, now, node_id, Command::restart, {});
            case Mode::software_update:
                // Bootloader mode, see https://github.com/Zubax/kocherga
                switch (heartbeat_data.health) {
                case Health::nominal:
                    if (heartbeat_data.vendor_specific_status_code > 0) {
                        // AppUpdateInProgress => keep responding to file.Read requests.
                    } else {
                        // BootDelay => wait for application to boot.
                        // TODO BFW-7918
                        // Illegal in Flash?
                    }
                    // Let other nodes make progress.
                    return false;
                case Health::advisory:
                    // BootCancelled
                    return execute_command(presentation, now, node_id, Command::software_update, dummy_parameter);
                case Health::caution:
                    // Out of spec => do nothing
                    return false;
                case Health::warning:
                    // NoAppToBoot => send request to update software
                    return execute_command(presentation, now, node_id, Command::software_update, dummy_parameter);
                }
                return false;
            case Mode::maintenance:
                // ???
                return false;
            }
            return false;
        }

        bool step(Presentation &presentation, const TimePoint, ApplicationImpl *, NodeId node_id, AcControllerAlive &alive) {
            if (alive.ac_controller->active != alive.ac_controller->desired) {
                alive.ac_controller->active = alive.ac_controller->desired;
                presentation.transmit_ac_controller_config_request(node_id, alive.ac_controller->active);
                return true;
            }
            if (alive.ac_controller->led_active != alive.ac_controller->led_desired) {
                alive.ac_controller->led_active = alive.ac_controller->led_desired;
                presentation.transmit_ac_controller_leds_config_request(node_id, alive.ac_controller->led_active);
                return true;
            }
            return false;
        }

        bool step(Presentation &presentation, const TimePoint, ApplicationImpl *, NodeId node_id, ToolOffsetSensorAlive &alive) {
            if (heartbeat_data.mode == Mode::software_update) {
                // The node has been reset back into its bootloader - go to Verify state
                heartbeat_valid = false;
                state = Verify {};
                last_command_valid = false;
                start_app_sent = false;
                presentation.transmit_diagnostic_record(Severity::warning, "TOS reset, restarting app");
                return true;
            }
            if (alive.tool_offset_sensor->active != alive.tool_offset_sensor->desired) {
                alive.tool_offset_sensor->active = alive.tool_offset_sensor->desired;
                presentation.transmit_tool_offset_sensor_config_request(node_id, alive.tool_offset_sensor->active);
                return true;
            }
            return false;
        }

        bool step(Presentation &presentation, const TimePoint, ApplicationImpl *application, NodeId node_id, NfcAlive &alive) {
            return application->get_nfc(alive.device).step(presentation, node_id);
        }

        bool verified() const {
            return match(
                state,
                [](const AcControllerAlive &) { return true; },
                [](const NfcAlive &) { return true; },
                [](const ToolOffsetSensorAlive &) { return true; },
                [](const Verify &verify) { return verify.finalized() && verify.matches; },
                [](const auto &) { return false; });
        }

        bool allocated() const {
            return match(
                state,
                [](const NotAllocated &) { return false; },
                [](const auto &) { return true; });
        }
    };

    /// Contains information about all the cyphal nodes on the network.
    /// We are the node 0, and we store our own info in the same array.
    /// Node ID itself is not stored in the structure and is implicitly
    /// given by its index in the array.
    std::array<Node, 16> nodes = {};

    /// Return Node structure corresponding to given node_id or nullptr
    /// if there is none such node.
    /// Node_id must have been previously allocated.
    Node *get_node(NodeId node_id) {
        const size_t index = (uint8_t)node_id;
        Node *node = index < nodes.size() ? &nodes[index] : nullptr;
        if (node && std::holds_alternative<Node::NotAllocated>(node->state)) {
            node = nullptr;
        }
        if (!node) {
            // We are the only node which allocates ID and we would never allocate this ID.
            // Something very fishy is happening on the bus, yet there is nothing we can do.
            last_warning = "rogue node detected";
        }
        return node;
    }

    /// Step internal state machine for node of this application.
    ///
    /// The only strictly required functionality is periodic publication
    /// of heartbeats. On top of that, we also send warnings to aid debugging.
    /// Other than that, this node does not need to support any other
    /// functionality. It only costs FLASH and RAM which we don't have.
    ///
    /// Return true if the state machine made progress.
    bool application_step(Presentation &presentation, const TimePoint now) {
        if (now > last_heartbeat + heartbeat_max_publication_period) {
            const uint32_t uptime = duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
            const uint16_t master_activity_snapshot = master_activity.load(std::memory_order_relaxed);
            const bool healthy = master_activity_snapshot != last_master_activity;
            presentation.transmit_heartbeat(uptime, healthy);
            last_heartbeat = now;
            last_master_activity = master_activity_snapshot;
            return true;
        }
        if (last_warning) {
            presentation.transmit_diagnostic_record(Severity::warning, last_warning);
            last_warning = nullptr;
            return true;
        }
        return false;
    }

    /// Step internal state machine for uavcan.read.* operations.
    /// Return true if the state machine made progress.
    bool filesystem_step(Presentation &presentation, const TimePoint now) {
        // No transfer currently running, but there's a request waiting on some node (possibly).
        if (!filesystem.current_node && filesystem.request_in_node) {
            debug_assert(filesystem.file == FirmwareFile::none);
            debug_assert(filesystem.buffer.size() == 0);
            bool found = false;
            for (size_t i = 0; i < nodes.size(); i++) {
                Node &node = nodes[i];
                if (auto *flash = std::get_if<Node::Flash>(&node.state)) {
                    if (flash->transfer_id) {
                        if (node.info.name == NodeName::none) {
                            // There's something connected. We have no idea what
                            // that thing is and what file it wants, but we don't
                            // have any for it. Let it wait forever.
                            node.state = Node::Inert {};
                            // Give up on this node, try looking into the next one.
                            continue;
                        } else {
                            filesystem.file = get_firmware_file_for_node_name(node.info.name);
                        }
                        filesystem.eof = false;
                        filesystem.delayed_update = true;
                        filesystem.read_offset = flash->req_offset;
                        // Note: Do *not* reset the filesystem.request_in_node just yet
                        // - there may be some other node waiting that also wants a
                        // file.
                        filesystem.current_node = i;
                        filesystem.last_activity = now;
                        found = true;
                        break;
                    }
                } else {
                    continue;
                }
            }
            if (!found) {
                // Don't try searching next time, unless something changes.
                filesystem.request_in_node = false;
            }
        }

        if (Node::Flash *flash = filesystem.current_node ? std::get_if<Node::Flash>(&nodes[*filesystem.current_node].state) : nullptr; flash != nullptr && flash->transfer_id) {
            Node &node = nodes[*filesystem.current_node];
            if (flash->req_offset != filesystem.read_offset) {
                // The CAN device wants to rewind/jump in the file. We don't
                // _directly_ handle these situations (too complex, too rare), we
                // simply drop all the state and start a new transfer from the
                // given position.
                filesystem.reset();
                return true;
            }
            size_t read_size = 256;
            std::byte buf[256];
            if (filesystem.eof) {
                read_size = std::min(read_size, filesystem.buffer.size());
            }
            if (filesystem.buffer.try_read(buf, read_size)) {
                presentation.transmit_file_read_response(NodeId { *filesystem.current_node }, *flash->transfer_id, { buf, read_size });
                filesystem.last_activity = now;
                // This chunk got sent, but the flashing continues, we expect we'll get another request.
                flash->transfer_id = std::nullopt;

                // Transfer done.
                if (filesystem.eof && filesystem.buffer.size() == 0) {
                    filesystem.current_node = std::nullopt;
                    node.state = Node::Verify {};
                }

                return true;
            } else {
                // No data available - check for silence.
                filesystem_check_timeout(now);
            }
        } else if (filesystem.current_node) { // But no transfer_id, no request from the CAN device -> check for silence.
            filesystem_check_timeout(now);
        }

        return false;
    }

    void filesystem_check_timeout(TimePoint now) {
        if (filesystem.current_node && now > filesystem.last_activity + transfer_inactivity_timeout) {
            // No activity for a long time. This transfer is likely dead, we cancel it and possibly go to the next one.
            nodes[*filesystem.current_node].state = Node::Flash {};
            filesystem.reset();
        }
    }

public:
    ApplicationImpl()
        : last_warning { nullptr } {
        filesystem.reset();
    }

    /// Step internal state machine.
    /// Return true if the state machine made progress.
    bool step(Presentation &presentation, TimePoint now) final {
        if (application_step(presentation, now)) {
            return true;
        }
        if (filesystem_step(presentation, now)) {
            return true;
        }
        for (uint8_t i = 1; i < nodes.size(); ++i) {
            const auto node_id = NodeId { i };
            auto &node = nodes[i];
            if (node.step(presentation, now, this, node_id)) {
                return true;
            }
        }
        return false;
    }

    void receive_pnp_allocation(const UniqueId &unique_id) final {
        // Try finding it in the allocation table first.
        for (uint8_t i = 1; i < nodes.size(); ++i) {
            Node &node = nodes[i];
            if (node.allocated() && node.unique_id == unique_id) {
                // Already allocated, send allocation response and we are done.
                node.allocation_response_sent = false;
                node.heartbeat_valid = false;
                return;
            }
        }

        // First time encountered, find a slot in the allocation table.
        for (uint8_t i = 1; i < nodes.size(); ++i) {
            Node &node = nodes[i];
            if (!node.allocated()) {
                // Slot found, add to allocation table and we are done.
                node.unique_id = unique_id;
                node.state = Node::InitialInfo {};
                return;
            }
        }

        // First time encountered, but there is no space in allocation table.
        last_warning = "allocation table is full";
    }

    void receive_node_heartbeat(NodeId remote_node_id, TimePoint now, const Heartbeat &heartbeat) final {
        if (Node *node = get_node(remote_node_id)) {
            node->heartbeat_data = heartbeat;
            node->heartbeat_timestamp = now;
            node->heartbeat_valid = true;
        }
    }

    void receive_node_execute_command_response(NodeId remote_node_id, uint8_t status, Bytes output) final {
        if (Node *node = get_node(remote_node_id)) {
            if (!node->last_command_valid) {
                // expired, ignore
                return;
            }
            node->last_command_valid = false;
            if (status == 2) {
                // not authorized == might as well be without a hash
                node->state = Node::Flash {};
                last_warning = "hash invalid";
                return;
            }
            if (status != 0) {
                node->heartbeat_valid = false;
                return;
            }
            switch (node->last_command.command) {
            case Command::start_app:
                node->start_app_sent = true;
                node->heartbeat_valid = false;
                break;
            case Command::get_app_salted_hash:
                if (auto *verify = std::get_if<Node::Verify>(&node->state); verify != nullptr) {
                    verify->received_from_device = true;
                    verify->received(output);
                    node->state = node->try_activate(this, *verify);
                } else {
                    // TODO BFW-7918
                    // Can it happen?
                }
                break;
            case Command::software_update:
                node->heartbeat_valid = false;
                break;
            case Command::restart:
                node->heartbeat_valid = false;
                break;
            }
        }
    }

    void receive_node_get_info_response(NodeId remote_node_id, Bytes name) final {
        if (Node *node = get_node(remote_node_id)) {
            node->info.name = parse_node_name(name);
            if (std::holds_alternative<Node::InitialInfo>(node->state)) {
                if (node->info.name == NodeName::none) {
                    last_warning = "unknown node name";
                    node->state = Node::Inert {};
                } else {
                    node->state = Node::Verify {};
                }
            }
        }
    }

    void receive_file_read_request(NodeId remote_node_id, TimePoint now, TransferId transfer_id, uint32_t offset) final {
        if (Node *node = get_node(remote_node_id)) {
            // Should we _check_ we are in the right state first?
            node->state = Node::Flash {
                .transfer_id = transfer_id,
                .req_offset = offset
            };
            filesystem.request_in_node = true;
            filesystem.delayed_update = true;
            if (filesystem.current_node && &nodes[*filesystem.current_node] == node) {
                filesystem.last_activity = now;
            }
        }
    }

    xbuddy_extension::NodeState get_node_state(NodeName name) const {
        using xbuddy_extension::NodeState;
        for (const auto &node : nodes) {
            if (node.info.name != name) {
                continue;
            }
            auto state = match(
                node.state,
                [](const ApplicationImpl::Node::NotAllocated &) { return NodeState::unknown; },
                [](const ApplicationImpl::Node::InitialInfo &) { return NodeState::unknown; },
                [](const ApplicationImpl::Node::Verify &) { return NodeState::verify; },
                [](const ApplicationImpl::Node::Flash &) { return NodeState::flash; },
                [](const ApplicationImpl::Node::AcControllerAlive &alive) {
                    return alive.ac_controller->seen_status ? NodeState::ready : NodeState::verify;
                },
                [](const ApplicationImpl::Node::ToolOffsetSensorAlive &alive) {
                    return alive.tool_offset_sensor->seen_status ? NodeState::ready : NodeState::verify;
                },
                [](const ApplicationImpl::Node::NfcAlive &) { return NodeState::ready; },
                [](const ApplicationImpl::Node::Inert &) { return NodeState::unknown; });
            if (state != NodeState::unknown) {
                return state;
            }
        }
        return NodeState::unknown;
    }

    bool receive_chunk(const uint8_t *data, size_t size, bool is_last, uint16_t file_id, uint32_t offset) final {
        filesystem.update_modbus_request();
        const xbuddy_extension::modbus::ChunkRequest received_request = {
            file_id,
            static_cast<uint16_t>(offset & 0xFFFF),
            static_cast<uint16_t>(offset >> 16)
        };
        const xbuddy_extension::modbus::ChunkRequest expected_request = {
            static_cast<uint16_t>(filesystem.modbus_request.flash_request),
            static_cast<uint16_t>(filesystem.modbus_request.offset & 0xFFFF),
            static_cast<uint16_t>(filesystem.modbus_request.offset >> 16)
        };
        if (received_request != expected_request
            || filesystem.file == FirmwareFile::none /* Aborted, marker until we do an update_modbus_request) */) {
            // The chunk is some kind of retransmit or desync.
            //
            // We _acknowledge_ getting it intact (so we don't get another
            // retransmit), but throw it away, it's not useful to us now. We'll get
            // the correct one next time.
            return true;
        }
        if (filesystem.buffer.try_write((std::byte *)data, size)) {
            filesystem.eof = is_last;
            // We want next one, once we are sure there's enough space. We
            // check that in the update_modbus_request method, invoked when our
            // registers are read.
            filesystem.delayed_update = true;
            if (is_last) {
                filesystem.file = FirmwareFile::none;
            }
            return true;
        } else {
            // Should not happen? We check for enough space before requesting more.
            return false;
        }
    }

    const ModbusRequest &request() final {
        filesystem.update_modbus_request();
        return filesystem.modbus_request;
    }

    bool receive_digest(FirmwareFile file, uint32_t salt, xbuddy_extension::DigestStatus status, std::span<const std::byte, 32> digest) final {
        if (filesystem.hash_request == file && filesystem.hash_salt == salt) {
            filesystem.hash_status = status;
            // static_assert(filesystem.hash_digest.size() == digest.size());
            std::memcpy(filesystem.hash_digest.data(), digest.data(), digest.size());
            std::swap(filesystem.hash_response, filesystem.hash_request);
        }
        return true;
    }

    bool receive(const ac_controller::Config &config) final {
        ac_controller.desired = config;
        return true;
    }

    bool receive(const ac_controller::LedConfig &config) final {
        ac_controller.led_desired = config;
        return true;
    }

    bool receive(const tool_offset_sensor::Config &config) final {
        tool_offset_sensor.desired = config;
        return true;
    }

    void request_ac_controller(xbuddy_extension::NodeState &node_state, ac_controller::Status &status) final {
        node_state = get_node_state(NodeName::cz_prusa3d_honeybee_ac_controller);
        status = ac_controller.status;
    }

    void request_tool_offset_sensor(xbuddy_extension::NodeState &node_state, tool_offset_sensor::Status &status) final {
        node_state = get_node_state(NodeName::cz_prusa3d_honeybee_tool_offset_sensor);
        status = tool_offset_sensor.status;
    }

    CyphalBridgeQueue<512> &bridge_queue() final {
        return bridge_queue_;
    }

    void receive_ac_controller_status(const ac_controller::Config &config, const ac_controller::Status &status) final {
        ac_controller.active = config;
        ac_controller.status = status;
        ac_controller.seen_status = true;
    }

    void log_from_app(std::string_view s) {
        receive_diagnostic_record(NodeId {}, as_bytes(std::span { s }));
    }

    void receive_diagnostic_record(NodeId, const Bytes &text) final {
        log_buffer.sequence++;
        log_buffer.text_size = std::min(text.size(), log_buffer.text.size());
        memcpy(log_buffer.text.data(), text.data(), log_buffer.text_size);
    }

    LogData get_log() const final {
        auto text = std::span { log_buffer.text };
        return LogData {
            .sequence = log_buffer.sequence,
            .text = text.subspan(0, log_buffer.text_size),
        };
    }

    void receive_nfc_event(cyphal::NodeId node_id, Bytes data) final {
        if (Node *node = get_node(node_id)) {
            if (auto *alive = std::get_if<Node::NfcAlive>(&node->state)) {
                get_nfc(alive->device).receive_event(data);
            }
        }
    }

    [[nodiscard]] NfcNode &get_nfc(anfc::Device device_) final {
        for (auto &[device, node] : nfc_nodes) {
            if (device == device_) {
                return node;
            }
        }
        abort();
    }

    [[nodiscard]] Node::State allocate_nfc_device() {
        if (next_nfc_node == nfc_nodes.end()) {
            return Node::Inert {};
        }
        const auto device = next_nfc_node->first;
        ++next_nfc_node;
        return Node::NfcAlive { .device = device };
    }

    void receive_tool_offset_sensor_status(const tool_offset_sensor::Status &status) final {
        tool_offset_sensor.status = status;
        tool_offset_sensor.seen_status = true;
    }
};
} // namespace cyphal

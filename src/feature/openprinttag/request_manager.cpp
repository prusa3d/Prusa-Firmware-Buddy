/// @file
#include <feature/openprinttag/request_manager.hpp>

#include <bsod/bsod.h>
#include <cstring>
#include <feature/openprinttag/detail/requests_base.hpp>
#include <feature/openprinttag/tool_tag.hpp>
#include <modbus/traits.hpp>
#include <mutex>
#include <shared_mutex>
#include <prusa3d/nfc/command/AcceptEvent_1_0.h>
#include <prusa3d/nfc/command/Request_1_0.h>
#include <prusa3d/nfc/event/Event_1_0.h>
#include <puppies/cyphal_node_table_host.hpp>
#include <logging/log.hpp>
#include <timing.h>
#include <utils/byte_utils.hpp>
#include <option/has_anfc_for_multiple_tools.h>
#include <option/has_indx.h>
#include <option/has_mmu2.h>

LOG_COMPONENT_REF(OpenPrintTag);

static constexpr int32_t request_timeout_ms = 5000;

static void serialize_forget_tag(uint16_t request_id, buddy::openprinttag::TagID tag_id, anfc::modbus::Request &request) {
    prusa3d_nfc_command_Request_Request_1_0 object;
    memset(&object, 0, sizeof(object));
    object.request_id.value = request_id;
    object.request.forget_tag.tag.value = tag_id;
    prusa3d_nfc_request_RequestData_1_0_select_forget_tag_(&object.request);
    request.data = {};
    auto buffer = std::as_writable_bytes(std::span { request.data });
    size_t size = buffer.size();
    if (prusa3d_nfc_command_Request_Request_1_0_serialize_(&object, reinterpret_cast<uint8_t *>(buffer.data()), &size) == 0) {
        request.size = static_cast<uint16_t>(size);
    } else {
        bsod_unreachable();
    }
}

static void serialize_accept_event(uint16_t event_id, anfc::modbus::AcceptEvent &accept_event) {
    prusa3d_nfc_command_AcceptEvent_Request_1_0 object;
    memset(&object, 0, sizeof(object));
    object.event_id.value = event_id;
    accept_event.data = {};
    auto buffer = std::as_writable_bytes(std::span { accept_event.data });
    size_t size = buffer.size();
    if (prusa3d_nfc_command_AcceptEvent_Request_1_0_serialize_(&object, reinterpret_cast<uint8_t *>(buffer.data()), &size) == 0) {
        accept_event.size = static_cast<uint16_t>(size);
    } else {
        bsod_unreachable();
    }
}

namespace buddy::openprinttag {

Manager::Manager() {
    for (auto &device : devices) {
        device.manager = this;
    }

    /// For now we support only anfc device 0 for virtual tool 0
    devices[VirtualToolIndex::from_raw(0)].device = anfc::Device::anfc0;
}

Manager::~Manager() {
    // Destroy the requests first, otherwise ugly race conditions
    for (auto &device : devices) {
        device.enable_radio_request.reset();
    }
}

puppies::CommunicationStatus Manager::step(anfc::modbus::Client &client) {
    puppies::CommunicationStatus result = puppies::CommunicationStatus::SKIPPED;
    bool should_poll = false;

    {
        std::lock_guard lock { mutex };

        check_timeouts();

        if (puppies::cyphal_node_table().get_node_state(xbuddy_extension::NodeName::cz_prusa3d_honeybee_nfc) != xbuddy_extension::NodeState::ready) {
            return result;
        }

        for (DeviceState &device_state : devices) {
            result = puppies::aggregate_communication_status({ result, device_state.step(client) });
            if (result == puppies::CommunicationStatus::ERROR) {
                return result;
            }
        }

        // Rate-limit the event polling when no active request
        // the polling is expensive (7 ms) and we don't need to block that often
        // Order is important - check the limiter even if there are no active requests
        should_poll = event_poll_limiter_ms.check(ticks_ms()) || std::ranges::any_of(active_requests, &ActiveRequestEntry::is_valid);
    }

    if (should_poll) {
        for (DeviceState &device_state : devices) {
            result = puppies::aggregate_communication_status({ result, device_state.poll_events_unlocked(client) });
            if (result == puppies::CommunicationStatus::ERROR) {
                return result;
            }
        }
    }

    return result;
}

void Manager::on_request_done(RequestID request_id, Bytes raw_event_data) {
    for (auto &entry : active_requests) {
        if (entry.is_valid() && entry.request_id == request_id) {
            entry.request->complete(raw_event_data);
            entry.request = nullptr;
            return;
        }
    }

    log_error(OpenPrintTag, "stray request %d", request_id.to_underlying());
}

void Manager::DeviceState::on_request_done(RequestID request_id, Bytes raw_event_data) {
    // handle forget_tag completion
    if (auto *f = std::get_if<TagForgetting>(&tag); f && f->request_id == request_id) {
        tag = TagUnused {};
        return;
    }

    // delegate to manager for regular requests
    manager->on_request_done(request_id, raw_event_data);
}

void Manager::DeviceState::on_tag_detected(TagID tag_id, TagUID tag_uid) {
    if (std::holds_alternative<TagUnused>(tag)) {
        tag = TagDetected { tag_id, tag_uid };
    } else {
        // BFW-8253 will ensure we never get more than one tag per antenna
        // Until then, we log and do nothing.
        log_info(OpenPrintTag, "ignoring tag detected event for tag %d", tag_id);
    }
}

void Manager::DeviceState::on_tag_lost(TagID tag_id) {
    if (auto *d = std::get_if<TagDetected>(&tag); d && d->tag_id == tag_id) {
        tag = TagLost { d->tag_id };
    } else {
        // BFW-8253 will ensure we never get more than one tag per antenna
        // Until then, we log and do nothing.
        log_info(OpenPrintTag, "ignoring tag lost event for tag %d", tag_id);
    }
}

puppies::CommunicationStatus Manager::DeviceState::forget_lost_tag(anfc::modbus::Client &client) {
    if (!device) {
        return puppies::CommunicationStatus::SKIPPED;
    }

    auto *lost = std::get_if<TagLost>(&tag);
    if (!lost) {
        return puppies::CommunicationStatus::SKIPPED;
    }

    const auto req_id = manager->make_request_id();
    anfc::modbus::Request request;
    serialize_forget_tag(req_id.to_underlying(), lost->tag_id, request);
    if (!client.write(*device, request)) {
        // will be forgotten next time
        return puppies::CommunicationStatus::ERROR;
    }

    tag = TagForgetting { req_id };
    return puppies::CommunicationStatus::OK;
}

puppies::CommunicationStatus Manager::handle_pending_request(anfc::modbus::Client &client) {
    if (pending_requests.empty()) {
        return puppies::CommunicationStatus::SKIPPED;
    }
    buddy::openprinttag::Request &pending_request = pending_requests.back();

    // The request could have failed during the last serialize() or could have been failed externally
    // Discard the request in that case
    if (pending_request.finished()) {
        pending_requests.remove(pending_request);
        return puppies::CommunicationStatus::SKIPPED;
    }

    const auto active_entry = std::ranges::find_if(active_requests, [](const auto &e) { return !e.is_valid(); });
    if (active_entry == active_requests.end()) {
        // No free slot, try again later
        return puppies::CommunicationStatus::SKIPPED;
    }

    const RequestID request_id = make_request_id();
    anfc::modbus::Request modbus_request = {};

    const auto target_device = pending_request.serialize(ManagerNoLockBadge {}, request_id, modbus_request);

    if (!target_device.has_value()) {
        return puppies::CommunicationStatus::SKIPPED;
    }

    if (!client.write(*target_device, modbus_request)) {
        log_warning(OpenPrintTag, "failed to write request");
        // keep pending request at its position in queue and try later
        // ERROR would result in the whole puppy modbus being reset
        // client.write can return false when the request queue is full on the xbe for example
        return puppies::CommunicationStatus::SKIPPED;
    }

    pending_requests.remove(pending_request);
    *active_entry = ActiveRequestEntry {
        .request = &pending_request,
        .request_id = request_id,
        .sent_at = ticks_ms(),
    };

    return puppies::CommunicationStatus::OK;
}

puppies::CommunicationStatus Manager::DeviceState::poll_events_unlocked(anfc::modbus::Client &client) {
    if (!device) {
        return puppies::CommunicationStatus::SKIPPED;
    }

    std::optional<uint16_t> accept_event_id;

    // modbus_event, reduce scope
    {
        anfc::modbus::Event modbus_event;
        modbus_event.size = 0;

        if (!client.read(*device, modbus_event)) {
            return puppies::CommunicationStatus::ERROR;
        }

        if (modbus_event.size != 0) {
            std::lock_guard lock { manager->mutex };
            accept_event_id = handle_event(modbus_event);
        }
    }

    if (accept_event_id.has_value()) {
        anfc::modbus::AcceptEvent accept_event;
        serialize_accept_event(*accept_event_id, accept_event);
        if (!client.write(*device, accept_event)) {
            return puppies::CommunicationStatus::ERROR;
        }
    }

    // We've definitely done client.read -> we've worked
    return puppies::CommunicationStatus::OK;
}

puppies::CommunicationStatus Manager::DeviceState::step(anfc::modbus::Client &client) {
    if (!device) {
        return puppies::CommunicationStatus::SKIPPED;
    }

    static_assert(!HAS_INDX(), "Revisit all HAS_ANFC_FOR_MULTIPLE_TOOLS block (especially code below) when implementing NFC for 'multi-tool' printers.");
#if (HAS_MMU2() && !HAS_ANFC_FOR_MULTIPLE_TOOLS())
    // Turn NFC antenna off if MMU is active
    // !!This code should be temporary!!
    const bool want_radio = !config_store().mmu2_enabled.get();
#else
    const bool want_radio = true;
#endif

    if (radio_enabled != want_radio) {
        if (!enable_radio_request.has_value()) {
            // Enable radio request has not been issued yed - issue
            enable_radio_request.emplace(*device, want_radio);
            manager->add_request_nolock(*enable_radio_request);

        } else if (!enable_radio_request->finished()) {
            // Wait for the request to finish

        } else if (enable_radio_request->has_error()) {
            // Error - re-issue
            manager->add_request_nolock(*enable_radio_request);
        } else if (enable_radio_request->enable() != want_radio) {
            // Target changed - re-issue
            enable_radio_request->set_enable(want_radio);
            manager->add_request_nolock(*enable_radio_request);
        } else {
            // Done!
            radio_enabled = want_radio;
        }
    }

    return puppies::aggregate_communication_status({
        manager->handle_pending_request(client),
        forget_lost_tag(client),
    });
}

std::optional<uint16_t> Manager::DeviceState::handle_event(const anfc::modbus::Event &modbus_event) {
    if (modbus_event.size == 0) {
        return std::nullopt;
    }

    // Deserialize event from modbus data
    const auto raw_event_data = ::modbus::payload(modbus_event);

    prusa3d_nfc_event_Event_1_0 event;
    size_t event_size = raw_event_data.size();
    if (prusa3d_nfc_event_Event_1_0_deserialize_(&event, reinterpret_cast<const uint8_t *>(raw_event_data.data()), &event_size) != 0) {
        return std::nullopt; // Ignore malformed events
    }

    // dispatch event to correct handler
    auto on_event = [&](const prusa3d_nfc_event_EventData_1_0 &event_data) {
        if (prusa3d_nfc_event_EventData_1_0_is_request_done_(&event_data)) {
            const auto &request_done = event_data.request_done;

            // unpack arguments
            const RequestID request_id = RequestID { request_done.request_id.value };

            // handle the event
            return on_request_done(request_id, raw_event_data);
        }

        if (prusa3d_nfc_event_EventData_1_0_is_tag_detected_(&event_data)) {
            const auto &tag_detected = event_data.tag_detected;

            // unpack arguments
            const TagID tag_id = tag_detected.tag.value;
            TagUID tag_uid = {};
            static_assert(sizeof(tag_uid.data) == sizeof(tag_detected.uid));
            std::memcpy(tag_uid.data.data(), tag_detected.uid, tag_uid.data.size());

            // handle the event
            return on_tag_detected(tag_id, tag_uid);
        }

        if (prusa3d_nfc_event_EventData_1_0_is_tag_lost_(&event_data)) {
            const auto &tag_lost = event_data.tag_lost;

            // unpack arguments
            const TagID tag_id = tag_lost.tag.value;

            // handle the event
            return on_tag_lost(tag_id);
        }

        // If this ever happens, it means either serious memory corruption
        // occured or somebody hijacked the bus. Just crash here, it is not
        // safe to proceed anyway.
        bsod_unreachable();
    };

    on_event(event.data);
    return event.event_id.value;
}

RequestID Manager::make_request_id() {
    // Note: overflow is OK given current request rate
    return RequestID { ++request_id };
}

void Manager::add_request(Badge<Request>, Request &request) {
    std::lock_guard lock { mutex };

    add_request_nolock(request);
}

void Manager::add_request_nolock(Request &request) {
    remove_request_nolock(request);
    pending_requests.push_front(request);

    // Reset request.finished in case this is a reissue
    request.finished_ = false;
    request.error_ = Request::Error::_cnt;
}

void Manager::remove_request(Badge<Request>, Request &request) {
    std::lock_guard lock { mutex };

    remove_request_nolock(request);
}

void Manager::remove_request_nolock(Request &request) {
    pending_requests.remove(request);
    for (auto &entry : active_requests) {
        if (entry.request == &request) {
            entry.request = nullptr;
            break;
        }
    }
}

void Manager::check_timeouts() {
    const auto now = ticks_ms();
    for (auto &entry : active_requests) {
        if (entry.is_valid() && ticks_diff(now, entry.sent_at) > request_timeout_ms) {
            log_warning(OpenPrintTag, "request %d timed out", entry.request_id.to_underlying());
            entry.request->set_finished(std::unexpected(Request::Error::other));
            entry.request = nullptr;
        }
    }
}

std::optional<TagUID> Manager::get_tag_uid_for_tool(VirtualToolIndex tool) const {
    std::shared_lock lock { mutex };

    const auto &device = devices[tool];
    if (auto *d = std::get_if<TagDetected>(&device.tag)) {
        return d->tag_uid;
    }
    return std::nullopt;
}

std::optional<TagUID> Manager::get_tag_uid(ToolTag tool_tag) const {
    const auto uid = get_tag_uid_for_tool(tool_tag.tool());
    if (uid.has_value() && uid->hash() == tool_tag.uid_hash()) {
        return uid;
    } else {
        return std::nullopt;
    }
}

std::optional<Manager::TagDeviceInfo> Manager::get_tag_device_info(ToolTag tool_tag) const {
    std::shared_lock lock { mutex };
    return get_tag_device_info_nolock({}, tool_tag);
}

std::optional<Manager::TagDeviceInfo> Manager::get_tag_device_info_nolock(ManagerNoLockBadge, ToolTag tool_tag) const {
    const DeviceState &device = devices[tool_tag.tool()];
    auto *d = std::get_if<TagDetected>(&device.tag);
    if (d && d->tag_uid.hash() == tool_tag.uid_hash() && device.device) {
        return TagDeviceInfo {
            .device = *device.device,
            .tag_id = d->tag_id,
        };
    } else {
        return std::nullopt;
    }
}

Manager &manager() {
    static Manager instance;
    return instance;
}

} // namespace buddy::openprinttag

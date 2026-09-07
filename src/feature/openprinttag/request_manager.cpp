/// @file
#include <feature/openprinttag/request_manager.hpp>

#include <bsod/bsod.h>
#include <cstring>
#include <feature/openprinttag/detail/requests_base.hpp>
#include <feature/openprinttag/tool_tag.hpp>
#include <modbus/traits.hpp>
#include <mutex>
#include <prusa3d/nfc/command/AcceptEvent_1_0.h>
#include <prusa3d/nfc/command/Request_1_0.h>
#include <prusa3d/nfc/event/Event_1_0.h>
#include <logging/log.hpp>
#include <timing.h>
#include <utils/byte_utils.hpp>

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

ToolTag::UIDHash Manager::TagUID::hash() const {
    uint16_t h = (data[0] << 8 | data[1])
        ^ (data[2] << 8 | data[3])
        ^ (data[4] << 8 | data[5])
        ^ (data[6] << 8 | data[7]);
    return h | 1; // make sure to never return no_tag_hash
}

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

bool Manager::step(anfc::modbus::Client &client) {
    std::lock_guard lock { mutex };

    check_timeouts();

    return std::ranges::all_of(devices, [&client](DeviceState &device_state) {
        return device_state.step(client);
    });
}

void Manager::on_request_done(RequestID request_id, Bytes raw_event_data) {
    for (auto &entry : active_requests) {
        if (entry.request && entry.request_id == request_id) {
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

void Manager::DeviceState::forget_lost_tag(anfc::modbus::Client &client) {
    if (!device) {
        return;
    }
    if (auto *lost = std::get_if<TagLost>(&tag)) {
        const auto req_id = manager->make_request_id();
        anfc::modbus::Request request;
        serialize_forget_tag(req_id.to_underlying(), lost->tag_id, request);
        if (client.write(*device, request)) {
            tag = TagForgetting { req_id };
        } else {
            // will be forgotten next time
        }
    }
}

void Manager::handle_pending_request(anfc::modbus::Client &client) {
    if (pending_requests.empty()) {
        return;
    }
    buddy::openprinttag::Request &pending_request = pending_requests.back();

    // The request could have failed during the last serialize() or could have been failed externally
    // Discard the request in that case
    if (pending_request.finished()) {
        pending_requests.remove(pending_request);
        return;
    }

    const auto active_entry = std::ranges::find_if(active_requests, [](const auto &e) { return e.request == nullptr; });
    if (active_entry == active_requests.end()) {
        // No free slot, try again later
        return;
    }

    const RequestID request_id = make_request_id();
    anfc::modbus::Request modbus_request = {};

    const auto target_device = pending_request.serialize(ManagerNoLockBadge {}, request_id, modbus_request);

    if (!target_device.has_value()) {
        return;
    }

    if (!client.write(*target_device, modbus_request)) {
        log_warning(OpenPrintTag, "failed to write request");
        // keep pending request at its position in queue and try later
        return;
    }

    pending_requests.remove(pending_request);
    *active_entry = ActiveRequestEntry {
        .request = &pending_request,
        .request_id = request_id,
        .sent_at = ticks_ms(),
    };
}

bool Manager::DeviceState::step(anfc::modbus::Client &client) {
    if (!device) {
        return true;
    }

    if (!radio_enabled) {
        if (!enable_radio_request.has_value()) {
            // Enable radio request has not been issued yed - issue
            enable_radio_request.emplace(*device, true);
            manager->add_request_nolock(*enable_radio_request);

        } else if (!enable_radio_request->finished()) {
            // Wait for the request to finish

        } else if (enable_radio_request->has_error()) {
            // Error - reset the request, re-issue
            manager->add_request_nolock(*enable_radio_request);

        } else {
            // Done!
            radio_enabled = true;
        }
    }

    anfc::modbus::Event modbus_event;
    if (!client.read(*device, modbus_event)) {
        return false;
    }

    manager->handle_pending_request(client);
    forget_lost_tag(client);

    return handle_event(client, modbus_event);
}

bool Manager::DeviceState::handle_event(anfc::modbus::Client &client, const anfc::modbus::Event &modbus_event) {
    if (modbus_event.size == 0) {
        return true;
    }

    // Deserialize event from modbus data
    const auto raw_event_data = ::modbus::payload(modbus_event);

    prusa3d_nfc_event_Event_1_0 event;
    size_t event_size = raw_event_data.size();
    if (prusa3d_nfc_event_Event_1_0_deserialize_(&event, reinterpret_cast<const uint8_t *>(raw_event_data.data()), &event_size) != 0) {
        return true; // Ignore malformed events
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
            std::memcpy(tag_uid.data, tag_detected.uid, sizeof(tag_uid.data));

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
    anfc::modbus::AcceptEvent accept_event;
    serialize_accept_event(event.event_id.value, accept_event);
    return client.write(*device, accept_event);
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
        if (entry.request && ticks_diff(now, entry.sent_at) > request_timeout_ms) {
            log_warning(OpenPrintTag, "request %d timed out", entry.request_id.to_underlying());
            entry.request->set_finished(std::unexpected(Request::Error::other));
            entry.request = nullptr;
        }
    }
}

std::optional<Manager::TagUID> Manager::get_tag_uid_for_tool(VirtualToolIndex tool) {
    std::lock_guard lock { mutex };

    const auto &device = devices[tool];
    if (auto *d = std::get_if<TagDetected>(&device.tag)) {
        return d->tag_uid;
    }
    return std::nullopt;
}

std::optional<Manager::TagDeviceInfo> Manager::get_tag_device_info(ToolTag tool_tag) {
    std::lock_guard lock { mutex };
    return get_tag_device_info_nolock({}, tool_tag);
}

std::optional<Manager::TagDeviceInfo> Manager::get_tag_device_info_nolock(ManagerNoLockBadge, ToolTag tool_tag) {
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

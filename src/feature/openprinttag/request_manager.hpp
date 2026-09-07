/// @file
#pragma once

#include <anfc/modbus.hpp>
#include <anfc/types.hpp>
#include <array>
#include <compact_pointer.hpp>
#include <feature/openprinttag/detail/requests_base.hpp>
#include <feature/openprinttag/detail/requests_util.hpp>
#include <feature/openprinttag/tool_tag.hpp>
#include <freertos/mutex.hpp>
#include <openprinttag/opt_reader.hpp>
#include <optional>
#include <tool_index.hpp>
#include <utils/badge.hpp>
#include <utils/storage/single_linked_list.hpp>
#include <utils/storage/strong_index_array.hpp>
#include <utils/uncopyable.hpp>
#include <variant>
#include <utils/byte_utils.hpp>

namespace buddy::openprinttag {

/// This class manages synchronization between ANFC modbus devices, virtual
/// tools of the printer and OpenPrintTag NFC chips.
///
/// Public methods are thread-safe.
class Manager final : public Uncopyable {
public:
    Manager();
    ~Manager();

    /// Step internal state machine.
    /// This may attempt communication on modbus, using provided client.
    /// Returns false on error.
    [[nodiscard]] bool step(anfc::modbus::Client &);

    /// Register Request with the manager.
    /// You must call remove_request() before Request's destruction.
    /// You may call this multiple times with the same Request at any time.
    void add_request(Badge<Request>, Request &);

    /// Unregister Request from the manager.
    /// You must call this before Request's destruction.
    /// You may call this even if Request was not registered with the manager.
    void remove_request(Badge<Request>, Request &);

    struct TagUID {
        std::uint8_t data[8];

        ToolTag::UIDHash hash() const;
    };

    /// Return TagUID detected on given tool, if any.
    std::optional<TagUID> get_tag_uid_for_tool(VirtualToolIndex);

    struct TagDeviceInfo {
        anfc::Device device;
        TagID tag_id;
    };

    /// @returns Device-specific TagID for the given tag, if the device is currently tracking the tag
    std::optional<TagDeviceInfo> get_tag_device_info(ToolTag tool_tag);

    std::optional<TagDeviceInfo> get_tag_device_info_nolock(ManagerNoLockBadge, ToolTag tool_tag);

private:
    /// Mutex guarding all the member variables of this Manager.
    /// Every public method of the Manager is required to obtain the lock or pass the ManagerNoLockBadge.
    /// No other method is allowed to obtain the lock.
    /// No method of Manager is allowed to call public method.
    freertos::Mutex mutex;

    /// Tag is not being tracked
    struct TagUnused {};

    /// Tag is detected and available for requests
    struct TagDetected {
        TagID tag_id; ///< tag id as reported by the reader
        TagUID tag_uid; ///< tag uid as reported by the chip
    };

    /// Tag was removed, waiting to send forget request
    struct TagLost {
        TagID tag_id;
    };

    /// forget_tag request sent, waiting for ack
    struct TagForgetting {
        RequestID request_id;
    };

    using TagState = std::variant<TagUnused, TagDetected, TagLost, TagForgetting>;

    struct DeviceState {
        Manager *manager;
        TagState tag;
        std::optional<anfc::Device> device;

        /// nullopt - enable radio has not been issued yet
        /// finished without error - radio is enabled
        std::optional<EnableRadioRequest> enable_radio_request;

        bool radio_enabled : 1 = false;

        [[nodiscard]] bool step(anfc::modbus::Client &);

        [[nodiscard]] bool handle_event(anfc::modbus::Client &, const anfc::modbus::Event &);

        void on_request_done(RequestID, Bytes);
        void on_tag_detected(TagID, TagUID);
        void on_tag_lost(TagID);

        void forget_lost_tag(anfc::modbus::Client &);
    };
    StrongIndexArray<DeviceState, VirtualToolIndex::count, VirtualToolIndex, VirtualToolIndex::to_raw_static> devices;

    SingleLinkedList<&Request::next_request> pending_requests;

    /// Global request id.
    /// We could use different ID for each device, but having a global one
    /// helps with debugging a lot.
    uint16_t request_id = 0;

    /// This structure is used to keep additional info for requests which
    /// transitioned from pending state. We are keeping the info separate
    /// from the Request, because it is only needed while the request is
    /// in flight and we want to keep the size of the Request small.
    struct ActiveRequestEntry {
        /// Request which is associated with this entry.
        /// nullptr indicates unused entry.
        CompactRAMPointer<Request> request = nullptr;

        /// Request ID used for matching response to request.
        /// This must be obtained by calling make_request_id()
        RequestID request_id;

        /// Timestamp at which the request was sent.
        /// This is in milliseconds since boot.
        /// This is used to handle timeouts.
        uint32_t sent_at;
    };

    /// We limit number of active requests.
    /// This trades increased request latency for decreased memory footprint.
    static constexpr size_t max_active_requests = 4;

    /// The table holding active requests.
    std::array<ActiveRequestEntry, max_active_requests> active_requests;

    void on_request_done(RequestID, Bytes);
    void handle_pending_request(anfc::modbus::Client &);
    void check_timeouts();
    RequestID make_request_id();
    void add_request_nolock(Request &);
    void remove_request_nolock(Request &);
};

Manager &manager();

} // namespace buddy::openprinttag

#pragma once

#include <atomic>
#include <functional>
#include <type_traits>
#include <utility>

#include <canard.h>
#include <timing.h>
#include <logging/log.hpp>
#include <cyphal_suber_call.hpp>
#include <cyphal_port_list.hpp>

#include <uavcan/node/Heartbeat_1_0.h>
#include <option/sysdebug.h>

#include "honeybee_shared_fault.hpp"
#include <bsod/bsod.h>

LOG_COMPONENT_REF(Watcher);

namespace watcher {

template <typename E>
concept EnumType = std::is_enum_v<E> && requires(E e) {
    E::_count;
};

/**
 * @brief Watcher for presence of something.
 * @tparam Watched enum type of watched things, needs to contain _count
 * @tparam timeout_us timeout [microseconds]
 */
template <EnumType Watched, int32_t timeout_us>
class Proto {
protected:
    static_assert(timeout_us > 0, "Timeout needs to be positive");

    static constexpr size_t COUNT = std::to_underlying(Watched::_count);

    std::atomic<uint32_t> last_presence[COUNT] = {}; ///< Lower 32 bits of timestamp when we last saw presence [us]

    /// True if last_presence is valid and being checked, set on first call to present(), reset on timeout
    std::atomic<bool> watching[COUNT] = {};

    /// True if we have already reported missing, to avoid spamming logs, may seem duplicate but it is used differently in children
    std::atomic<bool> faulted[COUNT] = {};

public:
    Proto() = default;

    /**
     * @brief Check if something is missing.
     * @param now current time
     */
    void check(int64_t now) {
        uint32_t now32 = static_cast<uint32_t>(now);

        for (size_t i = 0; i < COUNT; ++i) {
            if (watching[i].load() // Only if we have seen this thing
                && ticks_diff(now32, last_presence[i].load()) > timeout_us) {
                watching[i].store(false);
                faulted[i].store(true);
                proto_callback(Watched(i));
            }
        }
    }

    /**
     * @brief Mark something as present at this moment.
     * @param now current time
     * @param what what to mark as present
     */
    void present(int64_t now, Watched what) {
        debug_assert(std::to_underlying(what) < COUNT);
        last_presence[std::to_underlying(what)].store(static_cast<uint32_t>(now));
        faulted[std::to_underlying(what)].store(false);
        watching[std::to_underlying(what)].store(true);
    }

    /// @return True if we are currently watching this thing.
    inline bool is_watching(Watched what) const {
        debug_assert(std::to_underlying(what) < COUNT);
        return watching[std::to_underlying(what)].load();
    }

    /// @return True if this thing is currently considered missing.
    inline bool is_faulted(Watched what) const {
        debug_assert(std::to_underlying(what) < COUNT);
        return faulted[std::to_underlying(what)].load();
    }

    /**
     * @brief Callback when something is missing.
     * @param what what is missing
     */
    virtual void proto_callback(Watched what) = 0;
};

/**
 * @brief Watcher for presence of some data.
 * @tparam Watched enum type of watched things, needs to contain _count
 * @tparam timeout_us timeout [microseconds]
 * @tparam watcher_n watcher number for log in case there are more data watchers
 */
template <EnumType Watched, uint32_t timeout_us, uint32_t watcher_n = 0>
class Data : public Proto<Watched, timeout_us> {
public:
    /**
     * @brief Callback when data is missing and need to be cleared.
     * @param what what is missing
     */
    using ClearDataCallback = std::function<void(Watched)>;

    /**
     * @brief Watch for presence of some data.
     * @param callback_ when data is missing and need to be cleared
     */
    Data(const ClearDataCallback callback_)
        : callback(callback_) {
        debug_assert(callback);
    }

private:
    ClearDataCallback callback; ///< When data is missing and need to be cleared

    /**
     * @brief Callback when data is missing.
     * @param what what is missing
     */
    void proto_callback(Watched what) override {
        callback(what);

#if !SYSDEBUG()
        log_error(Watcher, "Data_%lu timeout for watched=%u", watcher_n, std::to_underlying(what));
        puppy::fault::trigger_fault(puppy::fault::SharedFault::data_timeout);
#else /* SYSDEBUG() */
        log_error(Watcher, "Data_%lu timeout for watched=%u, would fault itself if not sysdebug", watcher_n, std::to_underlying(what));
#endif /* SYSDEBUG() */
    }
};

/**
 * @brief Watcher for presence of heartbeats.
 * @tparam Watched enum type of watched nodes, needs to contain _count
 */
template <EnumType Watched>
class Heartbeat : public Proto<Watched, uavcan_node_Heartbeat_1_0_OFFLINE_TIMEOUT * 1'000'000> {
    using inherited = Proto<Watched, uavcan_node_Heartbeat_1_0_OFFLINE_TIMEOUT * 1'000'000>;

    /// Heartbeat subscription to watch if other nodes are alive
    can::cyphal::SuberCall<uavcan_node_Heartbeat_1_0, uavcan_node_Heartbeat_1_0_EXTENT_BYTES_> watch_heartbeat_suber;

    std::atomic<CanardNodeID> watch_node_id[inherited::COUNT]; ///< Watch these nodes

    /**
     * @brief Callback when data is missing.
     * @param what what is missing
     */
    void proto_callback([[maybe_unused]] Watched what) override {
        debug_assert(std::to_underlying(what) < inherited::COUNT);
#if !SYSDEBUG()
        log_error(Watcher, "Heartbeat timeout for watched=%u, node_id=%hhu",
            std::to_underlying(what),
            watch_node_id[std::to_underlying(what)].load());
        puppy::fault::trigger_fault(puppy::fault::SharedFault::heartbeat_missing);
#else /* SYSDEBUG() */
        log_error(Watcher, "Heartbeat timeout for watched=%u, node_id=%hhu, would fault itself if not sysdebug",
            std::to_underlying(what),
            watch_node_id[std::to_underlying(what)].load());
#endif /* SYSDEBUG() */
    }

public:
    /**
     * @brief Watcher for presence of heartbeats.
     */
    Heartbeat()
        : watch_heartbeat_suber(uavcan_node_Heartbeat_1_0_deserialize_, uavcan_node_Heartbeat_1_0_FIXED_PORT_ID_,
            [this]([[maybe_unused]] const uavcan_node_Heartbeat_1_0 &data, const can::cyphal::ProtoSuber::Meta &meta) {
                for (size_t i = 0; i < inherited::COUNT; ++i) {
                    if (watch_node_id[i] == meta.remote_node_id) { // We watch this node
                        inherited::present(meta.timestamp, Watched(i));
                    }
                }
            }) {
        for (size_t i = 0; i < inherited::COUNT; ++i) {
            watch_node_id[i].store(CANARD_NODE_ID_UNSET);
        }
    };

    /**
     * @brief Add itself to task and to portlist.
     * @note Does not add port name and type registers (not needed for uavcan types).
     * @param port_list node's object publishing used ports
     */
    void init(can::cyphal::PortList &port_list) {
        watch_heartbeat_suber.add_to_task();
        port_list.add(watch_heartbeat_suber);
    }

    /**
     * @brief Register node ID to watch.
     * @param what what to watch
     * @param node_id node ID to watch
     * @param watch_change true to fault if node_id changes, false to allow change only if not watching
     */
    void register_node_id(Watched what, CanardNodeID node_id, int64_t now, bool watch_change = true) {
        debug_assert(std::to_underlying(what) < inherited::COUNT);
        if (inherited::watching[std::to_underlying(what)].load() == false) {
            if (inherited::faulted[std::to_underlying(what)].load() == false) {
                inherited::present(now, what); // mark it as present - to establish baseline from which to count timeout
            }
            watch_node_id[std::to_underlying(what)].store(node_id);
        } else if (watch_change) {
            if (auto previous = watch_node_id[std::to_underlying(what)].exchange(node_id); previous != node_id) {
#if !SYSDEBUG()
                log_error(Watcher, "Heartbeat for watched=%u changed node_id from=%hhu to=%hhu",
                    std::to_underlying(what), previous, node_id);
                // The original node got replaced, so trigger heartbeat_missing fault
                puppy::fault::trigger_fault(puppy::fault::SharedFault::heartbeat_missing);
#else /* SYSDEBUG() */
                log_error(Watcher, "Heartbeat for watched=%u changed node_id from=%hhu to=%hhu, would fault itself if not sysdebug",
                    std::to_underlying(what), previous, node_id);
#endif /* SYSDEBUG() */
            }
        } else {
            if (auto previous = watch_node_id[std::to_underlying(what)].load(); previous != node_id) {
                log_warning(Watcher, "Heartbeat for watched=%u wanted to change node_id from=%hhu to=%hhu, ignored",
                    std::to_underlying(what), previous, node_id);
            }
        }
    }

    /**
     * @brief Get node ID of watched node.
     * @param what what is watched
     * @return node ID of watched node
     */
    [[nodiscard]] CanardNodeID get_node_id(Watched what) const {
        debug_assert(std::to_underlying(what) < inherited::COUNT);
        return watch_node_id[std::to_underlying(what)].load();
    }
};

} // namespace watcher

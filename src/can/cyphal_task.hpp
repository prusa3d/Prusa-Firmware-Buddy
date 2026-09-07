#pragma once

#include "timing.h"
#include <atomic>
#include <functional>
#include <cstdint>

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <utils/uncopyable.hpp>

#include <device/multi_watchdog.hpp>

#include <canard.h>
#include "can_driver.hpp"
#include "cyphal_proto_sender.hpp"
#include "cyphal_proto_suber.hpp"
#include <utils/atomic_circular_queue.hpp>

#include <uavcan/time/Synchronization_1_0.h>
#include <bsod/bsod.h>

namespace can::cyphal {

/**
 * @brief Element stored in Rx queue.
 * @note We don't want to copy this thing. It would be slow and we would have to modify frame.payload.
 */
struct TaskRxBufferElement : public Uncopyable {
    CanardFrame frame = {}; ///< Incoming CAN frame
    std::array<uint8_t, CANARD_MTU_CAN_FD> payload = {}; ///< Storage for payload data (needs to be linked into frame)
    CanardMicrosecond timestamp_us = 0; ///< Timestamp when the transfer was received

    TaskRxBufferElement() = default;
};

class Task {
    device::MultiWatchdog watchdog; ///< Add one instance of watchdog

    CanardInstance canard_instance; ///< Libcanard instance
    CanardTxQueue canard_tx_queue; ///< Libcanard TX queue instance
    Driver &driver; ///< CAN hardware driver instance
    bool automatic_retransmission_enable = false; ///< Marks current state of automatic retransmission

    StaticSemaphore_t senders_mutex_buffer;
    SemaphoreHandle_t senders_mutex = xSemaphoreCreateMutexStatic(&senders_mutex_buffer); ///< Mutex to handle non thread-safe access to senders
    ProtoSenderPeriodic *senders = nullptr; ///< List of messages to be sent
    ProtoSenderPeriodic *list_position = nullptr; ///< Ended here in the list on last loop()

    StaticSemaphore_t subers_mutex_buffer;
    SemaphoreHandle_t subers_mutex = xSemaphoreCreateRecursiveMutexStatic(&subers_mutex_buffer); ///< Mutex to handle non thread-safe access to services
    ///@note Mutex subers_mutex needs to be recursive because subscription can remove itself from its own callback, also lock access to received data.
    ProtoSender *pnp_sender = nullptr; ///< PnP sender object, the only one allowed while anonymous
    ProtoSuber *pnp_suber = nullptr; ///< PnP suber object, the only one allowed while anonymous
    uint8_t rx_buffer[ProtoSuber::MAX_DATA_SIZE] = {}; ///< Buffer used for deserialization

    TaskHandle_t loop_task = nullptr; ///< Loop task to wake up on CAN event
    static constexpr UBaseType_t notify_index = 1; ///< Wake up with an index (should not be 0)

    StaticSemaphore_t tx_buffer_semaphore_buffer;
    SemaphoreHandle_t tx_buffer_semaphore = xSemaphoreCreateBinaryStatic(&tx_buffer_semaphore_buffer); ///< Semaphore for tx_buffer, taken before written and given after processed
    std::atomic<bool> tx_buffer_used = false; ///< True if tx_buffer has valid data, false if not
    std::atomic<bool> tx_buffer_reserved = false; ///< If buffer is reserved for loop_task callback
    uint8_t tx_buffer[ProtoSender::MAX_SERIALIZED_SIZE_BYTES] = {}; ///< Buffer used for Tx serialization
    CanardTransferMetadata tx_meta; ///< Metadata for payload in tx_buffer
    CanardMicrosecond tx_timeout = 0; ///< Timestamp when the transmitted messages are discarded if not sent already
    size_t tx_buffer_size = 0; ///< Size of data stored in tx_buffer

    AtomicCircularQueueSizeless<TaskRxBufferElement, size_t> &rx_queue; ///< Buffer for received Cyphal transfers
    std::atomic<bool> rx_queue_used = false; ///< True if rx_queue is being used by thread and is blocked for interrupt

    class RAIIElement : public Uncopyable {
        stdext::inplace_function<void(void)> drop_callback;

    public:
        const TaskRxBufferElement &e;

        /**
         * @brief Take one element from rx_queue without copying and drop it from the queue when finished.
         * @param element_ peek the element `rx_queue.peek()`
         * @param drop_callback_ drop the element `rx_queue.drop()` and do other stuff, called in destructor
         */
        RAIIElement(const TaskRxBufferElement &element_, stdext::inplace_function<void(void)> drop_callback_)
            : drop_callback(drop_callback_)
            , e(element_) {}

        ~RAIIElement() { drop_callback(); }
    };

public:
    struct TimeSync {
        static constexpr CanardMicrosecond period_max_us = uavcan_time_Synchronization_1_0_MAX_PUBLICATION_PERIOD * 1000000; ///< Maximum period to send timesync messages [us]
        static constexpr CanardMicrosecond period_min_us = 100'000; ///< Minimum period to send timesync messages [us]
        CanardTxQueue canard_tx_queue = {}; ///< Libcanard TX queue instance to send time sync messages
        CanardMicrosecond period_us = 0; ///< 0 to disable, between period_min_us and period_max_us to send timesync messages
        CanardMicrosecond last_attempt = 0; ///< Timestamp when last timesync message was created
        CanardMicrosecond last_sent = 0; ///< Timestamp when last timesync message was really sent
        CanardTransferID transfer_id = 0; ///< Next transfer ID to use
        bool waiting_for_timestamp = false; ///< True when sent message and waiting for valid timestamp
    };

    using Allocator = void *(CanardInstance *const instance, const size_t bytes);
    static void *default_allocator(CanardInstance *const instance, const size_t bytes);

    using Deallocator = void(CanardInstance *const instance, void *const pointer);
    static void default_deallocator(CanardInstance *const instance, void *const pointer);

private:
    TimeSync timesync; ///< Variables to send timesync messages

    enum class Notify : uint32_t {
        Tx = 1 << 0, ///< Tx done, take data from Cyphal Tx queue
        Rx = 1 << 1, ///< Rx frame available, process it
        Sender = 1 << 2, ///< Data were put into tx_buffer, transfer them to Cyphal Tx queue
        RxLost = 1 << 3, ///< Rx frame was lost, log that
    };

    /// @brief Notify the task of events, both from isr and from tasks.
    void notify(Notify what);

    /// @brief Notify with Driver's notification.
    void notify(Driver::Notification notification) {
        switch (notification) {
        case Driver::Notification::TxDone:
            notify(Notify::Tx);
            break;
        case Driver::Notification::RxHighPrio:
        case Driver::Notification::RxDone:
            if (rx_queue.size() > 1) { // Only if we have a queue
                rx_to_queue();
            }
            notify(Notify::Rx);
            break;
        case Driver::Notification::RxLost:
            ++lost_rx_frames_isr;
            notify(Notify::RxLost);
            if (error_callback) {
                error_callback(Driver::Notification::RxLost);
            }
            break;
        case Driver::Notification::ErrorBusOff:
        case Driver::Notification::ErrorPassive:
        case Driver::Notification::ErrorWarning:
            if (error_callback) {
                error_callback(notification);
            }
            break;
        default:
            break;
        }
    }

    /// Notify the application about driver errors. Called from ISR or task.
    Driver::NotifyCallback error_callback = nullptr;

    std::atomic<uint32_t> lost_rx_frames_isr = 0; ///< Number of lost Rx frames (copy to prevent race)
    uint32_t lost_rx_frames = 0; ///< Number of lost Rx frames
    uint32_t lost_rx_frames_last = 0; ///< Number of lost Rx frames on last log
    int64_t lost_rx_frames_time = 0; ///< Time of last lost Rx frames
    static constexpr int64_t lost_rx_frames_delay = 100'000; ///< Delay before logging a lost Rx frame [us]
    static constexpr uint32_t lost_rx_frames_max = 100; ///< Maximum number of lost Rx frames before logging

    /// @brief Set automatic retransmission depending on anonymous state.
    void apply_automatic_retransmission();

    /// @brief Create timesync messages.
    void timesync_loop();

    /// @brief Check list of periodic messages, requests and responses and serialize them.
    void serialization_loop();

    /// @brief Take serialized data and put them to the Cyphal Tx queue.
    void sender_loop();

    /// @brief Transmit CAN frames from the Cyphal Tx queue.
    void tx_loop();

    /// Receive one CAN frame from HAL and put it to rx_queue.
    void rx_to_queue();

    /// @brief Receive CAN frames from queue or HAL and put them to Cyphal.
    void rx_loop();

    /**
     * @brief RTOS CAN task.
     * Sends periodic Cyphal messages, handles incoming CAN frames.
     * Called from static task() function.
     */
    [[noreturn]] void loop();

public:
    /**
     * @brief Default size of the TX queue.
     *     Cyphal demo suggest 100 for classic CAN.
     *     With 7 bytes per packet, that is a maximum message of 700 bytes.
     *     With 63 bytes per packet, we could lower this number to 12.
     *     One Tx item takes between 49 and 112 bytes, depending on the size of the payload.
     */
    static constexpr uint32_t default_tx_queue_size = 16;
    static_assert(63.f * default_tx_queue_size > ProtoSender::MAX_SERIALIZED_SIZE_BYTES * 1.5f,
        "Default TX queue size is too small for 1.5 of the largest message.");

    /// Type of rx queue buffer
    template <size_t N>
    using RxQueue = AtomicCircularQueue<TaskRxBufferElement, size_t, N>;

    /**
     * @brief Create a Cyphal instance.
     * @note Afterwards, create one RTOS task with task(this) function.
     * @param driver_ CAN hardware driver
     * @param rx_queue buffer for received Cyphal transfers, use RxQueue<N> or RxQueue<1> for no buffer
     * @param tx_queue_size size of the TX queue, see default_tx_queue_size
     * @param allocator custom allocator for libcanard
     * @param deallocator custom deallocator for libcanard
     */
    Task(Driver &driver_, AtomicCircularQueueSizeless<TaskRxBufferElement, size_t> &rx_queue,
        uint32_t tx_queue_size = default_tx_queue_size, Allocator allocator = default_allocator, Deallocator deallocator = default_deallocator);

    /**
     * @brief Lock mutex as long as this lives.
     */
    class [[nodiscard]] RAIILock {
        SemaphoreHandle_t &mutex; ///< Mutex to lock
        bool locked; ///< True if mutex is locked, false if timeout

    public:
        /**
         * @brief Lock mutex as long as this lives.
         * @param mutex_ mutex to lock
         * @param timeout timeout to wait for mutex lock
         */
        RAIILock(SemaphoreHandle_t &mutex_, TickType_t timeout = portMAX_DELAY)
            : mutex(mutex_)
            , locked(xSemaphoreTake(mutex, timeout) == pdTRUE) {}

        /// @brief Don't copy or move
        RAIILock &operator=(const RAIILock &) = delete;
        RAIILock(RAIILock &&other) = delete;
        RAIILock &operator=(RAIILock &&other) = delete;
        RAIILock(const RAIILock &other) = delete;

        /// @return true if mutex is locked, false if timeout
        bool is_locked() const {
            return locked;
        }

        /// @brief Unlock.
        ~RAIILock() {
            if (locked) {
                xSemaphoreGive(mutex);
            }
        }
    };

    /**
     * @brief Lock mutex as long as this lives.
     */
    class [[nodiscard]] RAIIRecursiveLock {
        SemaphoreHandle_t &mutex; ///< Mutex to lock
        bool locked; ///< True if mutex is locked, false if timeout

    public:
        /**
         * @brief Lock mutex as long as this lives.
         * @param mutex_ mutex to lock
         * @param timeout timeout to wait for mutex lock
         */
        RAIIRecursiveLock(SemaphoreHandle_t &mutex_, TickType_t timeout = portMAX_DELAY)
            : mutex(mutex_)
            , locked(xSemaphoreTakeRecursive(mutex, timeout) == pdTRUE) {}

        /// @brief Don't copy.
        RAIIRecursiveLock &operator=(const RAIIRecursiveLock &) = delete;
        RAIIRecursiveLock(RAIIRecursiveLock &&other) = delete;
        RAIIRecursiveLock &operator=(RAIIRecursiveLock &&other) = delete;
        RAIIRecursiveLock(const RAIIRecursiveLock &other) = delete;

        /// @return true if mutex is locked, false if timeout
        bool is_locked() const {
            return locked;
        }

        /// @brief Unlock.
        ~RAIIRecursiveLock() {
            if (locked) {
                xSemaphoreGiveRecursive(mutex);
            }
        }
    };

    /**
     * @brief Parse priority from CAN extended ID.
     * @param can_id CAN extended ID
     * @return priority
     */
    static CanardPriority priority_from_can_id(uint32_t can_id) {
        static constexpr size_t OFFSET_PRIORITY = 26U;
        return static_cast<CanardPriority>((can_id >> OFFSET_PRIORITY) & CANARD_PRIORITY_MAX);
    }

    /**
     * @brief Check if CAN extended ID is service or message.
     * @param can_id CAN extended ID
     * @return true if service, false if message
     */
    static bool service_from_can_id(uint32_t can_id) {
        static constexpr size_t FLAG_SERVICE_NOT_MESSAGE = (UINT32_C(1) << 25U);
        return can_id & FLAG_SERVICE_NOT_MESSAGE;
    }

    /**
     * @brief Parse port ID, either message subject ID or service ID from CAN extended ID.
     * @note This needs to be used in tandem with service_from_can_id(), the same port can be used for both.
     * @param can_id CAN extended ID
     * @return port ID
     */
    static CanardPortID port_from_can_id(uint32_t can_id) {
        static constexpr size_t OFFSET_SUBJECT_ID = 8U;
        static constexpr size_t OFFSET_SERVICE_ID = 14U;
        if (service_from_can_id(can_id)) {
            return (can_id >> OFFSET_SERVICE_ID) & CANARD_SERVICE_ID_MAX;
        } else {
            return (can_id >> OFFSET_SUBJECT_ID) & CANARD_SUBJECT_ID_MAX;
        }
    }

    /**
     * @brief Configure this node ID.
     * Use this to set static node-ID after start or use PnP.
     * Until the ID is set, only pnp_sender and pnp_suber can use CAN.
     * The Plug 'n' Play (PnP) algorithm will use this to set ID obtained from allocator.
     * @param node_id node ID of this node
     */
    void set_node_id(CanardNodeID node_id) {
        canard_instance.node_id = node_id;
    }

    /**
     * @return node ID of this node
     */
    [[nodiscard]] CanardNodeID get_node_id() const {
        return canard_instance.node_id;
    }

    /**
     * @return true if we don't have valid node ID yet
     */
    [[nodiscard]] bool is_anonymous() const {
        return canard_instance.node_id > CANARD_NODE_ID_MAX;
    }

    /**
     * @brief Whitelist PnP objects to work in anonymous mode.
     * @param pnp_sender_ PnP sender object, it is the only allowed in anonymous mode, nullptr if not used
     * @param pnp_suber_ PnP suber object, it is the only allowed in anonymous mode, nullptr if not used
     */
    void whitelist_pnp(ProtoSender *pnp_sender_ = nullptr, ProtoSuber *pnp_suber_ = nullptr) {
        pnp_sender = pnp_sender_;
        pnp_suber = pnp_suber_;
    }

    /**
     * @brief Enable sending timesync messages.
     * @param period 0 to disable, sending period otherwise [us]
     *      Needs to be between timesync.period_max_us and timesync.period_min_us.
     */
    void set_timesync_tx(CanardMicrosecond period_us) {
        if (period_us == 0) {
            timesync.period_us = 0;
        } else {
            debug_assert(period_us >= TimeSync::period_min_us);
            debug_assert(period_us <= TimeSync::period_max_us);
            timesync.period_us = period_us;
        }
    }

    /**
     * @brief RTOS CAN task.
     * Sends periodic Cyphal messages, handles incoming CAN frames.
     * @note This function never returns.
     * @param argument pointer to this object
     */
    static void task(void *argument);

    /**
     * @brief Add object to the list of stuff to be sent.
     * @note No need to use these, use ProtoSender::add_to_task() instead.
     * @param sender sending object to be added
     */
    void add_sender(ProtoSenderPeriodic &sender);

    /**
     * @brief Remove object from the list of stuff to be sent.
     * @note No need to use these, use ProtoSender::remove_from_task() instead.
     * @param sender sending object to be removed
     */
    void remove_sender(ProtoSenderPeriodic &sender);

    /**
     * @brief Lock mutex to handle safe access to senders.
     * @note To be used only by can::cyphal::SenderData.
     * @param timeout timeout to wait for mutex lock
     * @warning if timeout is used, you must check raii.is_locked() before accessing the data
     * @return lock senders_mutex for as long as this object lives
     */
    [[nodiscard]] RAIILock lock_senders_mutex(TickType_t timeout = portMAX_DELAY) {
        return RAIILock(senders_mutex, timeout);
    }

    /**
     * @brief Add subscription to receive messages, requests or responses.
     * @note No need to use these, use ProtoSuber::add_to_task() instead.
     * @param suber subscription prototype to be added
     */
    void add_suber(ProtoSuber &suber);

    /**
     * @brief Remove subscription and no longer receive messages, requests or responses.
     * @note No need to use these, use ProtoSuber::remove_from_task() instead.
     * @param suber subscription prototype to be removed
     */
    void remove_suber(ProtoSuber &suber);

    /**
     * @brief Lock mutex to handle safe access to subers.
     * @note To be used only by can::cyphal::SuberData.
     * @param timeout timeout to wait for mutex lock
     * @warning if timeout is used, you must check raii.is_locked() before accessing the data
     * @return lock subers_mutex for as long as this object lives
     */
    [[nodiscard]] RAIIRecursiveLock lock_subers_mutex(TickType_t timeout = portMAX_DELAY) {
        return RAIIRecursiveLock(subers_mutex, timeout);
    }

    /**
     * @brief Send data directly to the Cyphal Tx queue.
     * @param timestamp current time to save on get_timestamp_us() call
     * @param sender sending object to immediately send
     * @param timeout timeout to wait for mutex lock
     * @return true if put to tx_buffer, false if failed
     * @note This can fail by timeout or when in anonymous mode or if buffer cannot be taken (trying to send 2 things from cyphal callback).
     */
    [[nodiscard]] bool directly_send(CanardMicrosecond timestamp, ProtoSender &sender, TickType_t timeout = portMAX_DELAY);

    /**
     * @brief Send data directly to the Cyphal Tx queue.
     * @param sender sending object to immediately send
     * @param timeout timeout to wait for mutex lock
     * @return true if put to tx_buffer, false if failed
     * @note This can fail by timeout or when in anonymous mode or if buffer cannot be taken (trying to send 2 things from cyphal callback).
     */
    [[nodiscard]] bool directly_send(ProtoSender &sender, TickType_t timeout = portMAX_DELAY) {
        return directly_send(static_cast<CanardMicrosecond>(get_timestamp_us()), sender, timeout);
    }

    /**
     * @brief Know if we are in the Cyphal task callback.
     * @note You can directly send only once per callback.
     * @warning Also true when inside cyphal task in this module.
     * @return true if in the callback
     */
    [[nodiscard]] bool is_in_callback() const {
        return xTaskGetCurrentTaskHandle() == loop_task;
    }

    /**
     * @brief Get number of available filters.
     * @return number of indexes that can be used in set_filter()
     */
    uint32_t filter_count() const { return driver.filter_count(); }

    /**
     * @brief Set filter for incoming frames.
     * @param index filter index, has to be less than filter_count()
     * @param filter mask and id of the filter
     * @param timestamp true if the frame should be timestamped by hardware
     *        @note Has much higher precision, but receive needs to be called before another message arrives.
     *        Otherwise it will not timestamp the following message.
     * @param high_prio true if the frame should throw high priority interrupt
     *        @note Interrupt is enabled, but the handler is not provided by the driver (see HAL_FDCAN_HighPriorityMessageCallback() for STM32 FDCAN).
     * @note Throw bsod on hardware error.
     */
    void set_filter(uint32_t index, const CanardFilter &filter, bool timestamp, bool high_prio) { driver.set_filter(index, filter, timestamp, high_prio); }

    /**
     * @brief Get sum of error increments in both Rx and Tx error counters since start.
     * @return error log counter
     */
    uint32_t get_error_log() { return driver.get_error_log(); }

    /**
     * @brief Set callback for driver and Cyphal error notifications.
     * Intended to be set from the app.
     * @param callback callback to be called from ISR or task
     */
    inline void set_error_callback(Driver::NotifyCallback callback) {
        error_callback = callback;
    }
};

// Cyphal task singleton
extern Task cyphal_task;

} // namespace can::cyphal

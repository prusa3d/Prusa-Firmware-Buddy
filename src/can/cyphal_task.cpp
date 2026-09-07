
#include "cyphal_task.hpp"
#include <uavcan/time/Synchronization_1_0.h>

#include <utility>

#include <bsod.h>
#include <timing.h>
#include <logging/log.hpp>

LOG_COMPONENT_DEF(can, logging::Severity::debug);

namespace can::cyphal {

/**
 * @brief Malloc for canard.
 * @todo The execution time should be constant (malloc maybe isn't). Otherwise it will lag the bus.
 */
void *Task::default_allocator([[maybe_unused]] CanardInstance *const ins, const size_t amount) {
    return malloc(amount);
}

/**
 * @brief Free for canard.
 * @todo The execution time should be constant (free maybe isn't). Otherwise it will lag the bus.
 */
void Task::default_deallocator([[maybe_unused]] CanardInstance *const ins, void *const pointer) {
    free(pointer);
}

Task::Task(Driver &driver_, AtomicCircularQueueSizeless<TaskRxBufferElement, size_t> &rx_queue,
    uint32_t tx_queue_size, Allocator allocator, Deallocator deallocator)
    : watchdog("cyphal_task")
    , driver(driver_)
    , rx_queue(rx_queue) {
    debug_assert(rx_queue.size() > 0); // We need at least one buffer

    // Check all semaphores are valid
    debug_assert(senders_mutex != nullptr);
    debug_assert(tx_buffer_semaphore != nullptr);
    debug_assert(subers_mutex != nullptr);

    // Buffer is not used yet, give
    xSemaphoreGive(tx_buffer_semaphore);

    // Init Cyphal
    canard_instance = canardInit(allocator, deallocator);
    canard_instance.node_id = CANARD_NODE_ID_UNSET;
    debug_assert(63 * tx_queue_size > ProtoSender::MAX_SERIALIZED_SIZE_BYTES); // Queue size must be enough to hold the biggest message
    canard_tx_queue = canardTxInit(tx_queue_size, CANARD_MTU_CAN_FD);
    timesync.canard_tx_queue = canardTxInit(1, CANARD_MTU_CAN_FD);
}

void Task::apply_automatic_retransmission() {
    if (!is_anonymous() != automatic_retransmission_enable) {
        automatic_retransmission_enable = !is_anonymous();
        driver.set_automatic_retransmission(automatic_retransmission_enable);
    }
}

void Task::task(void *argument) {
    debug_assert(argument != nullptr);
    reinterpret_cast<Task *>(argument)->loop();
}

void Task::loop() {
    // Set callback for driver notifications
    loop_task = xTaskGetCurrentTaskHandle();
    driver.set_notify_callback([this](Driver::Notification notification) { notify(notification); });

    // Start CAN driver
    automatic_retransmission_enable = !is_anonymous();
    driver.start(automatic_retransmission_enable);

    while (true) {
        int64_t loop_start = get_timestamp_us();

        // Let watchdog know this loop is alive
        watchdog.kick();

        // Create timesync messages
        timesync_loop();

        // Switch anonymous mode
        apply_automatic_retransmission();

        // Handle periodic messages
        serialization_loop();

        // Log lost frames
        if (((lost_rx_frames != lost_rx_frames_last) && (loop_start - lost_rx_frames_time > lost_rx_frames_delay))
            || (lost_rx_frames - lost_rx_frames_last > lost_rx_frames_max)) {
            log_warning(can, "Lost %lu Rx frames (%lu total)", lost_rx_frames - lost_rx_frames_last, lost_rx_frames);
            lost_rx_frames_last = lost_rx_frames;
        }

        uint32_t what = 0;
        while (xTaskNotifyWaitIndexed(notify_index, 0, 0xffffffff, &what, pdMS_TO_TICKS(10)) == pdPASS) {
            int64_t notify_time = get_timestamp_us();

            // Switch anonymous mode
            apply_automatic_retransmission();

            // Take serialized message and put it to Cyphal Tx queue
            if (what & std::to_underlying(Notify::Sender)) {
                sender_loop();
            }

            // Transmit CAN frames from the Cyphal Tx queue
            if (what & std::to_underlying(Notify::Tx)) {
                tx_loop();
            }

            // Receive CAN frames from driver and put them to Cyphal
            if (what & std::to_underlying(Notify::Rx)) {
                rx_loop();
            }

            // Log buffer overflow
            if (what & std::to_underlying(Notify::RxLost)) {
                lost_rx_frames_time = notify_time;
                lost_rx_frames = lost_rx_frames_isr.load();
            }

            // Ensure periodic tasks get some processing too
            if (notify_time - loop_start > 100'000) {
                break;
            }
        }
    }
}

void Task::notify(Notify what) {
    debug_assert(loop_task);

    if (xPortIsInsideInterrupt() != 0) { // We are in ISR
        BaseType_t woken = pdFALSE;
        xTaskNotifyIndexedFromISR(loop_task, notify_index, std::to_underlying(what), eSetBits, &woken);
        portYIELD_FROM_ISR(woken);
    } else {
        xTaskNotifyIndexed(loop_task, notify_index, std::to_underlying(what), eSetBits);
    }
}

void Task::timesync_loop() {
    if (timesync.period_us < TimeSync::period_min_us || timesync.period_us > TimeSync::period_max_us) {
        return; // Bail if timesync is disabled
    }

    // Gather timestamp from last sent timesync frame
    if (timesync.waiting_for_timestamp && timesync.last_sent == 0) {
        auto last_timestamp = driver.get_sent_timestamp();
        if (last_timestamp.has_value()) {
            timesync.last_sent = last_timestamp.value();
            timesync.waiting_for_timestamp = false;
        }
    }

    // Check if we should send another timesync frame
    CanardMicrosecond timestamp = static_cast<CanardMicrosecond>(get_timestamp_us());
    if (timestamp - timesync.last_attempt >= timesync.period_us) {
        // Create and serialize timesync message
        uavcan_time_Synchronization_1_0 msg = { timesync.last_sent };
        uint8_t buffer[uavcan_time_Synchronization_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_];
        size_t buffer_size_bytes = sizeof(buffer);
        [[maybe_unused]] int8_t ret_serialize = uavcan_time_Synchronization_1_0_serialize_(&msg, buffer, &buffer_size_bytes);
        debug_assert(ret_serialize == 0); // Serialization should never fail

        // Metadata
        CanardTransferMetadata meta = {
            .priority = CanardPriorityNominal,
            .transfer_kind = CanardTransferKindMessage,
            .port_id = uavcan_time_Synchronization_1_0_FIXED_PORT_ID_,
            .remote_node_id = CANARD_NODE_ID_UNSET,
            .transfer_id = timesync.transfer_id,
        };

        // Push to be sent
        int32_t ret_push = canardTxPush(&timesync.canard_tx_queue,
            &canard_instance,
            timestamp + TimeSync::period_min_us, // Needs to be minimal period
            &meta,
            buffer_size_bytes,
            buffer);

        if (ret_push == -CANARD_ERROR_OUT_OF_MEMORY) { // Run out of queue
            return; // Wait for tx_loop() to send something and try again next time
        }
        debug_assert(ret_push > 0); // Other negative or 0 means incorrect format

        // Message put to Tx buffer, advance state
        timesync.last_attempt = timestamp;
        timesync.waiting_for_timestamp = false; // Don't accidentally read previous timestamp again
        timesync.last_sent = 0;
        timesync.transfer_id = ProtoSender::increment_transfer_id(timesync.transfer_id);
        notify(Notify::Tx); // Notify tx_loop() to process the buffer
    }
}

void Task::serialization_loop() {
    if (is_anonymous()) {
        return; // No periodic messages in anonymous mode
    }

    auto senders_lock = RAIILock(senders_mutex); // Take senders mutex

    if (senders == nullptr) {
        return; // Bail if the list is empty
    }

    CanardMicrosecond timestamp = static_cast<CanardMicrosecond>(get_timestamp_us()); // Get once for all iterations

    // Iterate over the list and check what message should be sent
    /// @todo: If this becomes a bottleneck, we could rewrite this to use array instead of linked list.
    if (list_position == nullptr) {
        list_position = senders;
    }
    ProtoSenderPeriodic *selected_to_send = list_position;
    while (selected_to_send->is_to_be_sent(timestamp) == false) {
        selected_to_send = selected_to_send->get_next();
        if (selected_to_send == nullptr) {
            selected_to_send = senders;
        }

        if (selected_to_send == list_position) {
            return; // Back where we started, nothing to send
        }
    }
    debug_assert(selected_to_send != nullptr); // We should have found something to send

    // Send, don't wait for buffer
    if (directly_send(timestamp, *selected_to_send, 0)) {
        list_position = selected_to_send->get_next(); // Remember next item in the list
    }
}

void Task::sender_loop() {
    if (xSemaphoreTake(tx_buffer_semaphore, 0) == pdTRUE) {
        xSemaphoreGive(tx_buffer_semaphore); // Buffer is not used, nothing to do
        return;
    }

    if (tx_buffer_used.load() == false) { // Buffer was taken but not yet filled
        /// @note Keep tx_buffer_semaphore taken to be processed next time.
        return;
    }

    // Push to canard queue
    int32_t ret_push = canardTxPush(&canard_tx_queue, &canard_instance, tx_timeout, &tx_meta, tx_buffer_size, tx_buffer);
    if (ret_push == -CANARD_ERROR_OUT_OF_MEMORY) { // Run out of queue
        return; // Wait for tx_loop() to send something and try again next time
    }
    debug_assert(ret_push > 0); // Other negative or 0 means incorrect format

    tx_buffer_used = false;
    xSemaphoreGive(tx_buffer_semaphore); // Buffer can be used again
    notify(Notify::Tx); // Notify tx_loop() to process the buffer
    notify(Notify::Rx); // Notify rx_loop() that the buffer is free
    return;
}

void Task::tx_loop() {
    // Look if there is timesync frame to send
    if (const CanardTxQueueItem *timesync_tqi = canardTxPeek(&timesync.canard_tx_queue);
        timesync_tqi != nullptr
        && driver.send(timesync_tqi->frame, true)) {
        canard_instance.memory_free(&canard_instance, canardTxPop(&timesync.canard_tx_queue, timesync_tqi));
        timesync.waiting_for_timestamp = true;
        notify(Notify::Sender); // Notify sender_loop() about free space in canard_tx_queue
    }

    // Send frames from the queue while driver has space and while there are any left
    const CanardTxQueueItem *tqi = canardTxPeek(&canard_tx_queue); // Find the highest-priority frame
    while (tqi != nullptr) {
        // Attempt transmission only if the frame is not yet timed out while waiting in the TX queue.
        if ((tqi->tx_deadline_usec == 0) || (tqi->tx_deadline_usec > static_cast<CanardMicrosecond>(get_timestamp_us()))) {
            if (driver.send(tqi->frame) == false) {
                break; // Wait and try next time
            }
        } else {
            // Otherwise just drop it and move on to the next one.
            if (auto priority = priority_from_can_id(tqi->frame.extended_can_id);
                priority == CanardPriorityOptional) {
                log_warning(can, "Optional Cyphal Tx frame time out, %s port %u",
                    service_from_can_id(tqi->frame.extended_can_id) ? "service" : "message",
                    port_from_can_id(tqi->frame.extended_can_id));
                /// @note Optional is really optional. This will drop multiframe messages when the bus is busy.
            } else {
                log_error(can, "Cyphal Tx frame time out, %s port %u, priority %u",
                    service_from_can_id(tqi->frame.extended_can_id) ? "service" : "message",
                    port_from_can_id(tqi->frame.extended_can_id),
                    priority);
                if (error_callback) {
                    error_callback(Driver::Notification::TxLost);
                }
            }
        }

        canard_instance.memory_free(&canard_instance, canardTxPop(&canard_tx_queue, tqi));
        notify(Notify::Sender); // Notify sender_loop() about free space in canard_tx_queue
        tqi = canardTxPeek(&canard_tx_queue);
    }
}

void Task::rx_to_queue() {
    if (rx_queue_used.exchange(true) == true) {
        return; // Already used by task
    }
    if (rx_queue.isFull()) {
        rx_queue_used.store(false);
        return; // Nothing to do
    }

    auto write_pointer = rx_queue.get_write_pointer();

    if (driver.receive(write_pointer->frame, write_pointer->payload, &write_pointer->timestamp_us)) {
        rx_queue.increment_write_pointer();
        notify(Notify::Rx); // Check the frame in rx_loop()
    }

    rx_queue_used.store(false);
}

void Task::rx_loop() {
    // Take buffer mutex
    if (xSemaphoreTake(tx_buffer_semaphore, 0) != pdTRUE) {
        notify(Notify::Sender); // Buffer is used, we need sender_loop() to process it first
        return;
    }

    if (rx_queue.isEmpty()) {
        // No frame in the queue
        xSemaphoreGive(tx_buffer_semaphore); // Buffer can be used again

        // Try to clear frames from driver
        rx_to_queue();

        return;
    } else {
        notify(Notify::Rx); // Check again until we run out of frames
    }

    debug_assert(tx_buffer_reserved.load() == false);
    tx_buffer_reserved.store(true); // Mark that the buffer is reserved for the callback

    auto subers_lock = RAIIRecursiveLock(subers_mutex);
    debug_assert(subers_lock.is_locked());

    {
        // Get reference to the queue without copying, we will drop it later
        std::optional<RAIIElement> element;
        element.emplace(rx_queue.peek(),
            [&]() {
                rx_queue.drop();

                // Try to clear frames from driver
                rx_to_queue();
            });

        // Put to libcanard
        CanardRxTransfer transfer = {};
        CanardRxSubscription *subscription = nullptr;
        const int8_t canard_result = canardRxAccept(&canard_instance, element->e.timestamp_us, &element->e.frame, 0, &transfer, &subscription);
        if (canard_result > 0) {
            if (!is_anonymous() // Allow only if we are not in anonymous mode
                || (pnp_suber != nullptr && subscription == &pnp_suber->get_raw())) { // Or PnP subscription

                element.reset(); // Advance the queue output, to make more space

                ProtoSuber::static_callback(subscription, transfer, rx_buffer); // Call subscription callback
            }
            canard_instance.memory_free(&canard_instance, (void *)transfer.payload);
        } else if (canard_result != 0) { // Some error occurred
            log_error(can, "Canard Rx accept error %d", canard_result);
            if (error_callback) {
                error_callback(Driver::Notification::RxLost);
            }
        }
        // else The frame did not complete a transfer so there is nothing to do

    } // Drop the buffer reference if not already

    if (tx_buffer_reserved.load()) { // Nobody used the reserved buffer
        debug_assert(tx_buffer_used == false); // Buffer should not be used
        tx_buffer_reserved.store(false); // Unreserve
        xSemaphoreGive(tx_buffer_semaphore); // Buffer can be used again
    } else {
        /// @note Keep tx_buffer_semaphore taken until sender_loop() processes the buffer.
        debug_assert(tx_buffer_used); // Buffer should be used
    }
}

bool Task::directly_send(CanardMicrosecond timestamp, ProtoSender &sender, TickType_t timeout) {
    if (loop_task == nullptr) {
        return false; // Task is not running yet
    }

    if (is_anonymous() && &sender != pnp_sender) {
        return false; // Only PnP sender is allowed in anonymous mode
    }

    if (is_in_callback()) { // Sending from callback
        if (tx_buffer_reserved.load()) {
            tx_buffer_reserved.store(false); // Use reserved buffer
        } else {
            // Reserved buffer is spent, or we are in serialization_loop()
            // Cannot wait for buffer here
            if (timeout == 0) { // We allow try
                if (xSemaphoreTake(tx_buffer_semaphore, 0) != pdTRUE) {
                    return false;
                }
            } else {
                debug_assert(false); // Do not use this function this way
                return false;
            }
        }
    } else { // Sending from outside, we need to wait for buffer
        if (xSemaphoreTake(tx_buffer_semaphore, timeout) != pdTRUE) {
            return false;
        }
    }

    // Serialize message into tx_buffer
    tx_buffer_size = sizeof(tx_buffer);
    tx_timeout = timestamp; // Prepare with current time, to_send() will add message lifespan
    sender.to_send(tx_timeout, tx_meta, tx_buffer, tx_buffer_size);

    /// @note Keep tx_buffer_semaphore taken until sender_loop() processes the buffer.
    tx_buffer_used = true;
    notify(Notify::Sender); // Notify send to process the buffer
    return true;
}

void Task::add_sender(ProtoSenderPeriodic &sender) {
    if (xSemaphoreTake(senders_mutex, portMAX_DELAY) != pdTRUE) {
        debug_assert(false);
    }
    sender.add_next(senders);
    senders = &sender;
    xSemaphoreGive(senders_mutex);
}

void Task::remove_sender(ProtoSenderPeriodic &sender) {
    if (xSemaphoreTake(senders_mutex, portMAX_DELAY) != pdTRUE) {
        debug_assert(false);
    }
    ProtoSenderPeriodic *prev = senders;
    while (prev != nullptr && prev->get_next() != &sender) {
        prev = prev->get_next();
    }
    if (prev != nullptr) {
        prev->add_next(sender.get_next());
        sender.remove_next();
        list_position = nullptr; // Start over while iterating the list
    }
    xSemaphoreGive(senders_mutex);
}

void Task::add_suber(ProtoSuber &suber) {
    if (xSemaphoreTakeRecursive(subers_mutex, portMAX_DELAY) != pdTRUE) {
        debug_assert(false);
    }
    canardRxSubscribe(&canard_instance, suber.get_kind(), suber.get_port_id(),
        suber.get_extent(), suber.get_timeout(), &suber.get_raw());
    suber.get_raw().user_reference = &suber; // Link to ProtoSuber for callback
    xSemaphoreGiveRecursive(subers_mutex);
}

void Task::remove_suber(ProtoSuber &suber) {
    if (xSemaphoreTakeRecursive(subers_mutex, portMAX_DELAY) != pdTRUE) {
        debug_assert(false);
    }
    canardRxUnsubscribe(&canard_instance, suber.get_kind(), suber.get_port_id());
    suber.get_raw().user_reference = nullptr; // Unlink ProtoSuber
    xSemaphoreGiveRecursive(subers_mutex);
}
} // namespace can::cyphal

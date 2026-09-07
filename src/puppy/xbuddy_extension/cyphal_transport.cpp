/// @file
#include "cyphal_transport.hpp"

#include "hal_pub.hpp"
#include <cstddef>
#include <freertos/mutex.hpp>
#include <freertos/queue.hpp>
#include <freertos/timing.hpp>
#include <canard.h>
#include <o1heap/o1heap.h>
#include <raii/lock_guard.hpp>
#include <utils/byte_utils.hpp>

alignas(O1HEAP_ALIGNMENT) static std::array<std::byte, 12 * 1024> memory;

static void *allocate(CanardInstance *canard, size_t amount) {
    return o1heapAllocate(static_cast<O1HeapInstance *>(canard->user_reference), amount);
}

static void deallocate(CanardInstance *canard, void *pointer) {
    o1heapFree(static_cast<O1HeapInstance *>(canard->user_reference), pointer);
}

class TransportImplementation : public cyphal::Transport {
private:
    CanardInstance canard;
    CanardTxQueue tx_queue;
    freertos::Mutex mutex;
    freertos::Queue<CanardRxTransfer, 16> rx_queue;

public:
    TransportImplementation() {
        canard = canardInit(allocate, deallocate);
        canard.user_reference = o1heapInit(memory.data(), memory.size());
        canard.node_id = 0; // it's a me

        constexpr size_t queue_capacity = 100;
        tx_queue = canardTxInit(queue_capacity, CANARD_MTU_CAN_FD);
    }

    [[nodiscard]] bool subscribe(
        const CanardTransferKind transfer_kind,
        const CanardPortID port_id,
        const size_t extent) {
        LockGuard lock { mutex };
        CanardRxSubscription *subscription = (CanardRxSubscription *)canard.memory_allocate(&canard, sizeof(CanardRxSubscription));
        if (subscription == nullptr) {
            return false;
        }
        if (canardRxSubscribe(&canard,
                transfer_kind,
                port_id,
                extent,
                CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
                subscription)
            < 0) {
            canard.memory_free(&canard, subscription);
            return false;
        }

        // We never unsubscribe after we successfully subscribed. This is intentionally not freed.
        (void)subscription;
        return true;
    }

    [[noreturn]] void run() {
        for (;;) {
            constexpr uint8_t max_steps_in_row = 10;
            static uint8_t busy_counter = max_steps_in_row;
            if (!step()) {
                freertos::delay(1);
                busy_counter = max_steps_in_row;
            } else {
                if (--busy_counter == 0) {
                    // If we have been busy for a while, yield to other tasks to prevent starvation.
                    freertos::delay(1);
                    busy_counter = max_steps_in_row;
                }
            }
        }
    }

    bool step() {
        if (const CanardTxQueueItem *ti = canardTxPeek(&tx_queue)) {
            if (ti->tx_deadline_usec == 0 || ti->tx_deadline_usec > get_current_microseconds()) {
                if (!hal::pub::transmit(ti->frame.extended_can_id, { (std::byte *)ti->frame.payload, ti->frame.payload_size })) {
                    return true; // If the driver is busy, break and retry later.
                }
            }
            // After the frame is transmitted or if it has timed out while waiting, pop it from the queue and deallocate:
            canard.memory_free(&canard, canardTxPop(&tx_queue, ti));
            return true;
        } else if (hal::pub::RxFrame hal_frame; hal::pub::receive(hal_frame)) {
            CanardFrame canard_frame {
                .extended_can_id = hal_frame.identifier,
                .payload_size = hal_frame.size,
                .payload = hal_frame.data,
            };

            CanardRxTransfer transfer;
            const int8_t result = canardRxAccept(&canard,
                get_current_microseconds(),
                &canard_frame,
                0,
                &transfer,
                nullptr);
            if (result < 0) {
                // invalid arg or OOM
            } else if (result == 1) {
                if (rx_queue.try_send(transfer, 0)) {
                    // transfer will be dispose()d after receiving from the queue
                } else {
                    dispose(transfer);
                }
            }
            return true;
        } else {
            return false;
        }
    }

    [[nodiscard]] bool transmit(const CanardMicrosecond deadline, const CanardTransferMetadata &metadata, const Bytes &buffer) {
        LockGuard lock { mutex };
        return canardTxPush(&tx_queue, &canard, deadline, &metadata, buffer.size(), buffer.data()) > 0;
    }

    void dispose(CanardRxTransfer &transfer) {
        LockGuard lock { mutex };
        canard.memory_free(&canard, transfer.payload);
    }

    [[nodiscard]] bool receive(CanardRxTransfer &transfer) {
        return rx_queue.try_receive(transfer, 0);
    }

    CanardMicrosecond get_current_microseconds() {
        // For now, we don't really need to implement this.
        return 0;
    }
};

cyphal::Transport &cyphal::transport() {
    static TransportImplementation instance;
    return instance;
}

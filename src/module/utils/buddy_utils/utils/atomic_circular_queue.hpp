#pragma once

#include <limits>
#include <atomic>
#include <utility>
#include <type_traits>
#include <span>
#include <utils/uncopyable.hpp>
#include <bsod/bsod.h>

#ifndef ACQ_ASSERT
    #define ACQ_ASSERT(cond) debug_assert(cond)
#endif

/**
 * @brief   SPSC (Single Producer Single Consumer) Atomic Circular Queue class
 * @details Implementation of an atomic ring buffer data structure which can use all slots
 *          at the cost of strict index requirements.
 *          Please note that the "atomicity" only means here that enqueing and dequeing can be in different threads.
 *          All enqueues have to be done in the same thread however, and all the dequeues too.
 *
 *          This can store N items, where N needs to be a power of 2.
 *
 * @note Inspired from https://www.snellman.net/blog/archive/2016-12-13-ring-buffers/, were head and
 * tail are not masked (masking is performed only when accessing the elements) allowing to
 * distinguish between empty and full without losing a slot.
 */
template <typename T, typename index_t>
class BaseAtomicCircularQueueSizeless : public Uncopyable {
    static_assert(std::numeric_limits<index_t>::is_integer, "Buffer index has to be an integer type");
    static_assert(!std::numeric_limits<index_t>::is_signed, "Buffer index has to be an unsigned type");

#ifndef DO_NOT_CHECK_ATOMIC_LOCK_FREE
    static_assert(std::is_trivially_destructible_v<T>, "T must be trivially destructible");
#endif

private:
    std::atomic<index_t> head = 0;
    std::atomic<index_t> tail = 0;
#ifndef NDEBUG
    bool is_allocated = false; // for debugging information
#endif
    const std::span<T> queue; ///< The queue buffer

    /// Mask for accessing the queue
    index_t mask(index_t val) const { return val & (queue.size() - 1); }

protected:
    /**
     * @brief   Reserves a slot in the queue for writing.
     * @note    Only one slot may be allocated at a time
     * @details Reserves the next available slot and returns a pointer to it.
     *          The caller should write to this location, then call commit().
     *          The item is not visible to consumers until commit() is called.
     * @return  Pointer to the reserved slot, or nullptr if queue is full
     */
    T *allocate() {
        if (isFull()) {
            return nullptr;
        }

#ifndef NDEBUG
        ACQ_ASSERT(!is_allocated);
        is_allocated = true;
#endif

        return &queue[mask(tail)];
    }

    /**
     * @brief   Commits a previously allocated slot
     * @details Makes the item written to an allocated slot visible to consumers.
     *          Must be called after allocate() and writing to the returned pointer.
     */
    void commit([[maybe_unused]] T *item) {
#ifndef NDEBUG
        ACQ_ASSERT(item == &queue[mask(tail)]);
        ACQ_ASSERT(is_allocated);
        is_allocated = false;
#endif

        tail += 1;
    }

public:
    /**
     * @param buffer Pointer to the buffer to be used as queue
     * @param size Size of the buffer, must be a power of 2
     */
    inline BaseAtomicCircularQueueSizeless(T *buffer, index_t size)
        : queue(buffer, size) {
        debug_assert(size < std::numeric_limits<index_t>::max()); // Buffer size bigger than the index can support
        debug_assert((size & (size - 1)) == 0); // The size of the queue has to be a power of 2
#ifndef DO_NOT_CHECK_ATOMIC_LOCK_FREE
        debug_assert(size == 0 || std::atomic<index_t>::is_always_lock_free); // index_t is not lock-free
#endif
    }

    /// Removes an item from the queue and stores it into \param target
    /// \returns false if the queue was empty
    [[nodiscard]] bool dequeue(T &target) {
        if (isEmpty()) {
            return false;
        }

        index_t index = head;
        target = std::move(queue[mask(index++)]);
        head = index;
        return true;
    }

    /**
     * @brief   Removes and returns a item from the queue
     * @details Removes the oldest item on the queue, pointed to by the
     *          buffer_t head field. The item is returned to the caller.
     * @return  type T item
     */
    T dequeue() {
        ACQ_ASSERT(!isEmpty());

        index_t index = head;
        T ret = std::move(queue[mask(index++)]);
        head = index;
        return ret;
    }

    /**
     * @brief   Drop the oldest item from the queue, discarding it.
     * @return  True if the item was dropped, false if the queue was empty.
     */
    bool drop() {
        if (isEmpty()) {
            return false;
        }
        ++head;
        return true;
    }

    /**
     * @brief   Adds an item to the queue
     * @details Adds an item to the queue on the location pointed by the buffer_t
     *          tail variable. Returns false if no queue space is available.
     * @param   item Item to be added to the queue
     * @return  true if the operation was successful
     */
    [[nodiscard]] bool enqueue(const T &item) {
        if (isFull()) {
            return false;
        }

        index_t index = tail;
        queue[mask(index++)] = item;
        tail = index;
        return true;
    }

    /**
     * @brief   Adds an item to the queue
     * @details Adds an item to the queue on the location pointed by the buffer_t
     *          tail variable. Returns false if no queue space is available.
     * @param   item Item to be added to the queue
     * @return  true if the operation was successful
     */
    [[nodiscard]] bool enqueue(T &&item) {
        if (isFull()) {
            return false;
        }

        index_t index = tail;
        queue[mask(index++)] = std::move(item);
        tail = index;
        return true;
    }

    /**
     * @brief  Adds a new item constructed directly into the queue
     * @param  args Arguments to be forwarded to the element constructor
     * @return True if the operation was successful, false when full
     */
    template <typename... ARGS>
    [[nodiscard]] bool emplace(ARGS &&...args) {
        if (isFull()) {
            return false;
        }
        index_t index = tail;
        new (&queue[mask(index++)]) T(std::forward<ARGS>(args)...);
        tail = index;
        return true;
    }

    /**
     * @brief   Checks if the queue has no items
     * @details Returns true if there are no items on the queue, false otherwise.
     * @return  true if queue is empty
     */
    bool isEmpty() const { return head == tail; }

    /**
     * @brief   Checks if the queue is full
     * @details Returns true if the queue is full, false otherwise.
     * @return  true if queue is full
     */
    bool isFull() const { return count() == queue.size(); }

    /**
     * @brief   Gets the queue size
     * @details Returns the maximum number of items a queue can have.
     * @return  the queue size
     */
    constexpr index_t size() const { return queue.size(); }

    /**
     * @brief   Gets the next item from the queue without removing it
     * @details Returns a reference to the next item in the queue without removing it
     *          or updating the pointers.
     * @return  first item in the queue
     */
    T &peek() {
        debug_assert(!isEmpty());
        return queue[mask(head)];
    }

    /**
     * @brief   Gets the next item from the queue without removing it
     * @details Returns a non-mutable reference to the next item in the queue without
     *          removing it or updating the pointers.
     * @return  first item in the queue
     */
    const T &peek() const {
        debug_assert(!isEmpty());
        return queue[mask(head)];
    }

    /**
     * @brief   Gets the number of items on the queue
     * @details Returns the current number of items stored on the queue.
     * @return  number of items in the queue
     * @note    Please note that head and tail are not masked: they're not indexes into the buffer,
     *          but free counters. Masking is performed only when indexing.
     */
    index_t count() const { return tail - head; }

    /**
     * @brief Clear the contents of the queue
     */
    void clear() { head.store(tail); }

    /**
     * @brief Returns pointer to the actual write position (for DMA usage)
     * @return pointer to write position in case there is space for at least one record, nullptr otherwise
     */
    [[nodiscard]] T *get_write_pointer() {
        if (isFull()) {
            return nullptr;
        } else {
            return &queue[mask(tail.load())];
        }
    }

    /**
     * @brief Increment write pointer by one item when written by independent process (i.e. DMA)
     * This should be called after get_write_pointer() was called and the item was written to the queue.
     */
    void increment_write_pointer() {
        debug_assert(!isFull());
        tail.fetch_add(1);
    }
};

template <typename T, typename index_t>
class AtomicCircularQueueSizeless : public BaseAtomicCircularQueueSizeless<T, index_t> {
public:
    /**
     * @param buffer Pointer to the buffer to be used as queue
     * @param size Size of the buffer, must be a power of 2
     */
    inline AtomicCircularQueueSizeless(T *buffer, index_t size)
        : BaseAtomicCircularQueueSizeless<T, index_t>(buffer, size) {}

    /**
     * @brief   Adds an item to the queue
     * @details Adds an item to the queue on the location pointed by the buffer_t
     *          tail variable. Returns false if no queue space is available.
     * @param   item Item to be added to the queue
     * @return  true if the operation was successful
     */
    [[nodiscard]] bool enqueue(const T &item) {
        auto ptr = this->allocate();
        if (!ptr) {
            return false;
        }
        *ptr = item;
        this->commit(ptr);
        return true;
    }

    /**
     * @brief   Adds an item to the queue
     * @details Adds an item to the queue on the location pointed by the buffer_t
     *          tail variable. Returns false if no queue space is available.
     * @param   item Item to be added to the queue
     * @return  true if the operation was successful
     */
    [[nodiscard]] bool enqueue(T &&item) {
        auto ptr = this->allocate();
        if (!ptr) {
            return false;
        }
        *ptr = std::move(item);
        this->commit(ptr);
        return true;
    }
};

/**
 * @brief   SPSC (Single Producer Single Consumer) Atomic Circular Queue class
 * @details Implementation of an atomic ring buffer data structure which can use all slots
 *          at the cost of strict index requirements.
 *          Please note that the "atomicity" only means here that enqueuing and dequeuing can be in different threads.
 *          All enqueues have to be done in the same thread however, and all the dequeues too.
 *
 *          This can store N items, where N needs to be a power of 2.
 *
 * @note Inspired from https://www.snellman.net/blog/archive/2016-12-13-ring-buffers/, were head and
 * tail are not masked (masking is performed only when accessing the elements) allowing to
 * distinguish between empty and full without losing a slot.
 */
template <typename T, typename index_t, index_t N>
class AtomicCircularQueue : public AtomicCircularQueueSizeless<T, index_t> {
    static_assert(N < std::numeric_limits<index_t>::max(), "Buffer size bigger than the index can support");
    static_assert((N & (N - 1)) == 0, "The size of the queue has to be a power of 2");
#ifndef DO_NOT_CHECK_ATOMIC_LOCK_FREE
    static_assert(N == 0 || std::atomic<index_t>::is_always_lock_free, "index_t is not lock-free");
#endif

private:
    T queue[N];

public:
    inline AtomicCircularQueue()
        : AtomicCircularQueueSizeless<T, index_t>(queue, N) {}
};

template <typename T, typename index_t, index_t N>
class AtomicReservableCircularQueue : public BaseAtomicCircularQueueSizeless<T, index_t> {
private:
    T queue[N];

public:
    using BaseAtomicCircularQueueSizeless<T, index_t>::allocate;
    using BaseAtomicCircularQueueSizeless<T, index_t>::commit;

    AtomicReservableCircularQueue()
        : BaseAtomicCircularQueueSizeless<T, index_t>(queue, N) {}
};

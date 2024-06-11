/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include <stdint.h>
#include <limits>
#include <atomic>

/**
 * @brief   Circular Queue class
 * @details Implementation of the classic ring buffer data structure
 */
template<typename T, uint8_t N>
class CircularQueue {
  private:

    /**
     * @brief   Buffer structure
     * @details This structure consolidates all the overhead required to handle
     *          a circular queue such as the pointers and the buffer vector.
     */
    struct buffer_t {
      uint8_t head;
      uint8_t tail;
      uint8_t count;
      uint8_t size;
      T queue[N];
    } buffer;

  public:
    /**
     * @brief   Class constructor
     * @details This class requires two template parameters, T defines the type
     *          of item this queue will handle and N defines the maximum number of
     *          items that can be stored on the queue.
     */
    CircularQueue() {
      buffer.size = N;
      buffer.count = buffer.head = buffer.tail = 0;
    }

    /**
     * @brief   Removes and returns a item from the queue
     * @details Removes the oldest item on the queue, pointed to by the
     *          buffer_t head field. The item is returned to the caller.
     * @return  type T item
     */
    T dequeue() {
      if (isEmpty()) return T();

      uint8_t index = buffer.head;

      --buffer.count;
      if (++buffer.head == buffer.size)
        buffer.head = 0;

      return buffer.queue[index];
    }

    /**
     * @brief   Adds an item to the queue
     * @details Adds an item to the queue on the location pointed by the buffer_t
     *          tail variable. Returns false if no queue space is available.
     * @param   item Item to be added to the queue
     * @return  true if the operation was successful
     */
    bool enqueue(T const &item) {
      if (isFull()) return false;

      buffer.queue[buffer.tail] = item;

      ++buffer.count;
      if (++buffer.tail == buffer.size)
        buffer.tail = 0;

      return true;
    }

    /**
     * @brief   Checks if the queue has no items
     * @details Returns true if there are no items on the queue, false otherwise.
     * @return  true if queue is empty
     */
    bool isEmpty() { return buffer.count == 0; }

    /**
     * @brief   Checks if the queue is full
     * @details Returns true if the queue is full, false otherwise.
     * @return  true if queue is full
     */
    bool isFull() { return buffer.count == buffer.size; }

    /**
     * @brief   Gets the queue size
     * @details Returns the maximum number of items a queue can have.
     * @return  the queue size
     */
    uint8_t size() { return buffer.size; }

    /**
     * @brief   Gets the next item from the queue without removing it
     * @details Returns the next item in the queue without removing it
     *          or updating the pointers.
     * @return  first item in the queue
     */
    T peek() { return buffer.queue[buffer.head]; }

    /**
     * @brief Gets the number of items on the queue
     * @details Returns the current number of items stored on the queue.
     * @return number of items in the queue
     */
    uint8_t count() { return buffer.count; }
};

/**
 * @brief   Atomic Circular Queue class
 * @details Implementation of an atomic ring buffer data structure which can use all slots
 *          at the cost of strict index requirements
 * @note Inspired from https://www.snellman.net/blog/archive/2016-12-13-ring-buffers/
 */
template<typename T, typename index_t, index_t N>
class AtomicCircularQueue {
  private:

    /**
     * @brief   Buffer structure
     * @details This structure consolidates all the overhead required to handle
     *          a circular queue such as the pointers and the buffer vector.
     */
    struct buffer_t {
      std::atomic<index_t> head;
      std::atomic<index_t> tail;
      T queue[N];
    } buffer;

    static index_t mask(index_t val) { return val & (N - 1); }

  public:
    /**
     * @brief   Class constructor
     * @details This class requires two template parameters, T defines the type
     *          of item this queue will handle and N defines the maximum number of
     *          items that can be stored on the queue.
     */
    AtomicCircularQueue() {
      static_assert(std::numeric_limits<index_t>::is_integer, "Buffer index has to be an integer type");
      static_assert(!std::numeric_limits<index_t>::is_signed, "Buffer index has to be an unsigned type");
      static_assert(N < std::numeric_limits<index_t>::max(), "Buffer size bigger than the index can support");
      static_assert((N & (N - 1)) == 0, "The size of the queue has to be a power of 2");
      static_assert(decltype(buffer_t::head)::is_always_lock_free, "index_t is not lock-free");
      static_assert(decltype(buffer_t::tail)::is_always_lock_free, "index_t is not lock-free");
      buffer.head = buffer.tail = 0;
    }

    /**
     * @brief   Removes and returns a item from the queue
     * @details Removes the oldest item on the queue, pointed to by the
     *          buffer_t head field. The item is returned to the caller.
     * @return  type T item
     */
    T dequeue() {
      if (isEmpty()) return T();

      index_t index = buffer.head;
      T ret = buffer.queue[mask(index++)];
      buffer.head = index;
      return ret;
    }

    /**
     * @brief   Adds an item to the queue
     * @details Adds an item to the queue on the location pointed by the buffer_t
     *          tail variable. Returns false if no queue space is available.
     * @param   item Item to be added to the queue
     * @return  true if the operation was successful
     */
    bool enqueue(T const &item) {
      if (isFull()) return false;

      index_t index = buffer.tail;
      buffer.queue[mask(index++)] = item;
      buffer.tail = index;
      return true;
    }

    /**
     * @brief   Checks if the queue has no items
     * @details Returns true if there are no items on the queue, false otherwise.
     * @return  true if queue is empty
     */
    bool isEmpty() { return buffer.head == buffer.tail; }

    /**
     * @brief   Checks if the queue is full
     * @details Returns true if the queue is full, false otherwise.
     * @return  true if queue is full
     */
    bool isFull() { return count() == N; }

    /**
     * @brief   Gets the queue size
     * @details Returns the maximum number of items a queue can have.
     * @return  the queue size
     */
    index_t size() { return N; }

    /**
     * @brief   Gets the next item from the queue without removing it
     * @details Returns the next item in the queue without removing it
     *          or updating the pointers.
     * @return  first item in the queue
     */
    T peek() { return buffer.queue[mask(buffer.head)]; }

    /**
     * @brief Gets the number of items on the queue
     * @details Returns the current number of items stored on the queue.
     * @return number of items in the queue
     */
    index_t count() { return buffer.tail - buffer.head; }
};

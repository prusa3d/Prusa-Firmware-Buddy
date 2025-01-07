#include "ring_allocator.hpp"

#include <cassert>

constexpr size_t alignment = 4;

namespace buddy {

RingAllocator::RingAllocator(size_t size)
    : size(size) {
    buffer.reset(new uint8_t[size]);
    // Check: All pointers are aligned to (at least) 4.
    assert(reinterpret_cast<uintptr_t>(buffer.get()) % alignment == 0);
    assert(size > sizeof(Record));
    Record *head = reinterpret_cast<Record *>(buffer.get());
    head->next = head->prev = nullptr;
    head->in_use = false;
    alloc_head = head;
}

void RingAllocator::free(void *ptr) {
    // Pointer to "bytes", because that actually has pointer arithmetics well-defined.
    uint8_t *ptr_b = reinterpret_cast<uint8_t *>(ptr);
    ptr_b -= sizeof(Record);
    // Check this is "our" pointer
    assert(ptr_b >= buffer.get());
    assert(ptr_b < buffer.get() + size);
    Record *rec = reinterpret_cast<Record *>(ptr_b);
    assert(rec->in_use);

    rec->in_use = false;

    // If there's an unused section before or after, make it into one big
    // chunk.
    merge(rec, rec->next);
    merge(rec->prev, rec);
}

void RingAllocator::merge(Record *l, Record *r) {
    // Sitting at the leftmost or rightmost edge.
    if (l == nullptr || r == nullptr) {
        return;
    }

    // We are merging only stuff that's unused.
    if (l->in_use || r->in_use) {
        return;
    }

    // r is disappearing, so if it is the one we are pointing at, just rewind
    // it towards the left one (which will be the new "complete" thing)
    if (r == alloc_head) {
        alloc_head = l;
    }

    if (r->next) {
        assert(r->next->prev == r);
        r->next->prev = l;
    }
    l->next = r->next;
}

void *RingAllocator::allocate(size_t size) {
    // Include the record header
    size += sizeof(Record);
    // Make sure to round it up to multiple of alignment
    size = (size + alignment - 1) / alignment * alignment;

    Record *stop = alloc_head;

    do {
        size_t record_size = available_size(alloc_head);
        if (!alloc_head->in_use && record_size >= size) {
            split(alloc_head, record_size, size);
            alloc_head->in_use = true;
            // Note: +1 means +1 _record_, not byte - point just after the record header.
            // Note: Leaving alloc_head at the current record for simplicity
            // (not having to deal with the last record at multiple places,
            // besides we may free it before we do another allocation).
            return reinterpret_cast<void *>(alloc_head + 1);
        }

        // This record didn't work (either used or too small). Move to the next one.
        alloc_head = alloc_head->next;
        // Got past the last one, rewind to the beginning.
        if (!alloc_head) {
            alloc_head = reinterpret_cast<Record *>(buffer.get());
        }
    } while (alloc_head != stop);

    // Didn't fit in any "gap"
    return nullptr;
}

void RingAllocator::split(Record *record, size_t current_size, size_t new_size) {
    assert(current_size >= new_size);
    assert(new_size % alignment == 0);
    size_t extra = current_size - new_size;
    // The "tail" is not large enough to hold even the record.
    if (extra < sizeof(Record)) {
        return;
    }

    uint8_t *record_pos = reinterpret_cast<uint8_t *>(record);
    Record *new_record = reinterpret_cast<Record *>(record_pos + new_size);
    assert(reinterpret_cast<uintptr_t>(new_record) % alignment == 0);
    new_record->in_use = false;
    new_record->next = record->next;
    new_record->prev = record;
    record->next->prev = new_record;
    record->next = new_record;
}

size_t RingAllocator::available_size(Record *record) {
    const uintptr_t rec_pos = reinterpret_cast<uintptr_t>(record);
    const uintptr_t end_pos = reinterpret_cast<uintptr_t>(buffer.get()) + size;
    const uintptr_t next_pos = record->next ? reinterpret_cast<uintptr_t>(record->next) : end_pos;
    assert(next_pos >= rec_pos + sizeof(Record));
    assert(next_pos <= end_pos);
    return next_pos - rec_pos;
}

} // namespace buddy

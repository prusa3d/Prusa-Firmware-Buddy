#include "ring_allocator.hpp"

#include <logging/log.hpp>

#include <cassert>

LOG_COMPONENT_DEF(RingAllocator, logging::Severity::info);

// Note: The alignment is possibly wrong on the 64bit system where tests run
// (it's fine on the real printer). However, that system also supports
// unaligned pointer access and all that, so for unit tests, that's fine
// enough.
constexpr size_t alignment = 4;

namespace buddy {

RingAllocator::RingAllocator(size_t size)
    : size(size) {
    buffer.reset(new uint8_t[size]);
    // Check: All pointers are aligned to (at least) 4.
    assert(reinterpret_cast<uintptr_t>(buffer.get()) % alignment == 0);
    assert(size > sizeof(Record));
    // Check: Buffer is smaller than our available offset (2^16)
    assert(size < 65536);
    Record *head = reinterpret_cast<Record *>(buffer.get());
    head->next = head->prev = 0;
    head->in_use = false;
    alloc_head = head;
}

void RingAllocator::free(void *ptr) {
    log_debug(RingAllocator, "Free %p", ptr);

    // Pointer to "bytes", because that actually has pointer arithmetics well-defined.
    uint8_t *ptr_b = reinterpret_cast<uint8_t *>(ptr);
    ptr_b -= sizeof(Record);
    // Check this is "our" pointer
    assert(ptr_b >= buffer.get());
    assert(ptr_b < buffer.get() + size);
    Record *rec = reinterpret_cast<Record *>(ptr_b);
    assert(rec->in_use);

    rec->in_use = false;
#ifdef EXTRA_RING_ALLOCATOR_LOGGING
    used_records--;
    used_bytes -= available_size(rec);
#endif

    // If there's an unused section before or after, make it into one big
    // chunk.
    merge(rec, get_next(rec));
    merge(get_prev(rec), rec);
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

    log_debug(RingAllocator, "Merge %p with %p", l, r);

    // r is disappearing, so if it is the one we are pointing at, just rewind
    // it towards the left one (which will be the new "complete" thing)
    if (r == alloc_head) {
        alloc_head = l;
    }

    if (r->next) {
        assert(get_prev(get_next(r)) == r);
        Record *r_next = get_next(r);
        r_next->prev = offset_between(l, r_next);
        l->next = offset_between(l, r_next);
    } else {
        l->next = 0;
    }

#ifdef EXTRA_RING_ALLOCATOR_LOGGING
    records--;
#endif
}

void *RingAllocator::allocate(size_t size) {
#ifdef EXTRA_RING_ALLOCATOR_LOGGING
    log_debug(RingAllocator, "Allocate %zu (used: %zu, records %zu/%zu)", size, used_bytes, used_records, records);
#else
    log_debug(RingAllocator, "Allocate %zu", size);
#endif
    // Include the record header
    size += sizeof(Record);
    // Make sure to round it up to multiple of alignment
    size = (size + alignment - 1) / alignment * alignment;

    Record *stop = alloc_head;

    do {
        size_t record_size = available_size(alloc_head);
        if (!alloc_head->in_use && record_size >= size) {
            log_debug(RingAllocator, "Found record %p of %zu", alloc_head, record_size);
            split(alloc_head, record_size, size);
            alloc_head->in_use = true;
#ifdef EXTRA_RING_ALLOCATOR_LOGGING
            used_records++;
            used_bytes += available_size(alloc_head);
#endif
            // Note: +1 means +1 _record_, not byte - point just after the record header.
            // Note: Leaving alloc_head at the current record for simplicity
            // (not having to deal with the last record at multiple places,
            // besides we may free it before we do another allocation).
            return reinterpret_cast<void *>(alloc_head + 1);
        }

        log_debug(RingAllocator, "Skip %p", alloc_head);

        // This record didn't work (either used or too small). Move to the next one.
        alloc_head = get_next(alloc_head);
        // Got past the last one, rewind to the beginning.
        if (!alloc_head) {
            alloc_head = reinterpret_cast<Record *>(buffer.get());
        }
    } while (alloc_head != stop);

    log_debug(RingAllocator, "Couldn't find");

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
    new_record->prev = new_record->next = 0;
    new_record->prev = encode_offset(new_size);
    if (record->next != 0) {
        new_record->next = encode_offset(extra);
        get_next(record)->prev = encode_offset(extra);
    }
    record->next = encode_offset(new_size);
#ifdef EXTRA_RING_ALLOCATOR_LOGGING
    records++;
#endif
}

size_t RingAllocator::available_size(Record *record) const {
    const uintptr_t rec_pos = reinterpret_cast<uintptr_t>(record);
    const uintptr_t end_pos = reinterpret_cast<uintptr_t>(buffer.get()) + size;
    const uintptr_t next_pos = record->next ? reinterpret_cast<uintptr_t>(record) + decode_offset(record->next) : end_pos;

    assert(next_pos >= rec_pos + sizeof(Record));
    assert(next_pos <= end_pos);
    return next_pos - rec_pos;
}

RingAllocator::Record *RingAllocator::get_next(Record *rec) const {
    if (rec->next == 0) {
        return nullptr;
    }

    uint8_t *ptr = reinterpret_cast<uint8_t *>(rec);
    ptr += decode_offset(rec->next);
    Record *next = reinterpret_cast<Record *>(ptr);
    return next;
}

RingAllocator::Record *RingAllocator::get_prev(Record *rec) const {
    if (rec->prev == 0) {
        return nullptr;
    }

    uint8_t *ptr = reinterpret_cast<uint8_t *>(rec);
    ptr -= decode_offset(rec->prev);
    Record *prev = reinterpret_cast<Record *>(ptr);
    return prev;
}

uint16_t RingAllocator::offset_between(const Record *from, const Record *to) const {
    assert(from <= to);

    // Compute the raw byte difference between two pointers:
    auto diff = reinterpret_cast<const std::uint8_t *>(to)
        - reinterpret_cast<const std::uint8_t *>(from);

    // Then encode into 15-bit storage
    return encode_offset(static_cast<std::uint16_t>(diff));
}

#ifdef UNITTESTS
void RingAllocator::sanity_check() {
    Record *r = reinterpret_cast<Record *>(buffer.get());
    assert(r != nullptr);

    bool seen_head = false;
    bool first = true;
    bool last_in_use = true;

    while (r != nullptr) {
        if (r == alloc_head) {
            assert(!seen_head);
            seen_head = true;
        }

        if (get_next(r) != nullptr) {
            assert(get_next(r) > r);
            get_next(r)->prev = r->next;
        }

        if (first) {
            first = false;
        } else {
            assert(r->prev != 0);
            assert(get_next(get_prev(r)) == r);
        }

        assert(last_in_use || r->in_use);
        last_in_use = r->in_use;

        r = get_next(r);
    }

    assert(seen_head);
}
#endif

} // namespace buddy

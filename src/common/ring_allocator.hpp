#pragma once

#include <memory>

// Adds extra computations and space for debug logging, we probably don't want
// that in general.
// #define EXTRA_RING_ALLOCATOR_LOGGING

namespace buddy {

/// A ring allocator for use with incoming network packets.
///
/// LwIP by default uses memory pools with fixed-sized buckets. This is fast to
/// allocate and doesn't suffer from fragmentation, but usually wastes a lot of
/// memory if packets are small. So to work properly, the pool needs to be
/// oversized and we can't afford that. Therefore, we tended to fill the pool
/// with large packets that wait for something and didn't have enough space for
/// eg. ACKs for our outbound data, leading to extensive retransmits or even
/// TCP deadlocks.
///
/// Instead we leverage the fact network packets are processed in mostly FIFO
/// manner (packets can be out of order, or missing and that slightly breaks
/// the FIFO, but roughly the packets are eventually either used in
/// close-to-in-order or the connection times out).
///
/// We therefore use a ringbuffer allocator. It is able to free them out of
/// order, with the risk of possibly leading to fragmentation if a packet is
/// left there for a long time ‒ but as mentioned above, that _mostly_ doesn't
/// happen and statistically, we tend to allocate from already freed area
/// and we can eat from it as we like (or skip to next large enough free area).
///
/// Experiments lead to conclusion the connections are more stable and look
/// more healthy in wireshark (eg. we properly and nicely handle the state
/// where our inbound TCP window is full, we are not consuming and just
/// sending).
///
/// Internal implementation:
///
/// * We have a buffer.
/// * The buffer is split into records. Each record can be either in use or free.
/// * The records form a linked list, so we can move around and merge/split
///   them as needed. However, the linked list is „in order“ inside the buffer.
/// * Two free buffers are never side by side. We merge them when we see them
///   (eg. during freeing).
/// * When allocating, we go from last position we were at and try to allocate
///   from the current record (splitting it). If we can't (this one is in use, or
///   is not large enough), we move forward or wrap around. If we get back to
///   where we started, we give up (we are full).
class RingAllocator {
private:
    // This is a compressed version of Record.
    //
    // * Instead of actual pointers (absolute path) it uses 15-bit offsets (relative path).
    // * This is possible due to buffer_size < 2^16 bytes and addresses being aligned.
    struct Record {
        bool in_use : 1;
        uint16_t prev : 15;
        uint16_t next : 15;
    };
    static_assert(sizeof(Record) == 4);

#ifdef EXTRA_RING_ALLOCATOR_LOGGING
    size_t used_bytes = 0;
    size_t used_records = 0;
    size_t records = 1;
#endif

    std::unique_ptr<uint8_t[]> buffer;
    size_t size;
    /// Finger to where we were sitting.
    Record *alloc_head;

    void merge(Record *l, Record *r);
    /// Including the record header itself.
    size_t available_size(Record *r) const;
    /// Both size are including the record header.
    ///
    /// Splits only if the new header fits.

    void split(Record *record, size_t current_size, size_t new_size);

    // offset helper functions
    Record *get_next(Record *rec) const;
    Record *get_prev(Record *rec) const;
    // Calculated relative offset between two Records
    uint16_t offset_between(const Record *from, const Record *to) const;
    // Compresses an even-aligned 16-bit offset into a 15-bit field by storing offset / 2.
    inline uint16_t encode_offset(uint16_t value) const {
        return (value >> 1); // store as value / 2
    }
    // Restores a 15-bit encoded offset to its full 16-bit even-aligned value.
    inline uint16_t decode_offset(uint16_t value) const {
        return (value << 1); // restore original value
    }

public:
    RingAllocator(size_t size);
    /// Like malloc.
    void *allocate(size_t size);
    /// Like free.
    void free(void *ptr);

#ifdef UNITTESTS
    /// To be used in tests. Asserts if something seems wrong.
    void sanity_check();
#endif
};

} // namespace buddy

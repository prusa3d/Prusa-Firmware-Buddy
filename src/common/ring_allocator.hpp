#pragma once

#include <memory>

namespace buddy {

class RingAllocator {
private:
    // TODO: This can be compressed.
    //
    // * We probably don't need actual pointers, 16-bit offsets should be fine.
    // * The used flag could be embedded into odd/even bit (the lowest bit) of
    //   one of them if we assume everything lives on even addresses (which it
    //   does).
    struct Record {
        Record *prev;
        Record *next;
        bool in_use;
    };

    std::unique_ptr<uint8_t[]> buffer;
    size_t size;
    Record *alloc_head;

    void merge(Record *l, Record *r);
    // Including the record header itself
    size_t available_size(Record *r);
    void split(Record *record, size_t current_size, size_t new_size);

public:
    RingAllocator(size_t size);
    void *allocate(size_t size);
    void free(void *ptr);
};

} // namespace buddy

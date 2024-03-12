#include "mmu2_bootloader_memory_resource.hpp"

using namespace MMU2::bootloader;

StackMemoryResource::StackMemoryResource(void *buffer, size_t buffer_size)
    : buffer(reinterpret_cast<uint8_t *>(buffer))
    , buffer_size(buffer_size) {
}

void StackMemoryResource::do_deallocate([[maybe_unused]] void *p, size_t bytes, [[maybe_unused]] size_t alignment) {
    // Always align to 8 bytes to keep the code simple
    assert(alignment <= 8);
    bytes = (bytes + 8) & ~7;

    // Check that we're removing from the end of the stack
    assert(p == buffer + used_bytes - bytes);

    used_bytes -= bytes;
}

void *StackMemoryResource::do_allocate(size_t bytes, [[maybe_unused]] size_t alignment) {
    // Always align to 8 bytes to keep the code simple
    assert(alignment <= 8);
    bytes = (bytes + 8) & ~7;

    used_bytes += bytes;

    // Check that we have enough memory on co_stack
    if (used_bytes > buffer_size) {
        bsod("StaticStackMemoryResource OOM");
    }

    return buffer + used_bytes - bytes;
}

bool StackMemoryResource::do_is_equal(const std::pmr::memory_resource &other) const noexcept {
    return &other == this;
}

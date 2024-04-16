#pragma once

#include <memory_resource>

#include <bsod.h>
#include <assert.h>

namespace MMU2::bootloader {

/// Memory resource that handles allocations/deallocations in a stack-like manner.
/// Violating the LIFO alloc/dealloc order results in UB.
class StackMemoryResource : public std::pmr::memory_resource {

public:
    StackMemoryResource(void *buffer, size_t buffer_size);

public:
    void *do_allocate(size_t bytes, [[maybe_unused]] size_t alignment) final;
    void do_deallocate(void *p, size_t bytes, size_t alignment) final;

    bool do_is_equal(const std::pmr::memory_resource &other) const noexcept final;

private:
    uint8_t *buffer;
    size_t buffer_size;
    size_t used_bytes = 0;
};

/// Memory resource that handles allocations/deallocations in a stack-like manner.
/// Calling deallocate on a pointer that was not allocated last results in UB.
template <size_t num_bytes>
class StaticStackMemoryResource final : public StackMemoryResource {

public:
    StaticStackMemoryResource()
        : StackMemoryResource(buffer.data(), buffer.size()) {}
    StaticStackMemoryResource(const StaticStackMemoryResource &) = delete;

    StaticStackMemoryResource &operator=(const StaticStackMemoryResource &) = delete;

private:
    alignas(8) std::array<uint8_t, num_bytes> buffer;
};

} // namespace MMU2::bootloader

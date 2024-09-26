#pragma once

#include <cstddef>

namespace freertos {

// We use erased storage in order to not pollute the scope with FreeRTOS internals.
// The actual size and alignment are statically asserted in implementation file.
// These numbers depend on particular FreeRTOSConfig.h and the platform.
#ifdef UNITTESTS
static constexpr size_t internal_storage_align = 8;
static constexpr size_t internal_storage_size = 168;
#else
static constexpr size_t internal_storage_align = 4;
static constexpr size_t internal_storage_size = FREERTOS_INTERNAL_STORAGE_SIZE;
#endif

// In current implementation of FreeRTOS all these are actually the same
// internal structure, but let's be more future-proof here...
static constexpr size_t mutex_storage_align = internal_storage_align;
static constexpr size_t mutex_storage_size = internal_storage_size;
static constexpr size_t queue_storage_align = internal_storage_align;
static constexpr size_t queue_storage_size = internal_storage_size;
static constexpr size_t semaphore_storage_align = internal_storage_align;
static constexpr size_t semaphore_storage_size = internal_storage_size;

} // namespace freertos

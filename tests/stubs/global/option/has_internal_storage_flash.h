#pragma once

#define HAS_INTERNAL_STORAGE_FLASH() 0

#ifdef __cplusplus
namespace option {
inline constexpr bool has_internal_storage_flash = 0;
}
#endif

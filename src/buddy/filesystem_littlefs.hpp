#pragma once

#include <option/has_internal_storage_flash.h>

/// Initialize all persistent littlefs filesystems (NOR system, NAND internal).
void filesystem_littlefs_init();

#if HAS_INTERNAL_STORAGE_FLASH()
/// Unmount the internal storage filesystem, e.g. before erasing the chip.
void internal_storage_unmount();
#endif

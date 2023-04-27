#include <bootloader/bootloader.hpp>
#include "log.h"

using Version = buddy::bootloader::Version;

LOG_COMPONENT_DEF(Bootloader, LOG_SEVERITY_INFO);

Version buddy::bootloader::get_version() {
    Version *const bootloader_version = (Version *)0x0801FFFA;
    return *bootloader_version;
}

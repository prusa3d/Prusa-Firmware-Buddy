#pragma once
#include <stdint.h>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/// Project's version (4.0.2)
extern const char project_version[];

/// Project's version (4.0.2) separated to components
extern const uint16_t project_version_major;
extern const uint16_t project_version_minor;
extern const uint16_t project_version_patch;

/// Full project's version (4.0.3-BETA+1035.PR111.B4)
extern const char project_version_full[];

/// Project's version suffix (-BETA+1035.PR111.B4)
extern const char project_version_suffix[];

/// Project's short version suffix (-BETA+1035)
extern const char project_version_suffix_short[];

/// Project's build number (number of commits in a branch)
extern const int project_build_number;

/// Firmware name
extern const char project_firmware_name[];

// !!! DO NOT MODIFY, THIS IS USED TO IDENTIFY CRASH DUMPS
// !!! IF THIS IS TO BE MODIFIED, CHANGE THE MAGIC AND ADD A CASE TO THE crash_dump_info.py SCRIPT
struct __attribute__((packed)) BuildIdentification {
    /// Magical constant used for locating the structure in the crash dump
    const char magic[10] = "#BUILDID#";

    /// Hash of the commit the firmware was built from
    char commit_hash[41];

    /// Full project's version (4.0.3-BETA+1035.PR111.B4)
    char project_version_full[48];

    /// Bitset of enabled translations
    uint16_t enabled_translations;

    uint8_t printer_code;

    /// Whether there were some local modifications to the commit
    bool commit_dirty;

    bool has_bootloader;
};

// !!! DO NOT MODIFY, THIS IS USED TO IDENTIFY CRASH DUMPS
// !!! IF THIS IS TO BE MODIFIED, CHANGE THE MAGIC AND ADD A CASE TO THE crash_dump_info.py SCRIPT
static_assert(sizeof(BuildIdentification) == 104);

/// Identification of the build, used for identifying crash dumps
extern const BuildIdentification project_build_identification;

/**
 * @brief Prints project_version into buffer without dots (4.0.2 -> 402)
 */
void fill_project_version_no_dots(char *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif //__cplusplus

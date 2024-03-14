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

/**
 * @brief Prints project_version into buffer without dots (4.0.2 -> 402)
 */
void fill_project_version_no_dots(char *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif //__cplusplus

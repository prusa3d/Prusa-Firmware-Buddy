/// @file version.h
#pragma once

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#define project_version_major 2
#define project_version_minor 0
#define project_version_revision 0

/// Project's version (2.0.0)
extern const char project_version[];

/// Full project's version (2.0.0-BETA+1035.PR111.B4)
extern const char project_version_full[];

/// Project's version suffix (-BETA+1035.PR111.B4)
extern const char project_version_suffix[];

/// Project's short version suffix (+1035)
extern const char project_version_suffix_short[];

/// Project's build number (number of commits in a branch)
extern const int project_build_number;

/// Firmware name
extern const char project_firmware_name[];

#ifdef __cplusplus
}
#endif //__cplusplus

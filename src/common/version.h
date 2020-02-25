#pragma once

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/// Project's version ...
extern const int project_version_major; // major (4)
extern const int project_version_minor; // minor (0)
extern const int project_version_patch; // patch (2)

/// Project's version (4.0.2)
extern const char project_version[];

/// Full project's version (4.0.3-BETA+1035.PR111.B4)
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

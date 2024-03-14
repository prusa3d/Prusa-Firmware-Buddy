/// @file
#pragma once
#include <stdint.h>

/// Project's version (2.0.0)
static constexpr char project_version[] = FW_VERSION_STR;

/// Full project's version (2.0.0-BETA+1035.PR111.B4)
static constexpr char project_version_full[] = FW_VERSION_FULL_STR;

/// Project's version suffix (-BETA+1035.PR111.B4)
static constexpr char project_version_suffix[] = FW_VERSION_SUFFIX_STR;

/// Project's short version suffix (+1035)
static constexpr char project_version_suffix_short[] = FW_VERSION_SUFFIX_SHORT_STR;

/// Project's major version
static constexpr uint8_t project_major = PROJECT_VERSION_MAJOR;

/// Project's minor version
static constexpr uint8_t project_minor = PROJECT_VERSION_MINOR;

/// Project's revision number
static constexpr uint16_t project_revision = PROJECT_VERSION_REV;

/// Project's build number (number of commits in a branch)
static constexpr uint16_t project_build_number = PROJECT_BUILD_NUMBER;

static_assert(PROJECT_VERSION_MAJOR <= UINT8_MAX);
static_assert(PROJECT_VERSION_MINOR <= UINT8_MAX);
static_assert(PROJECT_VERSION_REV <= UINT8_MAX);
static_assert(PROJECT_BUILD_NUMBER <= UINT16_MAX);

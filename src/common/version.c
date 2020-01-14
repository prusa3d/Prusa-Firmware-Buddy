//! @file

#include "version.h"
#include "config.h"

#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    #define FWNAME "Buddy_MINI";
#endif //(PRINTER_TYPE ==

#define _STR(x) #x
#define STR(x) _STR(x)

//TODO:
// MAJ/MIN/SUB in structure from FW_VERSION

#define VERSION_MAJ(v) #v
#define VERSION_MIN(v) #v
#define VERSION_SUB(v) #v

#define VERSION(ver) \
    VERSION_MAJ(4)   \
    "." VERSION_MIN(0) "." VERSION_SUB(2)

const char version_firmware_name[] = FWNAME;

#ifdef PRERELEASE
//! @brief semantic version (https://semver.org) is Prusa3D standard
const char version_version[] = VERSION(FW_VERSION) "-" STR(PRERELEASE) "+" STR(FW_BUILDNR) FW_BUILDSX;
#else
//! @brief semantic version (https://semver.org) is Prusa3D standard
const char version_version[] = VERSION(FW_VERSION);
#endif

#ifdef _DEBUG
const char version_build[] = "build " STR(FW_BUILDNR) FW_BUILDSX " (DEBUG)";
#else
const char version_build[] = "build " STR(FW_BUILDNR) FW_BUILDSX;
#endif

//! @brief build number
//!
//! do not use FW_BUILDNR macro, as it is not defined outside of this file

const int version_build_nr = FW_BUILDNR;

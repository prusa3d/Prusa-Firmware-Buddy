// version.c

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
    "." VERSION_MIN(0) "." VERSION_SUB(3)

const char version_firmware_name[] = FWNAME;

#ifdef PRERELEASE
const char version_version[] = VERSION(FW_VERSION) "-" STR(PRERELEASE) "+" STR(FW_BUILDNR) FW_BUILDSX;
#else
const char version_version[] = VERSION(FW_VERSION);
#endif

#ifdef _DEBUG
const char version_build[] = "build " STR(FW_BUILDNR) FW_BUILDSX " (DEBUG)";
#else
const char version_build[] = "build " STR(FW_BUILDNR) FW_BUILDSX;
#endif

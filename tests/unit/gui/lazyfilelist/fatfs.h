#pragma once
#include <stdint.h>
#include <vector>
#include <string>

// fatfs compatibility layer

#define FF_USE_LFN 2
#define FF_SFN_BUF 13
#define FF_LFN_BUF 96

struct FileEntry {
    std::string lfn;
    uint16_t date, time;
    bool dir; // priznak, jestli je to adresar
};

extern std::vector<FileEntry> testFiles0;

extern "C" {

typedef int FFOBJID;
typedef uint32_t DWORD;
typedef uint32_t LBA_t;
typedef uint8_t BYTE;
typedef uint16_t WCHAR;
typedef uint64_t FSIZE_t;
typedef uint16_t WORD;
typedef char TCHAR;
typedef int FRESULT;

enum {
    AM_RDO = 1, //	Read-only. Write mode open and deleting is rejected.
    AM_HID = 2, //	Hidden. Should not be shown in normal directory listing.
    AM_SYS = 4, //	System. Used by system and should not be accessed.
    AM_ARC = 8, //	Archive. Set on new creation or any modification to the file.
    AM_DIR = 16 //	Directory. This is not a file but a sub-directory container.
};

enum {
    FR_OK,
    FR_NO_FILE
};

typedef struct {
    FFOBJID obj; /* Object identifier */
    DWORD dptr;  /* Current read/write offset */
    DWORD clust; /* Current cluster */
    LBA_t sect;  /* Current sector */
    BYTE *dir;   /* Pointer to the current SFN entry in the win[] */
    BYTE *fn;    /* Pointer to the SFN buffer (in/out) {file[8],ext[3],status[1]} */
#if FF_USE_LFN
    DWORD blk_ofs; /* Offset of the entry block (0xFFFFFFFF:Invalid) */
    WCHAR *lfn;    /* Pointer to the LFN working buffer (in/out) */
#endif
#if FF_USE_FIND
    const TCHAR *pat; /* Ponter to the matching pattern */
#endif
} DIR;

typedef struct {
    FSIZE_t fsize; /* File size */
    WORD fdate;    /* Last modified date */
    WORD ftime;    /* Last modified time */
    BYTE fattrib;  /* Attribute */
#if FF_USE_LFN
    TCHAR altname[FF_SFN_BUF + 1]; /* Alternative object name */
    TCHAR fname[FF_LFN_BUF + 1];   /* Primary object name */
#else
    TCHAR fname[12 + 1]; /* Object name */
#endif
} FILINFO;

FRESULT f_findfirst(
    DIR *dp,             /* [OUT] Poninter to the directory object */
    FILINFO *fno,        /* [OUT] Pointer to the file information structure */
    const TCHAR *path,   /* [IN] Pointer to the directory name to be opened */
    const TCHAR *pattern /* [IN] Pointer to the matching pattern string */
);

FRESULT f_findnext(
    DIR *dp,     /* [IN] Poninter to the directory object */
    FILINFO *fno /* [OUT] Pointer to the file information structure */
);

FRESULT f_closedir(
    DIR *dp /* [IN] Pointer to the directory object */
);

FRESULT f_opendir(
    DIR *dp,          /* [OUT] Poninter to the directory object */
    const TCHAR *path /* [IN] Pointer to the directory name to be opened */
);

FRESULT f_readdir(
    DIR *dp,     /* [IN] Poninter to the directory object */
    FILINFO *fno /* [OUT] Pointer to the file information structure */
);
}

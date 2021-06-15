#include "fatfs.h"
#include <vector>
#include <string.h>
#include <string>
#include <algorithm>
#include <iostream>
#include <assert.h>

// Simulated filesystem with FATfs interface

#if 0
static std::vector<FileEntry> testFiles0 = {
{ "f1.gcode", 1, false },
{ "DIR1", 2, true },
{ "f2.gc", 3, false },
{ "f3.nebude_v_seznamu", 8, false },
{ "DIR2", 7, true },
{ "f4.gCode", 6, false },
{ "f5.gC", 12, false },
{ "f6.Gcode", 14, false }
};
#endif
#if 0
static std::vector<FileEntry> testFiles0 = {
{ "posledni.gcode", 1, 0, false },
{ "fw", 2, 0, true },
{ "OPrintKanystr.gcode", 3, 0, false },
{ "default_cube.gcode", 4, 0, false },
{ "KanystrLevaPulka.gcode", 5, 0, false },
{ "Kanystr_vicko.gcode", 6, 0, false },
{ "trice_raptor.gcode", 7, 0, false },
{ "alex.gcode", 8, 0, false },
{ "alex2.gcode", 8, 1, false }
};
#endif
#if 0
static std::vector<FileEntry> testFiles0;
void FillTestFiles(){
    for(uint16_t i = 0; i < 100; ++i){
        char tmp[95];
        snprintf(tmp, 95, "%05d.g", i);
        FileEntry e = { std::string(tmp), i, 0, false };
        testFiles0.emplace_back( e );
        std::cout << testFiles0.back().lfn << std::endl;
    }

}
#endif

std::vector<FileEntry> testFiles0;

extern "C" {

/// fill FILINFO with LFN and simulate a SFN based on file's index
void MakeLFNSFN(FILINFO *fno, const char *lfn, size_t fileindex) {
    assert(fileindex < 100); // to make the SFN numeric suffix within 2 digits for now
    assert(lfn);
    assert(fno);
    strncpy(fno->fname, lfn, sizeof(fno->fname));
    if (strlen(lfn) >= FF_SFN_BUF) {
        snprintf(fno->altname, sizeof(fno->altname), "%-.6s~%02u.GCO", lfn, (unsigned)fileindex);
    } else {
        fno->altname[0] = 0;
    }
}

FRESULT f_findfirst(
    DIR *dp,                /* [OUT] Poninter to the directory object */
    FILINFO *fno,           /* [OUT] Pointer to the file information structure */
    const TCHAR * /*path*/, /* [IN] Pointer to the directory name to be opened */
    const TCHAR *pattern    /* [IN] Pointer to the matching pattern string */
) {
    if (!strcmp(pattern, "*")) {
        MakeLFNSFN(fno, testFiles0[0].lfn.c_str(), 0);
        fno->fattrib = testFiles0[0].dir ? AM_DIR : 0;
        fno->fdate = testFiles0[0].date;
        fno->ftime = testFiles0[0].time;
        dp->obj = 0;
    } else {
        auto i = std::find_if(testFiles0.begin(), testFiles0.end(), [pattern](const FileEntry &e) -> bool {
            return strcmp(pattern, e.lfn.c_str()) == 0;
        });
        if (i != testFiles0.end()) {
            MakeLFNSFN(fno, i->lfn.c_str(), std::distance(i, testFiles0.begin()));
            fno->fattrib = i->dir ? AM_DIR : 0;
            fno->fdate = i->date;
            fno->ftime = i->time;
            dp->obj = i - testFiles0.begin();
        } else {
            return FR_OK;
        }
    }
    return FR_OK;
}

FRESULT f_findnext(
    DIR *dp,     /* [IN] Poninter to the directory object */
    FILINFO *fno /* [OUT] Pointer to the file information structure */
) {
    if (dp->obj >= (int)testFiles0.size() - 1) {
        fno->fname[0] = 0;
        fno->altname[0] = 0;
    } else {
        ++dp->obj;
        MakeLFNSFN(fno, testFiles0[dp->obj].lfn.c_str(), dp->obj);
        fno->fattrib = testFiles0[dp->obj].dir ? AM_DIR : 0;
        fno->fdate = testFiles0[dp->obj].date;
        fno->ftime = testFiles0[dp->obj].time;
    }
    return FR_OK;
}

FRESULT f_opendir(
    DIR *dp,               /* [OUT] Poninter to the directory object */
    const TCHAR * /*path*/ /* [IN] Pointer to the directory name to be opened */
) {
    dp->obj = 0;
    return FR_OK;
}

FRESULT f_readdir(
    DIR *dp,     /* [IN] Poninter to the directory object */
    FILINFO *fno /* [OUT] Pointer to the file information structure */
) {
    if (dp->obj >= (int)testFiles0.size()) {
        fno->fname[0] = 0;
    } else {
        MakeLFNSFN(fno, testFiles0[dp->obj].lfn.c_str(), dp->obj);
        fno->fattrib = testFiles0[dp->obj].dir ? AM_DIR : 0;
        fno->fdate = testFiles0[dp->obj].date;
        fno->ftime = testFiles0[dp->obj].time;
        ++dp->obj;
    }
    return FR_OK;
}

FRESULT f_closedir(
    DIR * /*dp*/ /* [IN] Pointer to the directory object */
) {
    return 1;
}
}

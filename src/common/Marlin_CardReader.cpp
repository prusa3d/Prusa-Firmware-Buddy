#include "../Marlin/src/sd/cardreader.h"
#include "bsod.h"
#include "dbg.h"
#include "ff.h"
#include "ffconf.h"
#include <stdbool.h>
#include <string.h>

#define DBG _dbg

//
// cardreader's `inserted` Status
//

static bool _media_inserted = false;

extern "C" bool media_is_inserted() {
    return _media_inserted;
}

extern "C" void media_set_inserted(bool inserted) {
    _media_inserted = inserted;
    CardReader::flag.mounted = true;
}

//need to be able to force clear them
static SdFile newDir1, newDir2;

//*****************************************************************************
//Roberts hack, needed to move it up so CardReader can see it
class Slot {
public:
    enum Type : uint8_t {
        File,
        Directory,
    };

    Type type;

    union {
        FIL file;
        DIR directory;
    };

    Slot()
        : type(File)
        , file()
        , is_used(false) {
    }

    static Slot *allocate(Type type) {
        for (int i = 0; i < _pool_size; i++) {
            auto &slot = _pool[i];
            if (slot.is_used)
                continue;
            slot.type = type;
            return &slot;
        }
        return nullptr;
    }

    static void free(Slot *slot) {
        slot->is_used = false;
    }
    static void free_all() {
        for (size_t i = 0; i < Slot::_pool_size; ++i)
            _pool[i].is_used = false;
    }

private:
    bool is_used;
    static constexpr int _pool_size = 3;
    static Slot _pool[];
};

Slot Slot::_pool[Slot::_pool_size] = {};

//disable warning in following hack
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

// hack - firstCluster is used to store Slot*
#define _slot_of(x) (*((Slot **)(&x->firstCluster_)))

//*****************************************************************************

//
// Utility functions
//

/// Get a DOS 8.3 filename in its useful form
static char *createFilename(char *const buffer, const dir_t &p) {
    char *pos = buffer;
    for (uint8_t i = 0; i < 11; i++) {
        if (p.name[i] == ' ')
            continue;
        if (i == 8)
            *pos++ = '.';
        *pos++ = p.name[i];
    }
    *pos++ = 0;
    return buffer;
}

//
// CardReader class implementation
//

card_flags_t CardReader::flag;
char CardReader::filename[FILENAME_LENGTH];
char CardReader::longFilename[LONG_FILENAME_LENGTH];
SdFile CardReader::file;
SdVolume CardReader::volume;
Sd2Card CardReader::sd2card;
uint32_t CardReader::filesize;
uint32_t CardReader::sdpos;
int8_t CardReader::autostart_index;

SdFile CardReader::root, CardReader::workDir,
    CardReader::workDirParents[MAX_DIR_DEPTH];
uint8_t CardReader::workDirDepth;

uint8_t CardReader::file_subcall_ctr;
uint32_t CardReader::filespos[SD_PROCEDURE_DEPTH];
char CardReader::proc_filenames[SD_PROCEDURE_DEPTH][MAXPATHNAMELENGTH];

CardReader::CardReader() {
    workDir = root;
    flag.sdprinting = flag.mounted = flag.saving = flag.logging = false;
    filesize = sdpos = 0;
    file_subcall_ctr = 0;
    ZERO(workDirParents);

    // Disable autostart until card is initialized
    autostart_index = -1;
}

void CardReader::openLogFile(char *const path) {
    DBG("NOT IMPLEMENTED: openLogFile(...)");
}

void CardReader::startFileprint() {
    if (isMounted())
        flag.sdprinting = true;
}

void CardReader::printingHasFinished() {
    if (isFileOpen())
        file.close();
    if (file_subcall_ctr > 0) {
        // Heading up to a parent file that called current as a procedure.
        file_subcall_ctr--;
        openFile(proc_filenames[file_subcall_ctr], true, true);
        setIndex(filespos[file_subcall_ctr]);
        startFileprint();
    } else {
        stopSDPrint();
    }
}

void CardReader::stopSDPrint() {
    flag.sdprinting = flag.abort_sd_printing = false;
    if (isFileOpen())
        file.close();

    Slot::free_all();
    workDir = root;
    newDir1 = root;
    newDir2 = root;
}

void CardReader::release() {
    // Nothing to do here. Releasing is done automatically outside of Marlin.
}

void CardReader::mount() {
    // Nothing to do here. We handle mounting automatically outside of Marlin.
}

void CardReader::getAbsFilename(char *dst) {
    *dst++ = '/';
    uint8_t cnt = 1;

    auto appendAtom = [&](SdFile &file) {
        file.getDosName(dst);
        while (*dst && cnt < MAXPATHNAMELENGTH) {
            dst++;
            cnt++;
        }
        if (cnt < MAXPATHNAMELENGTH) {
            *dst = '/';
            dst++;
            cnt++;
        }
    };

    for (uint8_t i = 0; i < workDirDepth; i++) // Loop down to current work dir
        appendAtom(workDirParents[i]);

    if (cnt < MAXPATHNAMELENGTH - (FILENAME_LENGTH)-1) { // Leave room for filename and nul
        appendAtom(file);
        --dst;
    }
    *dst = '\0';
}

/**
 * Dive to the given DOS 8.3 file path, with optional echo of the dive paths.
 *
 * On exit, curDir contains an SdFile reference to the file's directory.
 *
 * Returns a pointer to the last segment (filename) of the given DOS 8.3 path.
 *
 * A nullptr result indicates an unrecoverable error.
 */
const char *CardReader::diveToFile(SdFile *&curDir, const char *const path,
    const bool echo /*=false*/) {
    // Track both parent and subfolder
    SdFile *sub = &newDir1, *startDir;

    const char *item_name_adr = path;
    if (path[0] == '/') {
        curDir = &root;
        workDirDepth = 0;
        item_name_adr++;
    } else
        curDir = &workDir;

    startDir = curDir;

    // Start dive
    while (item_name_adr) {
        // Find next sub
        char *const name_end = strchr(item_name_adr, '/');
        if (name_end <= item_name_adr)
            break;

        // Set subDirName
        const uint8_t len = name_end - item_name_adr;
        char dosSubdirname[len + 1];
        strncpy(dosSubdirname, item_name_adr, len);
        dosSubdirname[len] = 0;

        if (echo)
            SERIAL_ECHOLN(dosSubdirname);

        // Open curDir
        if (!sub->open(curDir, dosSubdirname, O_READ)) {
            SERIAL_ECHOLNPAIR(MSG_SD_OPEN_FILE_FAIL, dosSubdirname, ".");
            return nullptr;
        }

        // Close curDir if not at starting-point
        if (curDir != startDir)
            curDir->close();

        // curDir now subDir
        curDir = sub;

        // Update workDirParents and workDirDepth
        if (workDirDepth < MAX_DIR_DEPTH)
            workDirParents[workDirDepth++] = *curDir;

        // Point sub pointer to unused newDir
        sub = (curDir != &newDir1) ? &newDir1 : &newDir2;

        // item_name_adr point to next sub
        item_name_adr = name_end + 1;
    }
    return item_name_adr;
}

void CardReader::openFile(char *const path, const bool read, bool subcall) {
    if (!isMounted())
        return;

    uint8_t doing = 0;
    if (isFileOpen()) { // Replacing current file or doing a subroutine
        if (subcall) {
            if (file_subcall_ctr > SD_PROCEDURE_DEPTH - 1) {
                bsod("sub-gcode openning too many files");
                return;
            }

            // Store current filename (based on workDirParents) and position
            getAbsFilename(proc_filenames[file_subcall_ctr]);
            filespos[file_subcall_ctr] = sdpos;

            SERIAL_ECHO_START();
            SERIAL_ECHOLNPAIR("SUBROUTINE CALL target:\"", path, "\" parent:\"",
                proc_filenames[file_subcall_ctr], "\" pos",
                sdpos);
            file_subcall_ctr++;
        } else {
            doing = 1;
        }
    } else if (subcall) {
        // Returning from a subcall?
        SERIAL_ECHO_MSG("END SUBROUTINE");
    } else {
        // Opening fresh file
        doing = 2;
        file_subcall_ctr = 0; // Reset procedure depth in case user cancels
                              // print while in procedure
    }

    if (doing) {
        SERIAL_ECHO_START();
        SERIAL_ECHOPGM("Now ");
        serialprintPGM(doing == 1 ? PSTR("doing") : PSTR("fresh"));
        SERIAL_ECHOLNPAIR(" file: ", path);
    }

    stopSDPrint();

    SdFile *curDir;
    const char *const fname = diveToFile(curDir, path);
    if (!fname)
        return;

    if (read) {
        if (file.open(curDir, fname, O_READ)) {
            filesize = file.fileSize();
            sdpos = 0;
            SERIAL_ECHOLNPAIR(MSG_SD_FILE_OPENED, fname, MSG_SD_SIZE, filesize);
            SERIAL_ECHOLNPGM(MSG_SD_FILE_SELECTED);

            selectFileByName(fname);
        } else {
            SERIAL_ECHOLNPAIR(MSG_SD_OPEN_FILE_FAIL, fname, ".");
        }
    } else {
        // write
        if (!file.open(curDir, fname, O_CREAT | O_APPEND | O_WRITE | O_TRUNC)) {
            SERIAL_ECHOLNPAIR(MSG_SD_OPEN_FILE_FAIL, fname, ".");
        } else {
            flag.saving = true;
            selectFileByName(fname);
#if ENABLED(EMERGENCY_PARSER)
            emergency_parser.disable();
#endif
            SERIAL_ECHOLNPAIR(MSG_SD_WRITE_TO_FILE, fname);
        }
    }
}

void CardReader::selectFileByName(const char *const match) {
    workDir.rewind();
    selectByName(workDir, match);
}

//
// Return 'true' if the item is a folder or G-code file
//
bool CardReader::is_dir_or_gcode(const dir_t &p) {
    uint8_t pn0 = p.name[0];

    if (pn0 == DIR_NAME_FREE || pn0 == DIR_NAME_DELETED // Clear or Deleted entry
        || pn0 == '.' || longFilename[0] == '.'         // Hidden file
        || !DIR_IS_FILE_OR_SUBDIR(&p)                   // Not a File or Directory
        || (p.attributes & DIR_ATT_HIDDEN)              // Hidden by attribute
    )
        return false;

    flag.filenameIsDir = DIR_IS_SUBDIR(&p); // We know it's a File or Folder

    return (flag.filenameIsDir                    // All Directories are ok
        || (p.name[8] == 'G' && p.name[9] != '~') // Non-backup *.G* files are accepted
    );
}

void CardReader::selectByName(SdFile dir, const char *const match) {
    dir_t p;
    for (uint8_t cnt = 0; dir.readDir(&p, longFilename) > 0; cnt++) {
        if (is_dir_or_gcode(p)) {
            createFilename(filename, p);
            if (strcasecmp(match, filename) == 0)
                return;
        }
    }
}

void CardReader::closefile(const bool store_location) {
    DBG("NOT IMPLEMENTED: closeFile(...)");
}

void CardReader::removeFile(const char *const name) {
    DBG("NOT IMPLEMENTED: removeFile(...)");
}

void CardReader::report_status() {
    DBG("NOT IMPLEMENTED: report_status(...)");
}

void CardReader::printFilename() {
    DBG("NOT IMPLEMENTED: printFilename(...)");
}

void CardReader::ls() {
    DBG("NOT IMPLEMENTED: ls()");
}

void CardReader::beginautostart() {
    DBG("NOT IMPLEMENTED: beginautostart()");
}

void CardReader::checkautostart() {
}

void CardReader::write_command(char *const buf) {
    DBG("NOT IMPLEMENTED: write_command(...)");
}

//
// SdBaseFile class implementation
//

static void fname_from_8_3(char *fn, char *fn83) {
    char *p;
    strncpy(fn, fn83, 11);
    fn[11] = 0;
    p = strchr(fn, ' ');
    if (fn83[8] != ' ') {
        if (p) {
            *p = '.';
            p++;
            strncpy(p, fn83 + 8, 3);
            p = strchr(p, ' ');
            if (p)
                *p = 0;
        } else {
            strncpy(fn + 9, fn83 + 8, 3);
            fn[8] = '.';
        }
    } else {
        *p = 0;
    }
}

bool SdBaseFile::getDosName(char *const name) {
    if (!isOpen())
        return false;
    if (isRoot()) {
        name[0] = '/';
        name[1] = '\0';
        return true;
    }
    if (isDir()) {
        DIR *pd = &_slot_of(this)->directory;
        fname_from_8_3(name, (char *)pd->fn);
        return true;
    } else {
        // TODO: fix
        bsod("get dos name of a file");
    }
    return false;
}

int8_t SdBaseFile::readDir(dir_t *dir, char *longFilename) {
    // TODO: cleanup (copied from previous implementation)
    FRESULT res;
    FILINFO nfo;
    char *str;
#if _USE_LFN != 0
    char *altname = nfo.altname;
    char *fname = nfo.fname;
#else
    char *altname = nfo.fname;
    // char* fname = 0;
#endif
    if (longFilename)
        longFilename[0] = '\0';
    while (1) {
        res = f_readdir(&_slot_of(this)->directory, &nfo);
        if (res != FR_OK)
            return -1;
        memset(dir->name, ' ', 11);
        str = strchr(altname, '.');
        if (str) {
            strncpy(((char *)dir->name) + 0, altname, str - altname);
            strncpy(((char *)dir->name) + 8, str + 1, 3);
        } else {
            strncpy(((char *)dir->name) + 0, altname, 8);
        }
        if (dir->name[0] == DIR_NAME_FREE)
            return 0;
        if (dir->name[0] == DIR_NAME_DELETED || dir->name[0] == '.') {
            //			if (longFilename) longFilename[0] = '\0';     //
            // Invalidate erased file long name, if any
            continue;
        }
#if _USE_LFN != 0
        if (longFilename)
            strncpy(longFilename, fname, _MAX_LFN);
#endif
        dir->attributes = nfo.fattrib;
        dir->lastWriteDate = nfo.fdate;
        dir->lastWriteTime = nfo.ftime;
        dir->fileSize = nfo.fsize;
        return sizeof(dir_t);
    }
    return 0;
}

bool SdBaseFile::seekSet(const uint32_t pos) {
    if ((pos == 0) && ((type_ == FAT_FILE_TYPE_ROOT_FIXED) || (type_ == FAT_FILE_TYPE_ROOT32) || (type_ == FAT_FILE_TYPE_SUBDIR))) {
        return f_rewinddir(&_slot_of(this)->directory) == FR_OK;
    }
    if (type_ != FAT_FILE_TYPE_NORMAL)
        return false;
    FSIZE_t szf = pos;
    if (f_lseek(&_slot_of(this)->file, szf) != FR_OK)
        return false;
    curPosition_ = pos;
    return true;
}

int16_t SdBaseFile::read(void *buf, uint16_t nbyte) {
    DBG("NOT IMPLEMENTED: read(...)");
    return 0;
}

int16_t SdBaseFile::read() {
    if (type_ != FAT_FILE_TYPE_NORMAL)
        return -1;

    uint8_t b;
    UINT c;
    if (f_read(&_slot_of(this)->file, &b, 1, &c) != FR_OK)
        return -1;
    curPosition_++;
    return b;
}

bool SdBaseFile::close() {
    Slot *slot;
    bool success = false;

    if (!isOpen())
        return false;
    switch (type_) {
    case FAT_FILE_TYPE_NORMAL:
        slot = _slot_of(this);
        success = f_close(&slot->file) == FR_OK;
        Slot::free(slot);
        _slot_of(this) = nullptr;
        break;
    case FAT_FILE_TYPE_ROOT_FIXED:
    case FAT_FILE_TYPE_ROOT32:
    case FAT_FILE_TYPE_SUBDIR:
        slot = _slot_of(this);
        success = f_closedir(&slot->directory) == FR_OK;
        Slot::free(slot);
        _slot_of(this) = nullptr;
        break;
    default:
        return false;
    }

    type_ = FAT_FILE_TYPE_CLOSED;
    return success;
}

bool SdBaseFile::open(SdBaseFile *dir, const char *path, uint8_t oflag) {
    if (!dir || isOpen())
        return false;

    // prepare full path
    char fullpath[256] = { 0 };
    if (dir->isRoot()) {
        strcat(fullpath, "/");
        strcat(fullpath, path);
    } else {
        auto dirp = &(_slot_of(dir)->directory);
        f_getdirpath(dirp, fullpath, sizeof(fullpath));
        strcat(fullpath, "/");
        strcat(fullpath, path);
    }

    // get file info
    FILINFO nfo;
    if (f_stat(fullpath, &nfo) != FR_OK)
        return false;

    if (nfo.fattrib & AM_DIR) {
        auto slot = Slot::allocate(Slot::Directory);
        if (!slot)
            return false;

        if (f_opendir(&slot->directory, path) == FR_OK) {
            _slot_of(this) = slot;
            type_ = FAT_FILE_TYPE_SUBDIR;
            curPosition_ = 0;
            fileSize_ = f_size(&slot->directory);
            return true;
        } else {
            Slot::free(slot);
            return false;
        }
    } else {
        auto slot = Slot::allocate(Slot::File);
        if (!slot)
            return false;

        BYTE mode = 0;
        if (oflag & O_READ)
            mode |= FA_READ;
        if (oflag & O_WRITE)
            mode |= FA_WRITE;

        if (f_open(&slot->file, fullpath, mode) == FR_OK) {
            _slot_of(this) = slot;
            type_ = FAT_FILE_TYPE_NORMAL;
            curPosition_ = 0;
            fileSize_ = f_size(&slot->file);
            return true;
        } else {
            Slot::free(slot);
            return false;
        }
    }
    return true;
}

//
// Sd2Card class implementation
//

bool Sd2Card::isInserted() {
    return media_is_inserted();
}

void Sd2Card::idle() {
}

//restore state - enable warnings
#pragma GCC diagnostic pop

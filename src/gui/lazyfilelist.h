#pragma once
#include <memory>
#include <algorithm>
#include "fatfs.h"
#include <string.h>
#include <limits.h>
#include "file_list_defs.h"

/// Lazy Dir View
/// Implements a fixed size view over a directory's content.
///
/// The entries are sorted according to one of the selected policies: BY_NAME, BY_CRMOD_DATETIME
/// Sorting is always done directories-first. Directories may be also sorted by name or by their CR/MOD datetime
///
/// The main benefits of this implementation over the previous one are:
/// - 8KB less RAM consumption
/// - unlimited number of dir entries supported
/// - start from a given filename to be displayed first - support for restoring file browser's exact content when returning from One Click Print
template <size_t WINDOW_SIZE>
class LazyDirView {
public:
    /// Types of directory entries
    enum class EntryType : uint8_t {
        FILE,
        DIR
    };

    /// Entries sort policies
    enum class SortPolicy : uint8_t {
        BY_NAME,
        BY_CRMOD_DATETIME ///< Sort by combined Creation and Modification time stamp.
                          ///< Expects support in FATfs - to return the most recent time stamp from both of them
    };

    LazyDirView()
        : totalFiles(0)
        , windowStartingFrom(0) {
        std::for_each(files.begin(), files.end(), [](Entry &e) { e.Clear(); });
    }

    /// @param windowIndex index within the window (i.e. [0 - WINDOW_SIZE-1])
    /// @return pointer to a filename (or nullptr, if there is nothing at that index) and the type of the entry (FILE / DIR)
    std::pair<const char *, EntryType> FileNameAt(size_t windowIndex) {
        return std::make_pair(files[windowIndex].name, files[windowIndex].isFile ? EntryType::FILE : EntryType::DIR);
    }

    /// @return total number of files/entries in a directory
    size_t TotalFilesCount() const { return totalFiles; }

    /// @return number of visible files/entries in a directory supported by this instance
    size_t VisibleFilesCount() const { return WINDOW_SIZE; }

    /// Initial population of the window + sorting is a normal insert sort algorithm with one run through the directory.
    /// Much of the comparison is done in RAM while avoiding slower USB/FATFS interface as much possible.
    ///
    /// Future implementation consideration for sorting policy:
    /// - compile time - making ChangeDirectory a template: +faster -more code size
    /// - run time - sort policy as a parameter: -slower +less code size
    ///
    /// @param p directory path (will be remembered inside this class)
    /// @param firstDirEntry filename to be placed at the 0th index of the window - the rest is computed accordingly - useful for restoring the view's content.
    ///                      A value of nullptr means put ".." first
    void ChangeDirectory(const char *p, SortPolicy sp = SortPolicy::BY_NAME, const char *firstDirEntry = nullptr) {
        size_t filesInWindow = 0; // number of files populated in the window - less than WINDOW_SIZE when there are less files in the dir
        totalFiles = 0;
        sortPolicy = sp;
        strlcpy(path, p, sizeof(path));
        if (!firstDirEntry) {
            files[0].SetDirUp(); // this is always the first (zeroth) one
            windowStartingFrom = -1;
        } else {
            // find the file in the directory using pattern search
            F_DIR_RAII_Find_One dir(path, firstDirEntry);
            if (dir.result != FR_OK || dir.fno.fname[0] == 0) {
                // the filename was not found, discard the firstDirEntry and start from the beginning
                // of the directory like if firstDirEntry was nullptr
                files[0].SetDirUp();
                windowStartingFrom = -1;
            } else {
                files[0].CopyFrom(dir.fno);
                // windowStartsFrom will be fine tuned later during iteration over the whole dir content
                // And the dir must closed here, because the search cycle uses a different search pattern
            }
        }

        switch (sortPolicy) {
        case SortPolicy::BY_NAME:
            LessEF = &LessByFNameEF;
            LessFE = &LessByFNameFE;
            MakeFirstEntry = &MakeFirstEntryByFName;
            MakeLastEntry = &MakeLastEntryByFName;
            break;
        case SortPolicy::BY_CRMOD_DATETIME:
            LessEF = &LessByTimeEF;
            LessFE = &LessByTimeFE;
            MakeFirstEntry = &MakeFirstEntryByTime;
            MakeLastEntry = &MakeLastEntryByTime;
            break;
        };

        // we now have at least one entry - either ".." or the file firstDirEntry
        ++filesInWindow;
        ++totalFiles;
        F_DIR_RAII_Iterator dir(path);
        while (dir.FindNext()) {
            // Find the right stop to insert the file/entry - a normal binary search algorithm
            // Searching is done from the first (not zeroth) index, because the zeroth must be kept intact - that's the start of our window
            // Impl. detail: cannot use auto, need the write iterator (non const)
            typename decltype(files)::iterator i = std::upper_bound(files.begin() + 1, files.begin() + filesInWindow, dir.fno, LessFE);
            if (i == files.end()) {
                // The file entry would have been inserted outside of the window - ignore
                // However, if it is less than the zeroth entry, we must increment windowStartsFrom
                if (LessFE(dir.fno, files[0])) {
                    ++windowStartingFrom;
                }
            } else {
                if (strcmp(files[0].name, dir.fno.fname) != 0) {
                    // i.e. we didn't get the same entry as the zeroth entry (which may occur when populating the window with non-null firstDirEntry)
                    // Make place in the window by standard item rotation downwards (to the right)
                    std::rotate(files.rbegin(), files.rbegin() + 1, std::make_reverse_iterator(i)); // resi i pripad, ze tech fajlu je zatim mene
                    // Save the entry
                    i->CopyFrom(dir.fno);
                    if (filesInWindow < WINDOW_SIZE) {
                        ++filesInWindow;
                    }
                }
            }
            ++totalFiles;
        }
    }

    /// This is what a user does with the knob - moves the window by 1 item up or down if the cursor is at the top end.
    /// It rotates the items downwards (to the right) and computes the new top missing entry
    /// by iterating through the directory's content (which is reading one or more FAT sectors).
    /// @return true if the window was actually moved (i.e. not at the very beginning of the dir)
    bool MoveUp() {
        if (windowStartingFrom < 0)
            return false;

        std::rotate(files.rbegin(), files.rbegin() + 1, files.rend());

        if (windowStartingFrom == 0) {
            // special case code - add a ".."
            files[0].SetDirUp();
            --windowStartingFrom;
            return true;
        }

        F_DIR_RAII_Iterator dir(path);
        // prepare the item at the zeroth position according to sort policy
        files[0] = MakeFirstEntry();
        while (dir.FindNext()) {
            if (LessEF(files[0], dir.fno) && LessFE(dir.fno, files[1])) {
                // to be inserted, the entry must be greater than zeroth entry AND less than the first entry
                files[0].CopyFrom(dir.fno);
            }
        }
        --windowStartingFrom;
        return true;
    }

    /// A similar operation like MoveUp, but moves the window down by 1 item.
    /// @return true if the window was actually moved
    bool MoveDown() {
        if (totalFiles <= windowStartingFrom + WINDOW_SIZE + 1) {
            return false; // no more files
        }
        std::rotate(files.begin(), files.begin() + 1, files.end());

        F_DIR_RAII_Iterator dir(path);
        // prepare the last item according to sort policy
        files[WINDOW_SIZE - 1] = MakeLastEntry();
        while (dir.FindNext()) {
            if (LessFE(dir.fno, files[WINDOW_SIZE - 1]) && LessEF(files[WINDOW_SIZE - 2], dir.fno)) {
                // to be inserted, the entry must be greater than the pre-last entry AND less than the last entry
                files[WINDOW_SIZE - 1].CopyFrom(dir.fno);
            }
        }
        ++windowStartingFrom;
        return true;
    }

private:
    struct Entry {
        bool isFile;
        char name[_MAX_LFN];
        uint16_t date, time;
        void Clear() {
            isFile = false;
            name[0] = 0;
            date = time = 0;
        }
        void CopyFrom(const FILINFO &fno) {
            strlcpy(name, fno.fname, sizeof(name));
            isFile = (fno.fattrib & AM_DIR) == 0;
            date = fno.fdate;
            time = fno.ftime;
        }
        void SetDirUp() {
            name[0] = name[1] = '.';
            name[2] = 0;
            isFile = false;
            date = time = UINT16_MAX;
        }
    };

    std::array<Entry, WINDOW_SIZE> files; ///< roughly WINDOW_SIZE * 100B, can be placed in CCRAM if needed. For MINI it is only 9 entries -> only 900B
    size_t totalFiles;                    ///< total number of entries in the directory
    int windowStartingFrom;               ///< from which entry index the window starts (e.g. from the 3rd file in dir).
                                          ///< intentionally int, because -1 means ".."
    char path[F_MAXPATHNAMELENGTH];       ///< current directory path - @@TODO this may not be enough - needs checking
    SortPolicy sortPolicy;                ///< sort policy set in ChangeDirectory - @@TODO probably not needed at runtime

    /// Current selected sort policy compare functions
    /// Could have been some std::function or some other advanced c++ method, but KISS ;)
    bool (*LessEF)(const Entry &, const FILINFO &);
    bool (*LessFE)(const FILINFO &, const Entry &);
    Entry (*MakeFirstEntry)();
    Entry (*MakeLastEntry)();

    /// This is just a simple RAII struct for finding one particular file/dir name using FATfs tools
    /// and closing the control structures accordingly
    struct F_DIR_RAII_Find_One {
        DIR dp;
        FILINFO fno;
        int result;
        F_DIR_RAII_Find_One(const char *path, const char *pattern) {
            result = f_findfirst(&dp, &fno, path, pattern);
        }
        ~F_DIR_RAII_Find_One() {
            f_closedir(&dp);
        }
    };

    /// RAII iterator structure over a directory with file/entry matching
    /// tailored for our purposes
    struct F_DIR_RAII_Iterator {
        DIR dp;
        FILINFO fno;
        int result;
        F_DIR_RAII_Iterator(const char *path) {
            result = f_opendir(&dp, path);
        }

        static bool FnameMatchesPattern(const char *fname) {
            size_t len = strlen(fname);
            static const char gcode[] = ".gcode";
            static const char gc[] = ".gc";
            static const char g[] = ".g";

            if (!strcasecmp(fname + len - sizeof(gcode) + 1, gcode))
                return true;
            if (!strcasecmp(fname + len - sizeof(gc) + 1, gc))
                return true;
            if (!strcasecmp(fname + len - sizeof(g) + 1, g))
                return true;
            return false;
        }

        /// @return true if a file/entry matching the requirements was found
        ///         false if there are no more files or an error iterating over a dir occured
        bool FindNext() {
            // f_find_next internally calls only f_readdir
            // and it only does pattern matching, which I don't need here - I have my own
            while ((result = f_readdir(&dp, &fno)) == FR_OK) {
                // idiotic API of f_readdir and f_findnext - it returns FR_OK even when there are no more files
                if (fno.fname[0] == 0) {
                    return false;
                }
                if (EntryAccepted()) {
                    return true; // found and accepted
                }
            }
            return false; // an error iterating over the dir
        }

        bool EntryAccepted() const {
            if ((fno.fattrib & (AM_SYS | AM_HID)) != 0) {
                return false; // system and hidden files/directories are not accepted
            }
            if ((fno.fattrib & AM_DIR) != 0) {
                return true; // all normal directories are accepted
            }
            // files are being filtered by their extension
            // this is vastly less code than generic patter_matching from FATFS
            return FnameMatchesPattern(fno.fname);
        }

        ~F_DIR_RAII_Iterator() {
            f_closedir(&dp);
        }
    };

    struct string_view_light {
        const char *s;
        inline string_view_light(const char *s)
            : s(s) {}
        inline bool operator<(const string_view_light &s2) const {
            return strcmp(s, s2.s) < 0;
        }
    };

    // These function must not be named the same, otherwise one would have to explicitely
    // specify the correct one in the upper_bound algoritm which looks horrible :)
    // Using std::tie to avoid errors in comparison of a heterogenous sequence of components
    static bool LessByFNameEF(const Entry &e, const FILINFO &fno) {
        string_view_light fnoName(fno.fname);
        string_view_light eName(e.name);
        bool fnoIsFile = (fno.fattrib & AM_DIR) == 0;
        return std::tie(e.isFile, eName) < std::tie(fnoIsFile, fnoName);
    }
    static bool LessByFNameFE(const FILINFO &fno, const Entry &e) {
        string_view_light fnoName(fno.fname);
        string_view_light eName(e.name);
        bool fnoIsFile = (fno.fattrib & AM_DIR) == 0;
        return std::tie(fnoIsFile, fnoName) < std::tie(e.isFile, eName);
    }

    static Entry MakeFirstEntryByFName() {
        static const Entry e = { false, "", 0U, 0U };
        return e;
    }
    static Entry MakeLastEntryByFName() {
        Entry e = { true, "", 0xffff, 0xffff };
        std::fill(e.name, e.name + sizeof(e.name) - 1, 0xff);
        e.name[sizeof(e.name) - 1] = 0;
        return e;
    }
    // This may look confusing - from the sorting perspective, the higher time stamp (the more recent one)
    // is less than an older time stamp - we want the newer files up higher in the list.
    // Therefore the condition must be inverted including the comparison sequence parameter meaning
    // -> directories first and then the most recent files
    static bool LessByTimeEF(const Entry &e, const FILINFO &fno) {
        bool fnoIsDir = (fno.fattrib & AM_DIR) != 0;
        bool eIsDir = !e.isFile;
        // beware - multiple files may have identical time stamps!
        // In such case, the file name is the only unique identifier and thus must be included in the comparison
        string_view_light fnoName(fno.fname);
        string_view_light eName(e.name);
        return std::tie(fnoIsDir, fno.fdate, fno.ftime, fnoName) < std::tie(eIsDir, e.date, e.time, eName);
    }
    static bool LessByTimeFE(const FILINFO &fno, const Entry &e) {
        bool fnoIsDir = (fno.fattrib & AM_DIR) != 0;
        bool eIsDir = !e.isFile;
        string_view_light fnoName(fno.fname);
        string_view_light eName(e.name);
        return std::tie(eIsDir, e.date, e.time, eName) < std::tie(fnoIsDir, fno.fdate, fno.ftime, fnoName);
    }
    static Entry MakeFirstEntryByTime() {
        Entry e = { false, "", 0xffff, 0xffff };
        std::fill(e.name, e.name + sizeof(e.name) - 1, 0xff);
        e.name[sizeof(e.name) - 1] = 0;
        return e;
    }
    static Entry MakeLastEntryByTime() {
        static const Entry e = { true, "", 0U, 0U };
        return e;
    }
};

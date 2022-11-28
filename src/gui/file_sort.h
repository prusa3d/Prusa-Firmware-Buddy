/**
 * @file file_sort.h
 * @brief File sort
 * Defines some file sort algorithms excluded from LazyDirView to be able to use it elsewhere
 * also does not require template argument
 */

#pragma once
#include <memory>
#include <iterator>
#include <algorithm>
#include <string.h>
#include <limits.h>
#include <array>
#include <stdio.h>
#include <dirent.h>
#include "file_raii.hpp"
#include "../../src/gui/file_list_defs.h"
#ifdef LAZYFILELIST_UNITTEST
extern "C" size_t strlcpy(char *dst, const char *src, size_t dsize);
extern "C" int strcasecmp(const char *a, const char *b); // strcasecmp defined weakly in unit tests to be able to compile them on windows
#endif

class FileSort {
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
    };

    static constexpr size_t MAX_SFN = 13;
    struct Entry {
        bool isFile;
        char lfn[FF_MAX_LFN];
        char sfn[FileSort::MAX_SFN]; // cache the short filenames too, since they will be used in communication with Marlin
        uint64_t time;
        void Clear() {
            isFile = false;
            lfn[0] = 0;
            sfn[0] = 0;
            time = 0;
        }
        void ReplaceNonAsciiChars(size_t charsCopied) {
            for (char *p = lfn; p != lfn + charsCopied; ++p) {
                if (*p < 32 || *p > 127) {
                    *p = '*';
                }
            }
        }
        void CopyFrom(const dirent *fno) {
            strlcpy(sfn, fno->d_name, sizeof(sfn));
            strlcpy(lfn, fno->lfn, sizeof(lfn));
            // Safety precautions - remove all non-ascii characters from the LFN, which are not in range <32-127>
            // and replace them with a placeholder - in our case a '*'
            // @@TODO beware: if someone deliberately makes 2 filenames differing only in one non-ascii character,
            // this may break the whole file sorting - because both filenames will have a '*' at that index,
            // i.e. the file names will not be unique. In such a case the list of files may be incomplete or work
            // incorrectly. But remember - we do not support diacritics in filenames AT ALL.
            // 9. 8. 2021
            // not needed anymore non-ascii characters are replaced with "?" by fix in FAT fs
            //            ReplaceNonAsciiChars(charsCopied);

            isFile = (fno->d_type & DT_REG) != 0;
            time = fno->time;
        }
        void SetDirUp() {
            lfn[0] = lfn[1] = sfn[0] = sfn[1] = '.';
            lfn[2] = sfn[2] = 0;
            isFile = false;
            time = UINT_LEAST64_MAX;
        }
    };

    struct string_view_light {
        const char *s;
        inline string_view_light(const char *s)
            : s(s) {}
        inline bool operator<(const string_view_light &s2) const {
            return strcasecmp(s, s2.s) < 0;
        }
    };

    // function pointer aliases
    using LessEF_t = bool (*)(const Entry &, const dirent *);
    using LessFE_t = bool (*)(const dirent *, const Entry &);
    using MakeEntry_t = Entry (*)();

    // These function must not be named the same, otherwise one would have to explicitely
    // specify the correct one in the upper_bound algoritm which looks horrible :)
    // Using std::tie to avoid errors in comparison of a heterogenous sequence of components
    static bool LessByFNameEF(const Entry &e, const dirent *fno) {
        string_view_light fnoName(fno->lfn);
        string_view_light eName(e.lfn);
        bool fnoIsFile = (fno->d_type & DT_REG) != 0;
        return std::tie(e.isFile, eName) < std::tie(fnoIsFile, fnoName);
    }
    static bool LessByFNameFE(const dirent *fno, const Entry &e) {
        string_view_light fnoName(fno->lfn);
        string_view_light eName(e.lfn);
        bool fnoIsFile = (fno->d_type & DT_REG) != 0;
        return std::tie(fnoIsFile, fnoName) < std::tie(e.isFile, eName);
    }

    static Entry MakeFirstEntryByFName() {
        static const Entry e = { false, "", "", 0U };
        return e;
    }
    static Entry MakeLastEntryByFName() {
        Entry e = { true, "", "", 0xffff };
        std::fill(e.lfn, e.lfn + sizeof(e.lfn) - 1, 0xff);
        e.lfn[sizeof(e.lfn) - 1] = 0;
        return e;
    }

    // This may look confusing - from the sorting perspective, the higher time stamp (the more recent one)
    // is less than an older time stamp - we want the newer files up higher in the list.
    // Therefore the condition must be inverted including the comparison sequence parameter meaning
    // -> directories first and then the most recent files
    static bool LessByTimeEF(const Entry &e, const dirent *fno) {
        bool fnoIsDir = (fno->d_type & DT_DIR) != 0;
        bool eIsDir = !e.isFile;
        // beware - multiple files may have identical time stamps!
        // In such case, the file name is the only unique identifier and thus must be included in the comparison
        string_view_light fnoName(fno->lfn);
        string_view_light eName(e.lfn);
        return std::tie(fnoIsDir, fno->time, fnoName) < std::tie(eIsDir, e.time, eName);
    }
    static bool LessByTimeFE(const dirent *fno, const Entry &e) {
        bool fnoIsDir = (fno->d_type & DT_DIR) != 0;
        bool eIsDir = !e.isFile;
        string_view_light fnoName(fno->lfn);
        string_view_light eName(e.lfn);
        bool res = std::tie(eIsDir, e.time, eName) < std::tie(fnoIsDir, fno->time, fnoName);
        return res;
    }
    static Entry MakeFirstEntryByTime() {
        Entry e = { false, "", "", UINT_LEAST64_MAX };
        std::fill(e.lfn, e.lfn + sizeof(e.lfn) - 1, 0xff);
        e.lfn[sizeof(e.lfn) - 1] = 0;
        // since the sfn is not used for comparison anywhere in LazyDirView,
        // its initialization may be skipped here to save some code
        //        std::fill(e.sfn, e.sfn + sizeof(e.sfn) - 1, 0xff);
        //        e.sfn[sizeof(e.sfn) - 1] = 0;
        return e;
    }
    static Entry MakeLastEntryByTime() {
        static const Entry e = { true, "", "", 0U };
        return e;
    }

    /// Define our own make_reverse_iterator for backward compatibility with c++11
    template <class Iter>
    static constexpr std::reverse_iterator<Iter> make_reverse_iterator(Iter i) {
        return std::reverse_iterator<Iter>(i);
    }
};

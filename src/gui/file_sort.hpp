/**
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
#include "mutable_path.hpp"

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

    struct DirentWPath {
        DirentWPath(const dirent &d, const MutablePath &full_path)
            : d(d)
            , full_path(full_path) {}
        const dirent &d;
        const MutablePath &full_path;
    };

    static constexpr size_t MAX_SFN = 13;
    struct Entry {
        bool isFile;
        char lfn[FF_MAX_LFN];
        char sfn[FileSort::MAX_SFN]; // cache the short filenames too, since they will be used in communication with Marlin
        uint64_t time;
        void Clear();
        void CopyFrom(const DirentWPath &dwp);
        void SetDirUp();
    };

    struct string_view_light {
        const char *s;
        inline string_view_light(const char *s)
            : s(s) {}
        inline bool operator<(const string_view_light &s2) const {
            return strcasecmp(s, s2.s) < 0;
        }
    };

    struct TimeComparator {
        TimeComparator(const Entry &e);
        TimeComparator(const DirentWPath &dwp);

        bool is_dir;
        uint64_t time;
        string_view_light lfn_name;

        // Using std::tie to avoid errors in comparison of a heterogenous sequence of components
        inline bool operator<(const TimeComparator &other) const {
            return std::tie(is_dir, time, lfn_name) < std::tie(other.is_dir, other.time, other.lfn_name);
        }
    };

    struct NameComparator {
        NameComparator(const Entry &e);
        NameComparator(const DirentWPath &dwp);

        bool is_file;
        string_view_light lfn_name;

        // Using std::tie to avoid errors in comparison of a heterogenous sequence of components
        inline bool operator<(const NameComparator &other) const {
            return std::tie(is_file, lfn_name) < std::tie(other.is_file, other.lfn_name);
        }
    };

    // function pointer aliases
    using LessEF_t = bool (*)(const Entry &, const DirentWPath &);
    using LessFE_t = bool (*)(const DirentWPath &, const Entry &);
    using MakeEntry_t = Entry (*)();

    // These function must not be named the same, otherwise one would have to explicitely
    // specify the correct one in the upper_bound algoritm which looks horrible :)
    inline static bool LessByFNameEF(const Entry &e, const DirentWPath &dwp) {
        return NameComparator { e } < NameComparator { dwp };
    }
    inline static bool LessByFNameFE(const DirentWPath &dwp, const Entry &e) {
        return NameComparator { dwp } < NameComparator { e };
    }

    inline static Entry MakeFirstEntryByFName() {
        return Entry { false, "", "", 0U };
    }
    static Entry MakeLastEntryByFName();

    // This may look confusing - from the sorting perspective, the higher time stamp (the more recent one)
    // is less than an older time stamp - we want the newer files up higher in the list.
    // Therefore the condition must be inverted including the comparison sequence parameter meaning
    // -> directories first and then the most recent files
    // beware - multiple files may have identical time stamps!
    // In such case, the file name is the only unique identifier and thus must be included in the comparison
    inline static bool LessByTimeEF(const Entry &e, const DirentWPath &dwp) {
        return TimeComparator { dwp } < TimeComparator { e };
    }
    inline static bool LessByTimeFE(const DirentWPath &dwp, const Entry &e) {
        return TimeComparator { e } < TimeComparator { dwp };
    }

    static Entry MakeFirstEntryByTime();

    inline static Entry MakeLastEntryByTime() {
        return Entry { true, "", "", 0U };
    }
};

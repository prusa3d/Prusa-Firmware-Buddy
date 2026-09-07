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
#include <common/filename_type.hpp>
#include <common/directory.hpp>
#include <strings.h>
#include <buddy/filename_defs.hpp>
#include "mutable_path.hpp"
#include "common/utils/utility_extensions.hpp"
#include <bsod/bsod.h>

#ifdef LAZYFILELIST_UNITTEST
extern "C" size_t strlcpy(char *dst, const char *src, size_t dsize);
extern "C" int strcasecmp(const char *a, const char *b); // strcasecmp defined weakly in unit tests to be able to compile them on windows
#endif

class FileSort {
public:
    /// Types of directory entries
    enum class EntryType : uint8_t {
        DIR,
        FILE,
        INVALID, /// Has to be last so that valid items are always less (and inserted)
    };

    /// Entries sort policies
    enum class SortPolicy : uint8_t {
        BY_NAME,
        BY_CRMOD_DATETIME, ///< Sort by combined Creation and Modification time stamp.
        _COUNT
    };

    struct StringViewLight {
        const char *s = nullptr;
        inline StringViewLight() = default;
        inline StringViewLight(const char *s)
            : s(s) {}
        inline bool operator<(const StringViewLight &s2) const {
            return strcasecmp(s, s2.s) < 0;
        }
    };

    // This has to be larger than standard SFN's 11, because SFN in this form gets pseudo-encoded into UTF-8, so the size can bloat.
    // As a result, filenames with diacritics wouldn't fit
    static constexpr size_t MAX_SFN = 24;
    struct EntryRef;
    struct Entry {

    public:
        inline bool is_valid() const {
            return type != EntryType::INVALID;
        }

    public:
        void Clear();
        void CopyFrom(const EntryRef &ref);
        void SetDirUp();

    public:
        uint64_t time = 0;
        EntryType type = EntryType::INVALID;
        char lfn[filename_defs::max_filename_length] = { 0 };
        char sfn[FileSort::MAX_SFN] = { 0 }; // cache the short filenames too, since they will be used in communication with Marlin
    };

    struct EntryRef {

    public:
        EntryRef() = default;
        EntryRef(const Entry &e);
        EntryRef(const dirent &de, const char *sfnPath);

    public:
        inline bool is_valid() const {
            return type != EntryType::INVALID;
        }

    public:
        uint64_t time = 0;
        StringViewLight lfn = nullptr;
        const char *sfn = nullptr;
        EntryType type = EntryType::INVALID;
    };

public:
    // function pointer aliases
    using LessFunc = bool (*)(const EntryRef &, const EntryRef &);

    // These function must not be named the same, otherwise one would have to explicitely
    // specify the correct one in the upper_bound algoritm which looks horrible :)
    inline static bool less_by_name(const EntryRef &a, const EntryRef &b) {
        debug_assert(a.is_valid() && b.is_valid());

        // Using std::tie to avoid errors in comparison of a heterogenous sequence of components
        return std::tie(a.type, a.lfn) < std::tie(b.type, b.lfn);
    }

    // This may look confusing - from the sorting perspective, the higher time stamp (the more recent one)
    // is less than an older time stamp - we want the newer files up higher in the list.
    // Therefore the condition must be inverted including the comparison sequence parameter meaning
    // -> directories first and then the most recent files
    // beware - multiple files may have identical time stamps!
    // In such case, the file name is the only unique identifier and thus must be included in the comparison
    inline static bool less_by_time(const EntryRef &a, const EntryRef &b) {
        debug_assert(a.is_valid() && b.is_valid());

        // Using std::tie to avoid errors in comparison of a heterogenous sequence of components
        return std::tie(a.type, b.time, b.lfn) < std::tie(b.type, a.time, a.lfn);
    }

    static inline constexpr LessFunc sort_policy_less[std::to_underlying(SortPolicy::_COUNT)] = {
        &less_by_name,
        &less_by_time,
    };

    static dirent *find_next(Directory &dir) {
        while (dirent *entry = dir.read()) {
            // ignore hidden files/directories
            if (entry->lfn[0] == '.') {
                continue;
            }

            // all normal directories are accepted
            if ((entry->d_type & DT_DIR) != 0) {
                return entry;
            }

            // files are being filtered by their extension
            if (filename_is_printable(entry->lfn)) {
                return entry;
            }
        }
        return nullptr;
    }
};

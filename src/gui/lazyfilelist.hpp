#pragma once
#include "file_sort.hpp"
#include "mutable_path.hpp"

class LazyDirViewBase : public FileSort {

public:
    void Clear();

    /// @param windowIndex index within the window (i.e. [0 - WINDOW_SIZE-1])
    /// @return pointer to a filename (or nullptr, if there is nothing at that index) and the type of the entry (FILE / DIR)
    std::pair<const char *, EntryType> LongFileNameAt(size_t windowIndex) const {
        return std::make_pair(files[windowIndex].lfn, files[windowIndex].isFile ? EntryType::FILE : EntryType::DIR);
    }

    /// @param windowIndex index within the window (i.e. [0 - WINDOW_SIZE-1])
    /// @return pointer to a filename (or nullptr, if there is nothing at that index) and the type of the entry (FILE / DIR)
    std::pair<const char *, EntryType> ShortFileNameAt(size_t windowIndex) const {
        return std::make_pair(files[windowIndex].sfn, files[windowIndex].isFile ? EntryType::FILE : EntryType::DIR);
    }

    /// @return total number of files/entries in a directory
    int TotalFilesCount() const { return totalFiles; }

    /// @return number of visible files/entries in a directory supported by this instance
    int WindowSize() const { return window_size; }

    /// @return number of currently visible files in the window
    int VisibleFilesCount() const { return totalFiles - windowStartingFrom; }

    /// Returns current window offset
    int window_offset() const { return windowStartingFrom; }

    /// Initial population of the window + sorting is a normal insert sort algorithm with one run through the directory.
    /// Much of the comparison is done in RAM while avoiding slower USB interface as much possible.
    ///
    /// Future implementation consideration for sorting policy:
    /// - compile time - making ChangeDirectory a template: +faster -more code size
    /// - run time - sort policy as a parameter: -slower +less code size
    ///
    /// @param p directory path (will be remembered inside this class)
    /// @param firstDirEntry filename to be placed at the 0th index of the window - the rest is computed accordingly - useful for restoring the view's content.
    ///                      A value of nullptr means put ".." first
    void ChangeDirectory(const char *p, SortPolicy sp = SortPolicy::BY_NAME, const char *firstDirEntry = nullptr);

    /// Sets window offset to the specified position.
    /// Not particularly iefficient implementation.
    /// \returns actual offset set
    int set_window_offset(int target);

    /// Moves the window by $amount items (>0 -> down, <= -> up).
    /// This is not a particularly efficient implementation, TODO better
    /// \returns amount of items actually moved.
    int move_window_by(int amount);

    /// This is what a user does with the knob - moves the window by 1 item up or down if the cursor is at the top end.
    /// It rotates the items downwards (to the right) and computes the new top missing entry
    /// by iterating through the directory's content (which is reading one or more FAT sectors).
    /// @return true if the window was actually moved (i.e. not at the very beginning of the dir)
    bool MoveUp();

    /// A similar operation like MoveUp, but moves the window down by 1 item.
    /// @return true if the window was actually moved
    bool MoveDown();

#ifndef LAZYFILELIST_UNITTEST
protected:
#endif
    int window_size;
    Entry *files; ///< roughly WINDOW_SIZE * 100B, can be placed in CCMRAM if needed. For MINI it is only 9 entries -> only 900B
    int totalFiles; ///< total number of entries in the directory
    int windowStartingFrom; ///< from which entry index the window starts (e.g. from the 3rd file in dir).
                            ///< intentionally int, because -1 means ".."
    char sfnPath[FILE_PATH_BUFFER_LEN]; ///< current directory path - @@TODO this may not be enough - needs checking
    SortPolicy sortPolicy; ///< sort policy set in ChangeDirectory - @@TODO probably not needed at runtime

    /// Current selected sort policy compare functions
    LessEF_t LessEF;
    LessFE_t LessFE;
    MakeEntry_t MakeFirstEntry;
    MakeEntry_t MakeLastEntry;
};

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
template <int WINDOW_SIZE>
class LazyDirView : public LazyDirViewBase {

public:
    LazyDirView() {
        files = files_array.data();
        window_size = WINDOW_SIZE;
        Clear();
    }

public:
    inline const auto &data() const {
        return files_array;
    }

protected:
    std::array<Entry, WINDOW_SIZE> files_array;
};

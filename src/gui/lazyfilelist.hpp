#pragma once
#include "file_sort.hpp"
#include "mutable_path.hpp"

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
class LazyDirView : public FileSort {
public:
    LazyDirView() {
        Clear();
    }

    void Clear() {
        totalFiles = 0;
        windowStartingFrom = 0;
        std::for_each(files.begin(), files.end(), [](Entry &e) { e.Clear(); });
    }

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
    int WindowSize() const { return WINDOW_SIZE; }

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
    void ChangeDirectory(const char *p, SortPolicy sp = SortPolicy::BY_NAME, const char *firstDirEntry = nullptr) {
        size_t filesInWindow = 0; // number of files populated in the window - less than WINDOW_SIZE when there are less files in the dir
        Clear();
        sortPolicy = sp;
        strlcpy(sfnPath, p, sizeof(sfnPath));
        if (!firstDirEntry || firstDirEntry[0] == 0) {
            files[0].SetDirUp(); // this is always the first (zeroth) one
            windowStartingFrom = -1;
        } else {
            // find the file in the directory using pattern search
            F_DIR_RAII_Find_One dir(sfnPath, firstDirEntry);
            if (dir.result != ResType::OK) {
                // the filename was not found, discard the firstDirEntry and start from the beginning
                // of the directory like if firstDirEntry was nullptr
                files[0].SetDirUp();
                windowStartingFrom = -1;
            } else {
                MutablePath dpath { sfnPath };
                dpath.push(dir.fno->d_name);
                files[0].CopyFrom({ *dir.fno, dpath });
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
        F_DIR_RAII_Iterator dir(sfnPath);
        while (dir.FindNext()) {
            MutablePath dpath { sfnPath };
            dpath.push(dir.fno->d_name);

            // Find the right stop to insert the file/entry - a normal binary search algorithm
            // Searching is done from the first (not zeroth) index, because the zeroth must be kept intact - that's the start of our window
            // Impl. detail: cannot use auto, need the write iterator (non const)
            typename decltype(files)::iterator i = std::upper_bound(files.begin() + 1, files.begin() + filesInWindow, DirentWPath { *dir.fno, dpath }, LessFE);
            if (i != files.end()) {
                if (i == files.begin() + 1 && LessFE(DirentWPath { *dir.fno, dpath }, files[0])) {
                    // The file entry could have been inserted outside of the window - i.e. before the first item, which is to be unmovable
                    // However, if it is less than the zeroth entry, we must increment windowStartsFrom
                    ++windowStartingFrom;
                } else {
                    if (strcmp(files[0].lfn, dir.fno->lfn) != 0) {
                        // i.e. we didn't get the same entry as the zeroth entry (which may occur when populating the window with non-null firstDirEntry)
                        // Make place in the window by standard item rotation downwards (to the right)
                        std::rotate(files.rbegin(), files.rbegin() + 1, std::make_reverse_iterator(i)); // solves also the case, when there are less files in the window
                        // Save the entry
                        i->CopyFrom({ *dir.fno, dpath });
                        if (filesInWindow < WINDOW_SIZE) {
                            ++filesInWindow;
                        }
                    }
                }
            } // if i == files.end() -> file entry would have been inserted after the end of the window - ignore

            ++totalFiles; // increment total discovered file entries count
        }
    }

    /// Sets window offset to the specified position.
    /// Not particularly iefficient implementation.
    /// \returns actual offset set
    int set_window_offset(int target) {
        move_window_by(target - window_offset());
        return window_offset();
    }

    /// Moves the window by $amount items (>0 -> down, <= -> up).
    /// This is not a particularly efficient implementation, TODO better
    /// \returns amount of items actually moved.
    int move_window_by(int amount) {
        int remaining = amount;
        while (remaining > 0) {
            if (!MoveDown()) {
                break;
            }

            remaining--;
        }

        while (remaining < 0) {
            if (!MoveUp()) {
                break;
            }

            remaining++;
        }

        return amount - remaining;
    }

    /// This is what a user does with the knob - moves the window by 1 item up or down if the cursor is at the top end.
    /// It rotates the items downwards (to the right) and computes the new top missing entry
    /// by iterating through the directory's content (which is reading one or more FAT sectors).
    /// @return true if the window was actually moved (i.e. not at the very beginning of the dir)
    bool MoveUp() {
        if (windowStartingFrom < 0) {
            return false;
        }

        std::rotate(files.rbegin(), files.rbegin() + 1, files.rend());

        if (windowStartingFrom == 0) {
            // special case code - add a ".."
            files[0].SetDirUp();
            --windowStartingFrom;
            return true;
        }

        F_DIR_RAII_Iterator dir(sfnPath);
        // prepare the item at the zeroth position according to sort policy
        files[0] = MakeFirstEntry();
        while (dir.FindNext()) {
            MutablePath dpath { sfnPath };
            dpath.push(dir.fno->d_name);
            if (LessEF(files[0], { *dir.fno, dpath }) && LessFE({ *dir.fno, dpath }, files[1])) {
                // to be inserted, the entry must be greater than zeroth entry AND less than the first entry
                files[0].CopyFrom({ *dir.fno, dpath });
            }
        }
        --windowStartingFrom;
        return true;
    }

    /// A similar operation like MoveUp, but moves the window down by 1 item.
    /// @return true if the window was actually moved
    bool MoveDown() {
        if (windowStartingFrom >= totalFiles - WINDOW_SIZE - 1) {
            return false; // no more files
        }
        std::rotate(files.begin(), files.begin() + 1, files.end());

        F_DIR_RAII_Iterator dir(sfnPath);
        // prepare the last item according to sort policy
        files[WINDOW_SIZE - 1] = MakeLastEntry();
        while (dir.FindNext()) {
            MutablePath dpath { sfnPath };
            dpath.push(dir.fno->d_name);
            if (LessFE({ *dir.fno, dpath }, files[WINDOW_SIZE - 1]) && LessEF(files[WINDOW_SIZE - 2], { *dir.fno, dpath })) {
                // to be inserted, the entry must be greater than the pre-last entry AND less than the last entry
                files[WINDOW_SIZE - 1].CopyFrom({ *dir.fno, dpath });
            }
        }
        ++windowStartingFrom;
        return true;
    }

#ifndef LAZYFILELIST_UNITTEST
private:
#endif

    std::array<Entry, WINDOW_SIZE> files; ///< roughly WINDOW_SIZE * 100B, can be placed in CCMRAM if needed. For MINI it is only 9 entries -> only 900B
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

#include "lazyfilelist.hpp"

void LazyDirViewBase::Clear() {
    totalFiles = 0;
    windowStartingFrom = 0;

    for (auto i = 0; i < window_size; i++) {
        files[i].Clear();
    }
}

void LazyDirViewBase::ChangeDirectory(const char *p, SortPolicy sp, const char *firstDirEntry) {
    int filesInWindow = 0; // number of files populated in the window - less than WINDOW_SIZE when there are less files in the dir
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

    const auto files_begin = files;
    const auto files_end = files + window_size;
    const auto files_rbegin = std::make_reverse_iterator(files_end);

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
        Entry *i = std::upper_bound(files_begin + 1, files_begin + filesInWindow, DirentWPath { *dir.fno, dpath }, LessFE);
        if (i != files_end) {
            if (i == files_begin + 1 && LessFE(DirentWPath { *dir.fno, dpath }, files[0])) {
                // The file entry could have been inserted outside of the window - i.e. before the first item, which is to be unmovable
                // However, if it is less than the zeroth entry, we must increment windowStartsFrom
                ++windowStartingFrom;
            } else {
                if (strcmp(files[0].lfn, dir.fno->lfn) != 0) {
                    // i.e. we didn't get the same entry as the zeroth entry (which may occur when populating the window with non-null firstDirEntry)
                    // Make place in the window by standard item rotation downwards (to the right)
                    std::rotate(files_rbegin, files_rbegin + 1, std::make_reverse_iterator(i)); // solves also the case, when there are less files in the window
                    // Save the entry
                    i->CopyFrom({ *dir.fno, dpath });
                    if (filesInWindow < window_size) {
                        ++filesInWindow;
                    }
                }
            }
        } // if i == files.end() -> file entry would have been inserted after the end of the window - ignore

        ++totalFiles; // increment total discovered file entries count
    }
}

int LazyDirViewBase::set_window_offset(int target) {
    move_window_by(target - window_offset());
    return window_offset();
}

int LazyDirViewBase::move_window_by(int amount) {
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

bool LazyDirViewBase::MoveUp() {
    if (windowStartingFrom < 0)
        return false;

    const auto files_rbegin = std::make_reverse_iterator(files + window_size);
    const auto files_rend = std::make_reverse_iterator(files);
    std::rotate(files_rbegin, files_rbegin + 1, files_rend);

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

bool LazyDirViewBase::MoveDown() {
    if (windowStartingFrom >= totalFiles - window_size - 1) {
        return false; // no more files
    }

    const auto files_begin = files;
    const auto files_end = files + window_size;
    std::rotate(files_begin, files_begin + 1, files_end);

    F_DIR_RAII_Iterator dir(sfnPath);
    // prepare the last item according to sort policy
    files[window_size - 1] = MakeLastEntry();
    while (dir.FindNext()) {
        MutablePath dpath { sfnPath };
        dpath.push(dir.fno->d_name);
        if (LessFE({ *dir.fno, dpath }, files[window_size - 1]) && LessEF(files[window_size - 2], { *dir.fno, dpath })) {
            // to be inserted, the entry must be greater than the pre-last entry AND less than the last entry
            files[window_size - 1].CopyFrom({ *dir.fno, dpath });
        }
    }
    ++windowStartingFrom;
    return true;
}

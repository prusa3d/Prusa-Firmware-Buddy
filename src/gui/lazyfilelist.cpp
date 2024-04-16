#include "lazyfilelist.hpp"

namespace {

using Entry = LazyDirViewBase::Entry;
using EntryRef = LazyDirViewBase::EntryRef;

// Stub implementation because std::shift_x uses exceptions in its functions which doesn't compile for us.... >:(
auto shift_left(auto begin, auto end, int amount) {
    end -= amount;
    while (begin < end) {
        *begin = std::move(*(begin + amount));
        begin++;
    }
}

auto shift_right(auto begin, auto end, int amount) {
    for (auto it = end - amount - 1; it >= begin; it--) {
        *(it + amount) = std::move(*it);
    }
}

/// Inserts the $item into the range [$insert_range_begin, $insert_range_end) so that it is kept sorted by $less.
/// The range is expanded ($insert_range_end is increased) when inserting the items until $insert_range_end == $full_insert_range_end.
/// After fully expanded, the function basically upkeeps top $n (full_insert_range_end - insert_range_begin) of the sorted range
void insert_item(Entry *insert_range_begin, Entry *&insert_range_end, Entry *full_insert_range_end, const EntryRef &item, const FileSort::LessFunc &less) {
    // Determine by binary search where we should insert the item
    Entry *it = std::lower_bound(insert_range_begin, insert_range_end, item, less);

    // Outside of the full insert range (the final is correct) -> throw away
    if (it == full_insert_range_end) {
        return;
    }

    // Expand the working insert range
    if (insert_range_end < full_insert_range_end) {
        insert_range_end++;
    }

    // Make space for the inserted item
    shift_right(it, insert_range_end, 1);

    it->CopyFrom(item);
}

} // namespace

void LazyDirViewBase::Clear() {
    totalFiles = 0;
    windowStartingFrom = 0;

    for (auto i = 0; i < window_size; i++) {
        files_data[i].Clear();
    }
}

void LazyDirViewBase::ChangeDirectory(const char *p, SortPolicy sp, const char *firstDirEntry) {
    Clear();

    sortPolicy = sp;
    strlcpy(sfnPath, p, sizeof(sfnPath));

    // Try to find the first dir entry
    const bool has_first_dir_entry = [&] {
        if (!firstDirEntry || firstDirEntry[0] == 0) {
            return false;
        }

        // the filename was not found, discard the firstDirEntry and start from the beginning
        // of the directory like if firstDirEntry was nullptr
        F_DIR_RAII_Find_One dir(sfnPath, firstDirEntry);
        if (dir.result != ResType::OK) {
            return false;
        }

        // Check if the entry isn't a link or something we don't support
        const auto eref = EntryRef(*dir.fno, sfnPath);
        if (!eref.is_valid()) {
            return false;
        }

        // windowStartsFrom will be fine tuned later during iteration over the whole dir content
        // And the dir must closed here, because the search cycle uses a different search pattern
        files_data[0].CopyFrom(eref);
        return true;
    }();

    // If not found, set the first item to be dir up
    if (!files_data[0].is_valid()) {
        files_data[0].SetDirUp(); // this is always the first (zeroth) one
    }

    // Either way, we already have one total file
    totalFiles = 1;

    // This will be adjusted in the following calculation
    windowStartingFrom = -1;

    const auto less = sort_policy_less[ftrstd::to_underlying(sortPolicy)];

    const auto files_begin = files_data;
    const auto files_end = files_data + window_size;

    auto filled_region_end = files_begin + 1;

    F_DIR_RAII_Iterator dir(sfnPath);
    while (dir.FindNext()) {
        const EntryRef curr(*dir.fno, sfnPath);

        // Check if the entry isn't a link or something we don't support
        if (!curr.is_valid()) {
            continue;
        }

        totalFiles++;

        // Check if we're <= first entry, in that case we don't add to the list, but instead increase windowStartingFrom
        // The <= is important and correct, because windowStartingFrom is initialized to -1
        if (has_first_dir_entry && !less(files_data[0], curr)) {
            windowStartingFrom++;
            continue;
        }

        // We use files_begin + 1 because the 0th item has already been filed before the loop
        insert_item(files_begin + 1, filled_region_end, files_end, curr, less);
    }
}

int LazyDirViewBase::set_window_offset(int target) {
    move_window_by(target - window_offset());
    return window_offset();
}

int LazyDirViewBase::move_window_by(int amount) {
    int remaining = amount;

    while (remaining > 0) {
        int tamount = std::min(remaining, window_size - 1);
        if (!MoveDown(tamount)) {
            break;
        }

        remaining -= tamount;
    }

    while (remaining < 0) {
        int tamount = std::min(-remaining, window_size - 1);
        if (!MoveUp(tamount)) {
            break;
        }

        remaining += tamount;
    }

    return amount - remaining;
}

bool LazyDirViewBase::MoveUp(int amount) {
    // This function does not support moving by more whan window_size - 1, because if needs anchor_item to base sorting off.
    // For larger jumps, use move_window_by
    assert(amount < window_size);

    if (amount == 0) {
        return true;
    }

    if (windowStartingFrom - amount < -1) {
        return false;
    }

    windowStartingFrom -= amount;

    const auto less = sort_policy_less[ftrstd::to_underlying(sortPolicy)];

    const auto files_end = files_data + window_size;
    const auto full_insert_range_end = files_data + amount;
    auto insert_range_begin = files_data;
    auto insert_range_end = insert_range_begin;

    // Shift the existing items
    shift_right(files_data, files_end, amount);

    // special case - add a ".."
    if (windowStartingFrom == -1) {
        files_data[0].SetDirUp();

        // If we added just the "..", we don't have to iterate the directory at all
        if (amount == 1) {
            return true;
        }

        // first record is fixed -> exclude from inserting
        insert_range_begin++;
        insert_range_end++;
    }

    // Iterate the directory and sort in missing items
    {
        const EntryRef anchor_item = *full_insert_range_end;

        F_DIR_RAII_Iterator dir(sfnPath);
        while (dir.FindNext()) {
            const EntryRef curr(*dir.fno, sfnPath);

            // Check if the entry isn't a link or something we don't support
            if (!curr.is_valid()) {
                continue;
            }

            // Check if we're not behind (comparison-wise) the anchor item
            if (!less(curr, anchor_item)) {
                continue;
            }

            // Cannot use insert_item because we're actually building a tail of the list, which needs different behavior
            // Determine by binary search where we should insert the item
            Entry *it = std::lower_bound(insert_range_begin, insert_range_end, curr, less);

            // If we're trying to insert behind the full end of the sorted range, move everything to the left to make space
            if (it == full_insert_range_end) {
                shift_left(insert_range_begin, insert_range_end, 1);
                it--;
            }

            // We're full and are trying to prepend item -> discard
            else if (insert_range_end == full_insert_range_end && it == insert_range_begin) {
                continue;
            }

            // If the buffer is full, start shifting to the left
            else if (insert_range_end == full_insert_range_end) {
                shift_left(insert_range_begin, it, 1);
                it--;
            }

            // Otherwise expand the range and move items to the right to make space
            else {
                insert_range_end++;
                shift_right(it, insert_range_end, 1);
            }

            it->CopyFrom(curr);
        }
    }

    // Fill in voids, in case files were removed somewhen during the file list existence
    while (insert_range_end != full_insert_range_end) {
        insert_range_end->Clear();
        insert_range_end++;
    }

    return true;
}

bool LazyDirViewBase::MoveDown(int amount) {
    // This function does not support moving by more whan window_size - 1, because if needs anchor_item to base sorting off.
    // For larger jumps, use move_window_by
    assert(amount < window_size);

    if (amount == 0) {
        return true;
    }

    if (windowStartingFrom + amount >= totalFiles - window_size) {
        return false; // no more files
    }

    windowStartingFrom += amount;

    const auto less = sort_policy_less[ftrstd::to_underlying(sortPolicy)];

    const auto files_end = files_data + window_size;
    const auto full_insert_range_end = files_end;
    const auto insert_range_begin = files_end - amount;
    auto insert_range_end = insert_range_begin;

    // Shift the existing items
    shift_left(files_data, files_end, amount);

    // Iterate the directory and sort in missing items
    {
        const EntryRef anchor_item = *(insert_range_begin - 1);

        F_DIR_RAII_Iterator dir(sfnPath);
        while (dir.FindNext()) {
            const EntryRef curr(*dir.fno, sfnPath);

            // Check if the entry isn't a link or something we don't support
            if (!curr.is_valid()) {
                continue;
            }

            // Check if we're not behind (comparison-wise) the anchor item
            if (!less(anchor_item, curr)) {
                continue;
            }

            insert_item(insert_range_begin, insert_range_end, full_insert_range_end, curr, less);
        }
    }

    // Fill in voids, in case files were removed somewhen during the file list existence
    while (insert_range_end != full_insert_range_end) {
        insert_range_end->Clear();
        insert_range_end++;
    }

    return true;
}

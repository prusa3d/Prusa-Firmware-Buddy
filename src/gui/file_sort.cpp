#include "file_sort.hpp"
#include <transfers/transfer_file_check.hpp>

void FileSort::Entry::Clear() {
    type = EntryType::INVALID;
    name = {};
    time = 0;
}

void FileSort::Entry::CopyFrom(const EntryRef &er) {
    name.assign(er.canonical, er.display.s);
    type = er.type;
    time = er.time;
}

void FileSort::Entry::SetDirUp() {
    name.assign("..");
    type = EntryType::DIR;
    time = UINT_LEAST64_MAX;
}

FileSort::EntryRef::EntryRef(const Entry &e)
    : time(e.time)
    , display(e.name.display())
    , canonical(e.name.canonical())
    , type(e.type) {
}
FileSort::EntryRef::EntryRef(const dirent &de, const char *canonical_path)
    : time(de.time)
    , display(de.lfn)
    , canonical(de.d_name) {

    // Directory might be an actual directory or a running file transfer. we gotta check
    if (de.d_type & DT_DIR) {
        MutablePath path(canonical_path);
        path.push(de.d_name);
        const auto r = transfers::transfer_check(path);

        if (r.is_valid()) {
            type = EntryType::FILE;
        } else if (r.is_transfer()) {
            type = EntryType::INVALID;
        } else {
            type = EntryType::DIR;
        }
    }

    else if (de.d_type & DT_REG) {
        type = EntryType::FILE;
    }

    else {
        type = EntryType::INVALID;
    }
}

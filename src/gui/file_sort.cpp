#include "file_sort.hpp"
#include <transfers/transfer_file_check.hpp>

void FileSort::Entry::Clear() {
    type = EntryType::INVALID;
    lfn[0] = 0;
    sfn[0] = 0;
    time = 0;
}

void FileSort::Entry::CopyFrom(const EntryRef &er) {
    if (er.sfn) {
        strlcpy(sfn, er.sfn, sizeof(sfn));
    }

    if (er.lfn.s) {
        strlcpy(lfn, er.lfn.s, sizeof(lfn));
    }

    type = er.type;
    time = er.time;
}

void FileSort::Entry::SetDirUp() {
    lfn[0] = lfn[1] = sfn[0] = sfn[1] = '.';
    lfn[2] = sfn[2] = 0;
    type = EntryType::DIR;
    time = UINT_LEAST64_MAX;
}

FileSort::EntryRef::EntryRef(const Entry &e)
    : time(e.time)
    , lfn(e.lfn)
    , sfn(e.sfn)
    , type(e.type) {
}
FileSort::EntryRef::EntryRef(const dirent &de, const char *sfnPath)
    : time(de.time)
    , lfn(de.lfn)
    , sfn(de.d_name) {

    // Directory might be an actual directory or a running file transfer. we gotta check
    if (de.d_type & DT_DIR) {
        MutablePath path(sfnPath);
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

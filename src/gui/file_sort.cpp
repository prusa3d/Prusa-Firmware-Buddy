#include "file_sort.hpp"
#include <transfers/transfer_file_check.hpp>

void FileSort::Entry::Clear() {
    isFile = false;
    lfn[0] = 0;
    sfn[0] = 0;
    time = 0;
}

void FileSort::Entry::CopyFrom(const DirentWPath &dwp) {
    strlcpy(sfn, dwp.d.d_name, sizeof(sfn));
    strlcpy(lfn, dwp.d.lfn, sizeof(lfn));

    isFile = (dwp.d.d_type & DT_REG) != 0

        || transfers::is_valid_transfer(dwp.full_path);
    time = dwp.d.time;
}

void FileSort::Entry::SetDirUp() {
    lfn[0] = lfn[1] = sfn[0] = sfn[1] = '.';
    lfn[2] = sfn[2] = 0;
    isFile = false;
    time = UINT_LEAST64_MAX;
}

FileSort::TimeComparator::TimeComparator(const Entry &e)
    : is_dir(!e.isFile)
    , time(e.time)
    , lfn_name(e.lfn) {}
FileSort::TimeComparator::TimeComparator(const DirentWPath &dwp)
    : is_dir((dwp.d.d_type & DT_DIR) != 0 && !transfers::is_valid_transfer(dwp.full_path))
    , time(dwp.d.time)
    , lfn_name(dwp.d.lfn) {}

FileSort::NameComparator::NameComparator(const Entry &e)
    : is_file(e.isFile)
    , lfn_name(e.lfn) {}
FileSort::NameComparator::NameComparator(const DirentWPath &dwp)
    : is_file((dwp.d.d_type & DT_REG) != 0 || transfers::is_valid_transfer(dwp.full_path))
    , lfn_name(dwp.d.lfn) {}

auto FileSort::MakeLastEntryByFName() -> Entry {
    Entry e = { true, "", "", 0xffff };
    std::fill(e.lfn, e.lfn + sizeof(e.lfn) - 1, 0xff);
    e.lfn[sizeof(e.lfn) - 1] = 0;
    return e;
}

auto FileSort::MakeFirstEntryByTime() -> Entry {
    Entry e = { false, "", "", UINT_LEAST64_MAX };
    std::fill(e.lfn, e.lfn + sizeof(e.lfn) - 1, 0xff);
    e.lfn[sizeof(e.lfn) - 1] = 0;
    // since the sfn is not used for comparison anywhere in LazyDirView,
    // its initialization may be skipped here to save some code
    //        std::fill(e.sfn, e.sfn + sizeof(e.sfn) - 1, 0xff);
    //        e.sfn[sizeof(e.sfn) - 1] = 0;
    return e;
}

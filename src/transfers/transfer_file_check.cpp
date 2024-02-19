#include <sys/stat.h>

#include <stat_retry.hpp>

#include "filename_type.hpp"
#include "transfer_file_check.hpp"

namespace transfers {

TransferCheckResult transfer_check(const MutablePath &filepath, TransferCheckValidOnly check_valid_only) {
    TransferCheckResult r;

    // This early rejection saves us two stat lookups, which are very expensive (tens of ms).
    if (!filename_is_transferrable(filepath.get())) {
        return r;
    }

    r.partial_file_found = filepath.execute_with_pushed(partial_filename, stat_retry, &r.partial_file_stat) == 0 && S_ISREG(r.partial_file_stat.st_mode);

    // Partial file not found -> definitely not valid, we can return early
    if (!r.partial_file_found && check_valid_only == TransferCheckValidOnly::yes) {
        return r;
    }

    struct stat backup_file_stat;
    r.backup_file_found = filepath.execute_with_pushed(backup_filename, stat_retry, &backup_file_stat) == 0 && S_ISREG(backup_file_stat.st_mode);
    r.backup_file_empty = r.backup_file_found && backup_file_stat.st_size == 0;

    return r;
}

bool is_valid_file_or_transfer(const MutablePath &file) {
    struct stat st;

    // Failed to get stat about the entry -> fail
    if (stat_retry(file.get(), &st) != 0) {
        return false;
    }

    // File is actually a file -> good enough for us
    if (S_ISREG(st.st_mode)) {
        return true;
    }

    // If it is a dir, it could be a valid transfer
    else if (S_ISDIR(st.st_mode)) {
        return is_valid_transfer(file);
    }

    else {
        return false;
    }
}

} // namespace transfers

#include <sys/stat.h>

#include <stat_retry.hpp>
#include "transfer_file_check.hpp"

namespace transfers {

IsTransferResult is_transfer(const MutablePath &filepath) {
    struct stat st;

    const bool partial_file_found = filepath.execute_with_pushed(partial_filename, stat_retry, &st) == 0 && S_ISREG(st.st_mode);
    const bool backup_file_found = filepath.execute_with_pushed(backup_filename, stat_retry, &st) == 0 && S_ISREG(st.st_mode);
    const bool backup_is_empty = backup_file_found && st.st_size == 0;

    // No backup or partial file -> this is not a transfer
    if (!backup_file_found && !partial_file_found) {
        return IsTransferResult::not_a_transfer;
    }

    // No partial file -> some invalid state
    // Existing empty backup file -> transfer is waiting to be removed
    else if (!partial_file_found || backup_is_empty) {
        return IsTransferResult::invalid_transfer;
    }

    // Partial file & no backup -> finished transfer waiting to be moved
    // Partial file & backup not empty -> running transfer
    else {
        return IsTransferResult::valid_transfer;
    }
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

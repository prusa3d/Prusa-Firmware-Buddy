#include <sys/stat.h>

#include <stat_retry.hpp>
#include "transfer_file_check.hpp"

namespace transfers {

bool is_valid_transfer(const MutablePath &destination_path) {
    struct stat st;

    if (bool partial_file_found = destination_path.execute_with_pushed(partial_filename, stat_retry, &st) == 0 && S_ISREG(st.st_mode);
        !partial_file_found) {
        return false;
    }

    bool backup_file_found = destination_path.execute_with_pushed(backup_filename, stat_retry, &st) == 0 && S_ISREG(st.st_mode);
    bool backup_is_empty = backup_file_found && st.st_size == 0;

    if (!backup_file_found) {
        // finished transfer, waiting for move to file
        return true;
    } else if (backup_is_empty) {
        // we gave up on this one, waiting to be removed
        return false;
    }

    // still in progress
    return true;
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

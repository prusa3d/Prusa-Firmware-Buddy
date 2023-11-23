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
} // namespace transfers

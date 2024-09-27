#pragma once

/**
 * @brief Contains check whether a directory is actually a transfer file. Split from transfer.hpp to be able to use this function without including all the other transfer.hpp dependencies
 *
 */
#include <mutable_path.hpp>
#include <sys/stat.h>

namespace transfers {

inline constexpr const char *partial_filename { "p" };
inline constexpr const char *backup_filename { "d" };

struct TransferCheckResult {
    struct stat partial_file_stat = {};

    bool partial_file_found : 1 = false;
    bool backup_file_found : 1 = false;
    bool backup_file_empty : 1 = false;

    /// \returns whether the result corresponds to a valid transfer
    inline bool is_valid() const {
        return is_running() || is_finished();
    }

    /// \returns whether the filepath corresponds to a transfer (valid or invalid)
    inline bool is_transfer() const {
        return backup_file_found || partial_file_found;
    }

    inline bool is_finished() const {
        // Partial file & no backup -> finished transfer waiting to be moved
        return partial_file_found && !backup_file_found;
    }

    inline bool is_running() const {
        // Partial file & backup present and not empty -> running transfer
        return partial_file_found && backup_file_found && !backup_file_empty;
    }

    inline bool is_aborted() const {
        return partial_file_found && backup_file_found && backup_file_empty;
    }
};

enum class TransferCheckValidOnly {
    no,
    yes
};

/// Checks if a provided path/filename is a transfer.
/// If \param check_valid_only is yes, the check is faster, but is_transfer() result might be wrong
TransferCheckResult transfer_check(const MutablePath &filepath, TransferCheckValidOnly check_valid_only = TransferCheckValidOnly::no);

/// Checks if it is a folder with a partial and backup file and is valid.
// It is valid, if the trasnfer is finished, in progress, or we intend to
// retry it when avaiable, if we gave up on trying it will be not valid.
inline bool is_valid_transfer(const MutablePath &filepath) {
    return transfer_check(filepath, TransferCheckValidOnly::yes).is_valid();
}

/// \returns whether the \p file is an existing file or a valid transfer
bool is_valid_file_or_transfer(const MutablePath &file);

} // namespace transfers

#pragma once

/**
 * @brief Contains check whether a directory is actually a transfer file. Split from transfer.hpp to be able to use this function without including all the other transfer.hpp dependencies
 *
 */
#include <mutable_path.hpp>

namespace transfers {

inline constexpr const char *partial_filename { "p" };
inline constexpr const char *backup_filename { "d" };

/**
 * @brief Checks if it is a folder with a partial and backup file and is valid.
 *  It is valid, if the trasnfer is finished, in progress, or we intent to
 *  retry it when avaiable, if we gave up on trying it will be not valid.
 *
 * @param destination_path
 */
bool is_valid_transfer(const MutablePath &destination_path);

/// \returns whether the \p file is an existing file or a valid transfer
bool is_valid_file_or_transfer(const MutablePath &file);

} // namespace transfers

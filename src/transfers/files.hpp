#pragma once

#include <common/unique_file_ptr.hpp>

#include <array>
#include <cstdlib>
#include <cstring>
#include <variant>

namespace transfers {

static const char *const USB_MOUNT_POINT = "/usb/";
static const size_t USB_MOUNT_POINT_LENGTH = strlen(USB_MOUNT_POINT);

// Length of the temporary file name. Due to the template below, we are sure to
// fit.
static const size_t TRANSFER_NAME_LEN = 20;
static const char *const CHECK_FILENAME = "/usb/check.tmp";

using TransferName = std::array<char, TRANSFER_NAME_LEN>;

/// Allocate a new index for temp transfer files.
///
/// Unique during each boot of the printer, starting at 0.
size_t next_transfer_idx();

/// Converts the index into full file name.
///
/// Eg. /usb/42.tmp
TransferName transfer_name(size_t idx);

using PreallocateResult = std::variant<const char *, unique_file_ptr>;

/// Tries to preallocate a given file of at least given size.
///
/// Returns either a file handle to the opened file or a string description of
/// an error.
PreallocateResult file_preallocate(const char *fname, size_t size);

/// Writes the whole block (or fails).
///
/// Will handle retries on certain temporary errors. If successful (returns true), the whole block has been written.
///
/// If not successful, the amount of data and content of the file is unspecified.
bool write_block(FILE *f, const uint8_t *data, size_t size);

} // namespace transfers

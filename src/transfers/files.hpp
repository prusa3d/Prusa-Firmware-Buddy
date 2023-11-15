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

} // namespace transfers

/// @file
/// Host-side helpers for tunnelling pubby firmware to a CAN/RS-485 sub-device
/// over Modbus. Shared by the bridge flashing hosts (XBuddyExtension, XlCan).
#pragma once

#include <xbuddy_extension/modbus.hpp>
#include <xbuddy_extension/shared_enums.hpp>

namespace buddy::puppies {

/// Open a pubby firmware blob (an embedded resource under /internal/res/puppies)
/// for reading. Returns the file descriptor, or -1 on failure (logged).
[[nodiscard]] int open_firmware_file(xbuddy_extension::FileId file_id);

/// Compute the salted SHA256 of firmware blob `file_id` into `out`, echoing
/// `request` and setting `out.status` (ok / unavailable / retry).
///  Heavy: reads the whole file, takes time.
void compute_digest_response(xbuddy_extension::modbus::DigestRequest request,
    xbuddy_extension::FileId file_id, xbuddy_extension::modbus::Digest &out);

} // namespace buddy::puppies

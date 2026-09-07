/// @file
#pragma once

#include <cstddef>
#include <cstdint>
#include <inplace_function.hpp>
#include <modbus/server_address.hpp>
#include <puppies/PuppyModbus.hpp>
#include <utils/uncopyable.hpp>
#include <xbuddy_extension/modbus.hpp>
#include <xbuddy_extension/shared_enums.hpp>

namespace buddy::puppies {

/// Master side of the firmware Cyphal--over-Modbus tunnel used to flash pubbies that
/// hang off a bridge board.
///
/// The pubby's bootloader asks its bridge (over Cyphal) to verify the app
/// digest, and to send firmware chunks when blank; the bridge surfaces those as
/// chunk_request/digest_request in its Status block. We answer them by
/// streaming the matching file over the Chunk and Digest holding blocks.
///
/// Not internally synchronized -- every method must be called with the owning
/// pubby's mutex held, progress_percent() included.
class CyphalBridgeFlashHost : public Uncopyable {
public:
    ~CyphalBridgeFlashHost() { close_flash_file(); }

    /// Computes the digest response. Runs a file read + SHA256, so
    /// write_digest() callers are expected to release the owning puppy's mutex
    /// for the duration and reacquire it before returning.
    using DigestComputeFn = stdext::inplace_function<
        void(
            xbuddy_extension::modbus::DigestRequest request,
            xbuddy_extension::FileId file_id,
            xbuddy_extension::modbus::Digest &out)>;

    /// Publish the requests read from the owner's Status block. Until this
    /// happens (or once invalidate() is called) write_chunk() refuses to act,
    /// so we never answer a request we're not sure is current.
    void set_requests(
        xbuddy_extension::modbus::ChunkRequest chunk,
        xbuddy_extension::modbus::DigestRequest digest);

    /// Mark the published requests as stale, after a failed Status read.
    void invalidate() { valid = false; }

    /// Whether a firmware file is open. Owners read the Status block on every
    /// exchange while this holds, to pick up the next chunk request ASAP.
    [[nodiscard]] bool flashing() const { return flash_fd != -1; }

    /// Flashing progress (0-100 percent, 0 when not flashing).
    [[nodiscard]] uint8_t progress_percent() const;

    /// Send the chunk the puppy asked for. ERROR if no current request has been
    /// published or the file could not be read; SKIPPED when there is nothing
    /// new to send.
    CommunicationStatus write_chunk(PuppyModbus &, modbus::ServerAddress server);

    /// Answer the pending digest request. Never reports ERROR: a write that
    /// didn't land is simply retried on the next cycle.
    CommunicationStatus write_digest(PuppyModbus &, modbus::ServerAddress server, DigestComputeFn compute);

    void close_flash_file();

private:
    /// Whether the request fields below come from a successful Status read.
    bool valid = false;

    /// Latest requests, published from the owner's Status read.
    xbuddy_extension::modbus::ChunkRequest current_chunk_request {};
    xbuddy_extension::modbus::DigestRequest current_digest_request {};

    /// Dedup: don't resend a request we already answered. Safe across pubby
    /// resets because each request carries a fresh salt/offset.
    xbuddy_extension::modbus::ChunkRequest last_chunk_request {};
    xbuddy_extension::modbus::DigestRequest last_digest_request {};

    /// Firmware file being streamed (-1 = not flashing) and its cached size.
    int flash_fd = -1;
    size_t flash_file_size = 0;
};

} // namespace buddy::puppies

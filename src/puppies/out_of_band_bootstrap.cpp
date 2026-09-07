#include <puppies/out_of_band_bootstrap.hpp>

#include <array>
#include <buddy/bootstrap_state.hpp>
#include <buddy/digest.hpp>
#include <common/timing.h>
#include <cstdint>
#include <fcntl.h>
#include <option/has_puppies_bootloader.h>
#include <option/puppy_flash_fw.h>
#include <random/random.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utils/uncopyable.hpp>

namespace buddy::puppies {

using buddy::BootstrapStage;

namespace {

    constexpr auto command_ok = BootloaderProtocol::status_t::COMMAND_OK;

    struct FirmwareInfo : Uncopyable {
        int fd;
        off_t size;

        explicit FirmwareInfo(const OutOfBandPuppy &puppy)
            : fd { open(puppy.firmware_path, O_RDONLY) }
            , puppy { puppy } {
            if (fd == -1) {
                fail();
            }

            struct stat fs;
            if (fstat(fd, &fs) == -1) {
                fail();
            }

            size = fs.st_size;
            if (size == 0) {
                fail();
            }
        }

        ~FirmwareInfo() {
            if (fd != -1) {
                close(fd);
            }
        }

        [[noreturn]] void fail() const {
            fatal_error(ErrCode::ERR_SYSTEM_PUPPY_FW_NOT_FOUND, puppy.name);
        }

    private:
        const OutOfBandPuppy &puppy;
    };

    void get_fingerprint(
        BootloaderProtocol &bootloader_protocol,
        BootloaderProtocol::fingerprint_t &puppy_fingerprint,
        uint32_t start_ms) {

        const auto timeout_ms = 1000;
        while (ticks_diff(ticks_ms(), start_ms) < timeout_ms) {
            if (bootloader_protocol.get_fingerprint(puppy_fingerprint) == command_ok) {
                return;
            }
        }
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_FINGERPRINT_TIMEOUT);
    }

    [[nodiscard]] bool verify(
        BootloaderProtocol &bootloader_protocol,
        const OutOfBandPuppy &puppy,
        const FirmwareInfo &firmware_info,
        uint32_t &salt,
        BootloaderProtocol::fingerprint_t &buddy_fingerprint) {

        bootstrap_state_set(0, puppy.verifying_stage);

        // salt needs to be changed for every attempt for security reasons
        salt = rand_u();

        // ask puppy to compute fw fingerprint
        if (bootloader_protocol.compute_fingerprint(salt) != command_ok) {
            fatal_error(ErrCode::ERR_SYSTEM_PUPPY_NOT_RESPONDING, puppy.name);
        }
        const auto fingerprint_wait_start = ticks_ms();

        Digest digest {
            (std::byte *)buddy_fingerprint.data(),
            buddy_fingerprint.size(),
        };
        buddy::compute_file_digest_with_retry(firmware_info.fd, salt, digest);

        BootloaderProtocol::fingerprint_t puppy_fingerprint;
        get_fingerprint(bootloader_protocol, puppy_fingerprint, fingerprint_wait_start);

        return buddy_fingerprint == puppy_fingerprint;
    }

    void flash(
        BootloaderProtocol &bootloader_protocol,
        const OutOfBandPuppy &puppy,
        const FirmwareInfo &firmware_info) {

        const auto get_data = [&](uint32_t offset, size_t size, uint8_t *out_data) -> bool {
            const uint8_t percent = static_cast<uint8_t>(100 * offset / firmware_info.size);
            bootstrap_state_set(percent, puppy.flashing_stage);
            if (lseek(firmware_info.fd, offset, SEEK_SET) == -1) {
                return false;
            }
            if (read(firmware_info.fd, out_data, size) == -1) {
                return false;
            }
            return true;
        };

        if (bootloader_protocol.write_flash(firmware_info.size, get_data) != command_ok) {
            fatal_error(ErrCode::ERR_SYSTEM_PUPPY_WRITE_FLASH_ERR, puppy.name);
        }
    }

    void verify_and_flash(
        BootloaderProtocol &bootloader_protocol,
        const OutOfBandPuppy &puppy,
        uint32_t &salt,
        BootloaderProtocol::fingerprint_t &buddy_fingerprint) {

#if PUPPY_FLASH_FW()
        const FirmwareInfo firmware_info { puppy };
        while (!verify(bootloader_protocol, puppy, firmware_info, salt, buddy_fingerprint)) {
            flash(bootloader_protocol, puppy, firmware_info);
        }
#else
        // #error dead code found by automatic analyses (see BFW-5461)
        // We still need a valid salt and fingerprint for subsequent run_app().
        // Just read back the computed fingerprint, the puppy will always accept it.
        bootstrap_state_set(0, puppy.verifying_stage);
        salt = 0;
        bootloader_protocol.compute_fingerprint(salt);
        get_fingerprint(bootloader_protocol, buddy_fingerprint, ticks_ms());
#endif
    }

} // namespace

bool out_of_band_discover(
    BootloaderProtocol &bootloader_protocol,
    const OutOfBandPuppy &puppy,
    uint16_t &protocol_version) {

    constexpr int max_attempts = 3;
    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        puppy.reset();
        if (bootloader_protocol.get_protocolversion(protocol_version) == command_ok) {
            return true;
        }
    }
    return false;
}

void out_of_band_bootstrap(BootloaderProtocol &bootloader_protocol, const OutOfBandPuppy &puppy) {
#if HAS_PUPPIES_BOOTLOADER()
    bootstrap_state_set(0, puppy.looking_for_stage);

    bootloader_protocol.set_address(puppy.bootloader_address);

    uint16_t protocol_version;
    if (!out_of_band_discover(bootloader_protocol, puppy, protocol_version)) {
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_NOT_RESPONDING, puppy.name);
    }
    if ((protocol_version & 0xff00) != (BootloaderProtocol::BOOTLOADER_PROTOCOL_VERSION & 0xff00)) {
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_INCOMPATIBLE_BOOTLODER, protocol_version, BootloaderProtocol::BOOTLOADER_PROTOCOL_VERSION);
    }

    BootloaderProtocol::HwInfo hwinfo;
    if (bootloader_protocol.get_hwinfo(hwinfo) != command_ok) {
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_NOT_RESPONDING, puppy.name);
    }
    if (hwinfo.hw_type != puppy.hw_type) {
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_UNKNOWN_TYPE);
    }

    std::array<uint8_t, 32> otp;
    if (bootloader_protocol.read_otp_cmd(0, otp.data(), otp.size()) != command_ok) {
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_NOT_RESPONDING, puppy.name);
    }
    puppy.set_otp(*reinterpret_cast<const OTP_v5 *>(otp.data()));

    uint32_t salt;
    BootloaderProtocol::fingerprint_t buddy_fingerprint;
    verify_and_flash(bootloader_protocol, puppy, salt, buddy_fingerprint);

    if (bootloader_protocol.run_app(salt, buddy_fingerprint) != command_ok) {
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_START_APP_ERR);
    }
#else
    // #error dead code found by automatic analyses (see BFW-5461)
    (void)bootloader_protocol;
    (void)puppy;
#endif
}

} // namespace buddy::puppies

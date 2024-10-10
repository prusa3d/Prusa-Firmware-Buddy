#include "puppies/PuppyBootstrap.hpp"
#include "buffered_serial.hpp"
#include "puppies/BootloaderProtocol.hpp"
#include "puppies/PuppyBus.hpp"
#include "puppies/PuppyModbus.hpp"
#include "bsod.h"
#include <sys/stat.h>
#include "assert.h"
#include "hwio_pindef.h"
#include "mbedtls/sha256.h"
#include <logging/log.hpp>
#include <buddy/main.h>
#include "tasks.hpp"
#include "timing.h"
#include "bsod.h"
#include "otp.hpp"
#include <option/has_puppies_bootloader.h>
#include <option/puppy_flash_fw.h>
#include <option/has_dwarf.h>
#include <option/has_modularbed.h>
#include <puppies/puppy_crash_dump.hpp>
#include <cstring>
#include <random.h>
#include "bsod.h"

LOG_COMPONENT_REF(Puppies);

namespace buddy::puppies {

using buddy::hw::Pin;

const char *PuppyBootstrap::Progress::description() {
    if (stage == PuppyBootstrap::FlashingStage::START) {
        return "Waking up puppies";
    } else if (stage == PuppyBootstrap::FlashingStage::DISCOVERY) {
        return "Looking for puppies";
    } else if (stage == PuppyBootstrap::FlashingStage::CALCULATE_FINGERPRINT) {
        return "Verifying puppies";
    }
#if HAS_DWARF()
    else if (stage == PuppyBootstrap::FlashingStage::CHECK_FINGERPRINT && puppy_type == PuppyType::DWARF) {
        return "Verifying dwarf";
    }
#endif
#if HAS_MODULARBED()
    else if (stage == PuppyBootstrap::FlashingStage::CHECK_FINGERPRINT && puppy_type == PuppyType::MODULARBED) {
        return "Verifying modularbed";
    }
#endif
#if HAS_DWARF()
    else if (stage == PuppyBootstrap::FlashingStage::FLASHING && puppy_type == PuppyType::DWARF) {
        return "Flashing dwarf";
    }
#endif
#if HAS_MODULARBED()
    else if (stage == PuppyBootstrap::FlashingStage::FLASHING && puppy_type == PuppyType::MODULARBED) {
        return "Flashing modularbed";
    }
#endif
    else if (stage == PuppyBootstrap::FlashingStage::DONE) {
        return ""; // Currently guimain prints nothing for the last bit of initialization, this should match
    } else {
        return "?";
    }
}

bool PuppyBootstrap::attempt_crash_dump_download(Dock dock, BootloaderProtocol::Address address) {
    flasher.set_address(address);
    std::array<uint8_t, BootloaderProtocol::MAX_RESPONSE_DATA_LEN> buffer;

    return crash_dump::download_dump_into_file(buffer, flasher,
        get_puppy_info(to_puppy_type(dock)).name,
        get_dock_info(dock).crash_dump_path);
}

PuppyBootstrap::BootstrapResult PuppyBootstrap::run(
    [[maybe_unused]] PuppyBootstrap::BootstrapResult minimal_config,
    [[maybe_unused]] unsigned int max_attempts) {
    PuppyBootstrap::BootstrapResult result;
#if HAS_DWARF()
    progressHook({ 0, FlashingStage::START, PuppyType::DWARF });
#elif HAS_MODULARBED()
    progressHook({ 0, FlashingStage::START, PuppyType::MODULARBED });
#endif
    auto guard = buddy::puppies::PuppyBus::LockGuard();

#if HAS_PUPPIES_BOOTLOADER()
    while (true) {
        reset_all_puppies();
        result = run_address_assignment();
        if (is_puppy_config_ok(result, minimal_config)) {
            // done, continue with bootstrap
            break;
        } else {
            // inadequate puppy config, will try again
            if (--max_attempts) {
                log_error(Puppies, "Not enough puppies discovered, will try again");
                continue;
            } else {
    #if HAS_DWARF()
                if (result.discovered_num() == 0) {
                    fatal_error(ErrCode::ERR_SYSTEM_PUPPY_DISCOVER_ERR);
                } else
    #endif
                {
                    // signal to user that puppy is not connected properly
                    auto get_first_missing_dock_string = [minimal_config, result]() -> const char * {
                        for (const auto dock : DOCKS) {
                            if (minimal_config.is_dock_occupied(dock) && !result.is_dock_occupied(dock)) {
                                return to_string(dock);
                            }
                        }
                        return "unknown";
                    };
                    fatal_error(ErrCode::ERR_SYSTEM_PUPPY_NOT_RESPONDING, get_first_missing_dock_string());
                }
            }
        }
    }
    #if HAS_DWARF()
    progressHook({ 10, FlashingStage::CALCULATE_FINGERPRINT, PuppyType::DWARF });
    #elif HAS_MODULARBED()
    progressHook({ 10, FlashingStage::CALCULATE_FINGERPRINT, PuppyType::MODULARBED });
    #endif
    int percent_per_puppy = 80 / result.discovered_num();
    int percent_base = 20;

    // Select random salt for modular bed and for dwarf
    fingerprints_t fingerprints;
    #if HAS_MODULARBED()
    fingerprints.get_salt(Dock::MODULAR_BED) = rand_u();
    #endif
    #if HAS_DWARF()
    fingerprints.get_salt(Dock::DWARF_1) = rand_u();
    for (const auto dock : DWARFS) {
        fingerprints.get_salt(dock) = fingerprints.get_salt(Dock::DWARF_1); // Copy salt to all dwarfs
    }
    #endif

    // Ask puppies to compute fw fingerprint
    for (const auto dock : DOCKS) {
        if (!result.is_dock_occupied(dock)) {
            // puppy not detected here, nothing to bootstrap
            continue;
        }
        auto address = get_boot_address_for_dock(dock);
        start_fingerprint_computation(address, fingerprints.get_salt(dock));
    }

    auto fingerprint_wait_start = ticks_ms();

    #if PUPPY_FLASH_FW()
    // Precompute firmware fingerprints
    { // Modular bed
        #if HAS_MODULARBED()
        unique_file_ptr fw_file = get_firmware(MODULARBED);
        off_t fw_size = get_firmware_size(MODULARBED);
        calculate_fingerprint(fw_file, fw_size, fingerprints.get_fingerprint(Dock::MODULAR_BED), fingerprints.get_salt(Dock::MODULAR_BED));
        #endif
    }
    { // Dwarf
        #if HAS_DWARF()
        unique_file_ptr fw_file = get_firmware(DWARF);
        off_t fw_size = get_firmware_size(DWARF);
        calculate_fingerprint(fw_file, fw_size, fingerprints.get_fingerprint(Dock::DWARF_1), fingerprints.get_salt(Dock::DWARF_1));
        for (Dock dock : DWARFS) {
            fingerprints.get_fingerprint(dock) = fingerprints.get_fingerprint(Dock::DWARF_1); // Copy fingerprint to all dwarfs
        }
        #endif
    }
    #endif /* PUPPY_FLASH_FW() */

    // Check puppies if they finished fingerprint calculations
    for (const auto dock : DOCKS) {
        if (!result.is_dock_occupied(dock)) {
            // puppy not detected here, nothing to check
            continue;
        }

        auto address = get_boot_address_for_dock(dock);
        flasher.set_address(address);
        wait_for_fingerprint(fingerprint_wait_start);

    #if !PUPPY_FLASH_FW()
        // Get fingerprint from puppies to start the app
        BootloaderProtocol::status_t result = flasher.get_fingerprint(fingerprints.get_fingerprint(dock));
        if (result != BootloaderProtocol::COMMAND_OK) {
            fatal_error(ErrCode::ERR_SYSTEM_PUPPY_FINGERPRINT_MISMATCH);
        }
    #endif /* !PUPPY_FLASH_FW() */
    }

    // Check fingerprints and flash firmware
    for (const auto dock : DOCKS) {
        if (!result.is_dock_occupied(dock)) {
            // puppy not detected here, nothing to bootstrap
            continue;
        }

        auto address = get_boot_address_for_dock(dock);
        auto puppy_type = to_puppy_type(dock);

        progressHook({ percent_base, FlashingStage::CHECK_FINGERPRINT, puppy_type });

        attempt_crash_dump_download(dock, address);
    #if PUPPY_FLASH_FW()
        uint8_t offset = 0;
        uint8_t size = sizeof(fingerprint_t);
        #if HAS_DWARF()
        if (to_puppy_type(dock) == DWARF) {
            // Check this chunk from one puppy, -1 fo modular bed which has different fingerprint
            size = sizeof(fingerprint_t) / (result.discovered_num() - 1);
            offset = size * (static_cast<uint8_t>(dock) - 1);
        }
        #endif
        flash_firmware(dock, fingerprints, offset, size, percent_base, percent_per_puppy);
    #endif
        percent_base += percent_per_puppy;
    }
    #if HAS_DWARF()
    progressHook({ 100, FlashingStage::DONE, PuppyType::DWARF });
    #elif HAS_MODULARBED()
    progressHook({ 100, FlashingStage::DONE, PuppyType::MODULARBED });
    #endif

    // Start application
    for (const auto dock : DOCKS) {
        if (!result.is_dock_occupied(dock)) {
            // puppy not detected here, nothing to start
            continue;
        }

        auto address = get_boot_address_for_dock(dock);
        auto puppy_type = to_puppy_type(dock);
        start_app(puppy_type, address, fingerprints.get_salt(dock), fingerprints.get_fingerprint(dock)); // Use last known salt that may already be calculated in puppy
    }

#else
    reset_all_puppies();
    result = MINIMAL_PUPPY_CONFIG;
#endif // HAS_PUPPIES_BOOTLOADER()

    return result;
}

bool PuppyBootstrap::is_puppy_config_ok(PuppyBootstrap::BootstrapResult result, PuppyBootstrap::BootstrapResult minimal_config) {
    // at least all bits that are set in minimal_config are set
    return (result.docks_preset & minimal_config.docks_preset) == minimal_config.docks_preset;
}

PuppyBootstrap::BootstrapResult PuppyBootstrap::run_address_assignment() {
    BootstrapResult result = {};

    for (auto dock = DOCKS.begin(); dock != DOCKS.end(); ++dock) {
        auto puppy_type = to_puppy_type(*dock);
        auto address = get_boot_address_for_dock(*dock);

        progressHook({ std::to_underlying(*dock), FlashingStage::START, puppy_type });

        progressHook({ 0, FlashingStage::DISCOVERY, puppy_type });
        log_info(Puppies, "Discovering whats in dock %s %d",
            get_puppy_info(puppy_type).name, std::to_underlying(*dock));

        // Wait for puppy to boot up
        osDelay(5);

        // assign address to all of them
        // this request is no-reply, so there is no issue in sending to multiple puppies
        assign_address(BootloaderProtocol::Address::DEFAULT_ADDRESS, address);

        // delay to make sure that command was sent fully before reset
        osDelay(50);

        // reset, all the not-bootstrapped-yet puppies which we don't care about now
        reset_puppies_range(std::next(dock), DOCKS.end());

        bool status = discover(puppy_type, address);
        if (status) {
            log_info(Puppies, "Dock %d: discovered puppy %s, assigned address: %d",
                std::to_underlying(*dock), get_puppy_info(puppy_type).name, address);
            result.set_dock_occupied(*dock);
        } else {
            log_info(Puppies, "Dock %d: no puppy discovered", std::to_underlying(*dock));
        }
    }

    verify_address_assignment(result);

    return result;
}

void PuppyBootstrap::assign_address(BootloaderProtocol::Address current_address, BootloaderProtocol::Address new_address) {
    auto status = flasher.assign_address(current_address, new_address);

    // this is no reply message - so failure is not expected, it would have to fail while writing message
    if (status != BootloaderProtocol::status_t::COMMAND_OK) {
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_ADDR_ASSIGN_ERR);
    }
}

void PuppyBootstrap::verify_address_assignment(BootstrapResult result) {
    // reset every puppy that is supposed to be empty
    for (auto dock = DOCKS.begin(); dock != DOCKS.end(); ++dock) {
        if (!result.is_dock_occupied(*dock)) {
            reset_puppies_range(dock, std::next(dock));
        }
    }

    // check if nobody still listens on address zero (ie if there is unassigned puppy)
    flasher.set_address(BootloaderProtocol::Address::DEFAULT_ADDRESS);
    uint16_t protocol_version;
    BootloaderProtocol::status_t status = flasher.get_protocolversion(protocol_version);
    if (status != BootloaderProtocol::status_t::NO_RESPONSE) {
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_NO_ADDR);
    }
}

void PuppyBootstrap::reset_all_puppies() {
    reset_puppies_range(DOCKS.begin(), DOCKS.end());
}

#define HAS_PIN_RESETABLE_PUPPIES() HAS_DWARF() || HAS_MODULARBED()

#if HAS_PIN_RESETABLE_PUPPIES()
constexpr bool is_pin_resetable(Dock dock) {
    const auto type = to_puppy_type(dock);
    switch (type) {
    #if HAS_DWARF()
    case DWARF:
        return true;
    #endif
    #if HAS_MODULARBED()
    case MODULARBED:
        return true;
    #endif
    default:
        return false;
    }
}

// FIXME: This decltype is ugly
inline decltype(buddy::hw::modularBedReset) &get_reset_pin(Dock dock) {
    switch (dock) {
    #if HAS_DWARF()
    case Dock::DWARF_1:
        return buddy::hw::dwarf1Reset;
    case Dock::DWARF_2:
        return buddy::hw::dwarf2Reset;
    case Dock::DWARF_3:
        return buddy::hw::dwarf3Reset;
    case Dock::DWARF_4:
        return buddy::hw::dwarf4Reset;
    case Dock::DWARF_5:
        return buddy::hw::dwarf5Reset;
    case Dock::DWARF_6:
        return buddy::hw::dwarf6Reset;
    #endif
    #if HAS_MODULARBED()
    case Dock::MODULAR_BED:
        return buddy::hw::modularBedReset;
    #endif
    default:
        std::abort();
    }
    std::unreachable();
}

#endif

void PuppyBootstrap::reset_puppies_range([[maybe_unused]] DockIterator begin, [[maybe_unused]] DockIterator end) {
#if HAS_PIN_RESETABLE_PUPPIES()
    const auto write_puppies_reset_pin = [](DockIterator dockFrom, DockIterator dockTo, Pin::State state) {
        for (auto dock = dockFrom; dock != dockTo; dock = std::next(dock)) {
            get_reset_pin(*dock).write(state);
        }
    };

    write_puppies_reset_pin(begin, end, Pin::State::high);
    osDelay(1);
    write_puppies_reset_pin(begin, end, Pin::State::low);
#endif
}

bool PuppyBootstrap::discover(PuppyType type, BootloaderProtocol::Address address) {
    flasher.set_address(address);

    auto check_status = [](BootloaderProtocol::status_t status) {
        if (status == BootloaderProtocol::status_t::NO_RESPONSE) {
            return false;
        } else if (status != BootloaderProtocol::status_t::COMMAND_OK) {
            log_error(Puppies, "Puppy discover error: %d", status);
            fatal_error(ErrCode::ERR_SYSTEM_PUPPY_DISCOVER_ERR);
        } else {
            return true;
        }
    };

    uint16_t protocol_version;
    if (check_status(flasher.get_protocolversion(protocol_version)) == false) {
        return false;
    }
    if ((protocol_version & 0xff00) != (BootloaderProtocol::BOOTLOADER_PROTOCOL_VERSION & 0xff00)) // Check major version of bootloader protocol version before anything else
    {
        log_error(Puppies, "Puppy uses incompatible bootloader protocol %04x, buddy wants %04x", protocol_version, BootloaderProtocol::BOOTLOADER_PROTOCOL_VERSION);
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_INCOMPATIBLE_BOOTLODER, protocol_version, BootloaderProtocol::BOOTLOADER_PROTOCOL_VERSION);
    }

    BootloaderProtocol::HwInfo hwinfo;
    // TODO: allow getting hwinfo for xbuddy ext, when we are able to read otp from it
    if (type != XBUDDY_EXTENSION) {
        if (check_status(flasher.get_hwinfo(hwinfo)) == false) {
            return false;
        }
    } else {
        hwinfo.hw_type = get_puppy_info(type).hw_info_hwtype;
        hwinfo.bl_version = MINIMAL_BOOTLOADER_VERSION;
    }

    // Here it is possible to read raw puppy's OTP before flashing, perhaps to flash a different firmware
    // TODO: allow reading otp from xbuddy ext board, when we are able to read it
    if (protocol_version >= 0x0302 && type != XBUDDY_EXTENSION) { // OTP read was added in protocol 0x0302

        uint8_t otp[32]; // OTP v5 will fit to 32 Bytes
        if (check_status(flasher.read_otp_cmd(0, otp, 32)) == false) {
            return false;
        }
        auto puppy_datamatrix = otp_parse_datamatrix(otp, sizeof(otp));
        if (puppy_datamatrix) {
            log_info(Puppies, "Puppy's hardware ID is %d with revision %d", puppy_datamatrix->product_id, puppy_datamatrix->revision);
        } else {
            log_warning(Puppies, "Puppy's hardware ID was not written properly to its OTP");
        }
    } // else - older bootloader has revision 0

    if (hwinfo.hw_type != get_puppy_info(type).hw_info_hwtype) {
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_UNKNOWN_TYPE);
    }
    if (hwinfo.bl_version < MINIMAL_BOOTLOADER_VERSION) {
        log_error(Puppies, "Puppy's bootloader is too old %04" PRIx32 " buddy wants %04" PRIx32, hwinfo.bl_version, MINIMAL_BOOTLOADER_VERSION);
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_INCOMPATIBLE_BOOTLODER, hwinfo.bl_version, MINIMAL_BOOTLOADER_VERSION);
    }

    // puppy responded, all is as expected
    return true;
}

void PuppyBootstrap::start_app([[maybe_unused]] PuppyType type, BootloaderProtocol::Address address, uint32_t salt, const fingerprint_t &fingerprint) {
    // start app
    log_info(Puppies, "Starting puppy app");
    flasher.set_address(address);
    BootloaderProtocol::status_t status = flasher.run_app(salt, fingerprint);
    if (status != BootloaderProtocol::COMMAND_OK) {
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_START_APP_ERR);
    }
}

unique_file_ptr PuppyBootstrap::get_firmware(PuppyType type) {
    const char *fw_path = get_puppy_info(type).fw_path;
    return unique_file_ptr(fopen(fw_path, "rb"));
}

off_t PuppyBootstrap::get_firmware_size(PuppyType type) {
    const char *fw_path = get_puppy_info(type).fw_path;

    struct stat fs;
    bool success = stat(fw_path, &fs) == 0;
    if (!success) {
        log_info(Puppies, "Firmware not found:  %s", fw_path);
        return 0;
    }

    return fs.st_size;
}

void PuppyBootstrap::flash_firmware(Dock dock, fingerprints_t &fw_fingerprints, uint8_t chunk_offset, uint8_t chunk_size, int percent_offset, int percent_span) {
    auto puppy_type = to_puppy_type(dock);
    unique_file_ptr fw_file = get_firmware(puppy_type);
    off_t fw_size = get_firmware_size(puppy_type);

    if (fw_size == 0 || fw_file.get() == nullptr) {
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_FW_NOT_FOUND, get_puppy_info(puppy_type).name);
        return;
    }

    flasher.set_address(get_boot_address_for_dock(dock));

    progressHook({ percent_offset, FlashingStage::CHECK_FINGERPRINT, puppy_type });

    bool match = fingerprint_match(fw_fingerprints.get_fingerprint(dock), chunk_offset, chunk_size);
    log_info(Puppies, "Puppy %d-%s fingerprint %s", static_cast<int>(dock), get_puppy_info(puppy_type).name, match ? "matched" : "didn't match");

    // if application firmware fingerprint doesn't match, flash it
    if (!match) {

        const struct {
            unique_file_ptr &fw_file;
            off_t fw_size;
            int percent_offset;
            int percent_span;
            PuppyType puppy_type;
        } params {
            .fw_file = fw_file,
            .fw_size = fw_size,
            .percent_offset = percent_offset,
            .percent_span = percent_span,
            .puppy_type = puppy_type,
        };

        BootloaderProtocol::status_t result = flasher.write_flash(fw_size, [this, &params](uint32_t offset, size_t size, uint8_t *out_data) -> bool {
            // update GUI progress bar
            this->progressHook({ static_cast<int>(params.percent_offset + offset * params.percent_span / params.fw_size), FlashingStage::FLASHING, params.puppy_type });
            log_info(Puppies, "Flashing puppy %s offset %" PRIu32 "/%ld", get_puppy_info(params.puppy_type).name, offset, params.fw_size);

            // get data
            assert(offset + size <= static_cast<size_t>(params.fw_size));
            int sret = fseek(params.fw_file.get(), offset, SEEK_SET);
            assert(sret == 0);
            UNUSED(sret);
            unsigned int ret = fread(out_data, sizeof(uint8_t), size, params.fw_file.get());
            assert(ret == size);
            UNUSED(ret);

            return true;
        });

        if (result != BootloaderProtocol::COMMAND_OK) {
            fatal_error(ErrCode::ERR_SYSTEM_PUPPY_WRITE_FLASH_ERR, get_puppy_info(puppy_type).name);
        }

        progressHook({ percent_offset + percent_span, FlashingStage::CHECK_FINGERPRINT, puppy_type });

        // Calculate new fingerprint, salt needs to be changed so the flashing cannot be faked
        fw_fingerprints.get_salt(dock) = rand_u();
        start_fingerprint_computation(get_boot_address_for_dock(dock), fw_fingerprints.get_salt(dock));

        auto fingerprint_wait_start = ticks_ms();

        calculate_fingerprint(fw_file, fw_size, fw_fingerprints.get_fingerprint(dock), fw_fingerprints.get_salt(dock));

        // Check puppy if it finished fingerprint calculation
        wait_for_fingerprint(fingerprint_wait_start);

        // check fingerprint after flashing, to make sure it went well
        if (!fingerprint_match(fw_fingerprints.get_fingerprint(dock))) {
            fatal_error(ErrCode::ERR_SYSTEM_PUPPY_FINGERPRINT_MISMATCH, get_puppy_info(puppy_type).name);
        }
    }
}

void PuppyBootstrap::wait_for_fingerprint(uint32_t calculation_start) {
    constexpr uint32_t WAIT_TIME = 1000; // Puppies should calculate fingerprint in 330 ms, but it all takes almost 600 ms
    uint16_t protocol_version;

    while (1) {
        BootloaderProtocol::status_t status = flasher.get_protocolversion(protocol_version); // Test if puppy is communicating

        if (status == BootloaderProtocol::status_t::COMMAND_OK) // Any response from puppy means it is ready
        {
            return; // Done
        }

        if (ticks_diff(calculation_start + WAIT_TIME, ticks_ms()) < 0) {
            fatal_error(ErrCode::ERR_SYSTEM_PUPPY_FINGERPRINT_TIMEOUT);
        }

        osDelay(50); // Wait between attempts
    }
}

void PuppyBootstrap::calculate_fingerprint(unique_file_ptr &file, off_t fw_size, fingerprint_t &fingerprint, uint32_t salt) {
    int ret = fseek(file.get(), 0, SEEK_SET);
    assert(ret == 0);
    UNUSED(ret);

    mbedtls_sha256_context sha;

    mbedtls_sha256_init(&sha);
    mbedtls_sha256_starts_ret(&sha, 0);

    mbedtls_sha256_update_ret(&sha, reinterpret_cast<uint8_t *>(&salt), sizeof(salt)); // Add salt

    uint8_t buffer[128];
    while (fw_size > 0 && !feof(file.get())) {
        if (int read = fread(buffer, 1, std::min(fw_size, static_cast<off_t>(sizeof(buffer))), file.get());
            read > 0) {
            mbedtls_sha256_update_ret(&sha, buffer, read);
            fw_size -= read;
        }
        if (ferror(file.get())) {
            fatal_error(ErrCode::ERR_SYSTEM_PUPPY_FINGERPRINT_MISMATCH);
        }
    }

    mbedtls_sha256_finish_ret(&sha, fingerprint.data());
    mbedtls_sha256_free(&sha);
}

bool PuppyBootstrap::fingerprint_match(const fingerprint_t &fingerprint, uint8_t offset, uint8_t size) {
    if ((offset + size) > sizeof(fingerprint)) {
        return false;
    }

    // read current firmware fingerprint
    fingerprint_t read_fingerprint = { 0 };
    BootloaderProtocol::status_t result = flasher.get_fingerprint(read_fingerprint, offset, size);
    if (result != BootloaderProtocol::COMMAND_OK) {
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_FINGERPRINT_MISMATCH);
    }

    return (std::memcmp(&read_fingerprint.data()[offset], &fingerprint.data()[offset], size) == 0); // Compare requested chunk
}

void PuppyBootstrap::start_fingerprint_computation(BootloaderProtocol::Address address, uint32_t salt) {
    flasher.set_address(address);
    flasher.compute_fingerprint(salt);
}

} // namespace buddy::puppies

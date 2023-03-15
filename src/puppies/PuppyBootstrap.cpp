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
#include "log.h"
#include "main.h"
#include "tasks.h"
#include "timing.h"
#include "bsod.h"
#include "otp.h"
#include <option/has_puppies_bootloader.h>
#include <option/puppy_flash_fw.h>
#include <puppies/puppy_crash_dump.hpp>
#include <cstring>
#include "bsod_gui.hpp"

LOG_COMPONENT_REF(Puppies);

namespace buddy::puppies {

using buddy::hw::Pin;

const char *PuppyBootstrap::Progress::description() {
    if (stage == PuppyBootstrap::FlashingStage::START)
        return "Waking up puppies";
    else if (stage == PuppyBootstrap::FlashingStage::DISCOVERY)
        return "Looking for puppies";
    else if (stage == PuppyBootstrap::FlashingStage::CALCULATE_FINGERPRINT)
        return "Verifying puppies";
    else if (stage == PuppyBootstrap::FlashingStage::CHECK_FINGERPRINT && puppy_type == PuppyType::DWARF)
        return "Verifying dwarf";
    else if (stage == PuppyBootstrap::FlashingStage::CHECK_FINGERPRINT && puppy_type == PuppyType::MODULARBED)
        return "Verifying modularbed";
    else if (stage == PuppyBootstrap::FlashingStage::FLASHING && puppy_type == PuppyType::DWARF)
        return "Flashing dwarf";
    else if (stage == PuppyBootstrap::FlashingStage::FLASHING && puppy_type == PuppyType::MODULARBED)
        return "Flashing modularbed";
    else if (stage == PuppyBootstrap::FlashingStage::DONE)
        return ""; // Currently guimain prints nothing for the last bit of initialization, this should match
    else
        return "?";
}

PuppyBootstrap::PuppyBootstrap(ProgressHook aprogressHook)
    : progressHook(aprogressHook) {
}

bool PuppyBootstrap::attempt_crash_dump_download(Kennel kennel, BootloaderProtocol::Address address) {
    flasher.set_address(address);
    std::array<uint8_t, BootloaderProtocol::MAX_RESPONSE_DATA_LEN> buffer;

    return crash_dump::download_dump_into_file(buffer, flasher,
        puppy_info[to_puppy_type(kennel)].name,
        kennel_info[to_info_idx(kennel)].crash_dump_path);
}

PuppyBootstrap::BootstrapResult PuppyBootstrap::run(PuppyBootstrap::BootstrapResult minimal_config, unsigned int max_attempts) {
    PuppyBootstrap::BootstrapResult result;

    progressHook({ 0, FlashingStage::START });
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
                if (result.discovered_num() == 0) {
                    fatal_error(ErrCode::ERR_SYSTEM_PUPPY_DISCOVER_ERR);
                } else {
                    // signal to user that puppy is not connected properly
                    auto get_first_missing_kennel_string = [minimal_config, result]() -> const char * {
                        for (Kennel kennel = Kennel::FIRST; kennel <= Kennel::LAST; kennel = kennel + 1) {
                            if (minimal_config.is_kennel_occupied(kennel) && !result.is_kennel_occupied(kennel)) {
                                return to_string(kennel);
                            }
                        }
                        return "unknown";
                    };
                    fatal_error(ErrCode::ERR_SYSTEM_PUPPY_NOT_RESPONDING, get_first_missing_kennel_string());
                }
            }
        }
    }

    progressHook({ 10, FlashingStage::CALCULATE_FINGERPRINT });
    int percent_per_puppy = 80 / result.discovered_num();
    int percent_base = 20;

    //Select random salt for modular bed and for dwarf
    fingerprints_t fingerprints;
    HAL_RNG_GenerateRandomNumber(&hrng, &(fingerprints.get_salt(Kennel::MODULAR_BED)));
    HAL_RNG_GenerateRandomNumber(&hrng, &(fingerprints.get_salt(Kennel::DWARF_1)));
    for (Kennel kennel = Kennel::DWARF_1; kennel <= Kennel::LAST; kennel = kennel + 1) {
        fingerprints.get_salt(kennel) = fingerprints.get_salt(Kennel::DWARF_1); // Copy salt to all dwarfs
    }

    // Ask puppies to compute fw fingerprint
    for (Kennel kennel = Kennel::FIRST; kennel <= Kennel::LAST; kennel = kennel + 1) {
        if (!result.is_kennel_occupied(kennel)) {
            // puppy not detected here, nothing to bootstrap
            continue;
        }
        auto address = get_boot_address_for_kennel(kennel);
        start_fingerprint_computation(address, fingerprints.get_salt(kennel));
    }

    auto fingerprint_wait_start = ticks_ms();

    // Precompute firmware fingerprints
    { // Modular bed
        unique_file_ptr fw_file = get_firmware(MODULARBED);
        off_t fw_size = get_firmware_size(MODULARBED);
        calculate_fingerprint(fw_file, fw_size, fingerprints.get_fingerprint(Kennel::MODULAR_BED), fingerprints.get_salt(Kennel::MODULAR_BED));
    }
    { // Dwarf
        unique_file_ptr fw_file = get_firmware(DWARF);
        off_t fw_size = get_firmware_size(DWARF);
        calculate_fingerprint(fw_file, fw_size, fingerprints.get_fingerprint(Kennel::DWARF_1), fingerprints.get_salt(Kennel::DWARF_1));
        for (Kennel kennel = Kennel::DWARF_1; kennel <= Kennel::LAST; kennel = kennel + 1) {
            fingerprints.get_fingerprint(kennel) = fingerprints.get_fingerprint(Kennel::DWARF_1); // Copy fingerprint to all dwarfs
        }
    }

    // Check puppies if they finished fingerprint calculations
    for (Kennel kennel = Kennel::FIRST; kennel <= Kennel::LAST; kennel = kennel + 1) {
        if (!result.is_kennel_occupied(kennel)) {
            // puppy not detected here, nothing to check
            kennel = kennel + 1; // Check next puppy
            continue;
        }

        auto address = get_boot_address_for_kennel(kennel);
        flasher.set_address(address);
        wait_for_fingerprint(fingerprint_wait_start);
    }

    // Check fingerprints and flash firmware
    for (Kennel kennel = Kennel::FIRST; kennel <= Kennel::LAST; kennel = kennel + 1) {
        if (!result.is_kennel_occupied(kennel)) {
            // puppy not detected here, nothing to bootstrap
            continue;
        }

        auto address = get_boot_address_for_kennel(kennel);
        auto puppy_type = to_puppy_type(kennel);

        progressHook({ percent_base, FlashingStage::CHECK_FINGERPRINT, puppy_type });

        attempt_crash_dump_download(kennel, address);
    #if PUPPY_FLASH_FW()
        uint8_t offset = 0;
        uint8_t size = sizeof(fingerprint_t);
        if (to_puppy_type(kennel) == DWARF) {
            // Check this chunk from one puppy, -1 fo modular bed which has different fingerprint
            size = sizeof(fingerprint_t) / (result.discovered_num() - 1);
            offset = size * (static_cast<uint8_t>(kennel) - 1);
        }
        flash_firmware(kennel, fingerprints, offset, size, percent_base, percent_per_puppy);
    #endif
        percent_base += percent_per_puppy;
    }

    progressHook({ 100, FlashingStage::DONE });

    // Start application
    for (Kennel kennel = Kennel::FIRST; kennel <= Kennel::LAST; kennel = kennel + 1) {
        if (!result.is_kennel_occupied(kennel)) {
            // puppy not detected here, nothing to start
            continue;
        }

        auto address = get_boot_address_for_kennel(kennel);
        auto puppy_type = to_puppy_type(kennel);
        start_app(puppy_type, address, fingerprints.get_salt(kennel), fingerprints.get_fingerprint(kennel)); //Use last known salt that may already be calculated in puppy
    }

#else
    reset_all_puppies();
    result = MINIMAL_PUPPY_CONFIG;
#endif

    return result;
}

bool PuppyBootstrap::is_puppy_config_ok(PuppyBootstrap::BootstrapResult result, PuppyBootstrap::BootstrapResult minimal_config) {
    // at least all bits that are set in minimal_config are set
    return (result.kennels_preset & minimal_config.kennels_preset) == minimal_config.kennels_preset;
}

PuppyBootstrap::BootstrapResult PuppyBootstrap::run_address_assignment() {
    BootstrapResult result = {};

    for (Kennel kennel = Kennel::FIRST; kennel <= Kennel::LAST; kennel = kennel + 1U) {
        auto address = get_boot_address_for_kennel(kennel);
        auto puppy_type = to_puppy_type(kennel);

        progressHook({ (int)kennel, FlashingStage::START });

        progressHook({ 0, FlashingStage::DISCOVERY, puppy_type });
        log_info(Puppies, "Discovering whats in kennel %s %i", puppy_info[puppy_type].name, kennel);

        // Wait for puppy to boot up
        osDelay(5);

        // assign address to all of them
        // this request is no-reply, so there is no issue in sending to multiple puppies
        assign_address(BootloaderProtocol::Address::DEFAULT_ADDRESS, address);

        // delay to make sure that command was sent fully before reset
        osDelay(50);

        // reset, all the not-bootstrapped-yet puppies which we don't care about now
        reset_puppies_range(kennel + 1U, Kennel::LAST);

        bool status = discover(puppy_type, address);
        if (status) {
            log_info(Puppies, "Kennel %i: discovered puppy %s, assigned address: %d", kennel, puppy_info[puppy_type].name, address);
            result.set_kennel_occupied(kennel);
        } else {
            log_info(Puppies, "Kennel %i: no puppy discovered", kennel);
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
    for (Kennel kennel = Kennel::FIRST; kennel <= Kennel::LAST; kennel = kennel + 1U) {
        if (!result.is_kennel_occupied(kennel)) {
            reset_puppies_range(kennel, kennel);
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
    reset_puppies_range(Kennel::FIRST, Kennel::LAST);
}

void PuppyBootstrap::reset_puppies_range(Kennel from, Kennel to) {
    const auto write_puppies_reset_pin = [](Kennel kennelFrom, Kennel to, Pin::State state) {
        static const buddy::hw::PCA9557OutputPin *const reset_pins[] = {
            &buddy::hw::modularBedReset,
            &buddy::hw::dwarf1Reset,
            &buddy::hw::dwarf2Reset,
            &buddy::hw::dwarf3Reset,
            &buddy::hw::dwarf4Reset,
            &buddy::hw::dwarf5Reset,
            &buddy::hw::dwarf6Reset,
        };
        for (Kennel k = kennelFrom; k <= to; k = k + 1U) {
            reset_pins[static_cast<uint8_t>(k)]->write(state);
        }
    };

    write_puppies_reset_pin(from, to, Pin::State::high);
    osDelay(1);
    write_puppies_reset_pin(from, to, Pin::State::low);
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
    if (check_status(flasher.get_hwinfo(hwinfo)) == false) {
        return false;
    }

    // Here it is possible to read raw puppy's OTP before flashing, perhaps to flash a different firmware
    datamatrix_t puppy_datamatrix = { 0 };
    if (protocol_version >= 0x0302) { // OTP read was added in protocol 0x0302

        uint8_t otp[32]; // OTP v5 will fit to 32 Bytes
        if (check_status(flasher.read_otp_cmd(0, otp, 32)) == false) {
            return false;
        }

        if (otp_parse_datamatrix(&puppy_datamatrix, otp, sizeof(otp)) == false) {
            log_warning(Puppies, "Puppy's hardware ID was not written properly to its OTP");
        }
    } // else - older bootloader has revision 0
    log_info(Puppies, "Puppy's hardware ID is %d with revision %d", puppy_datamatrix.product_id, puppy_datamatrix.revision);

    if (hwinfo.hw_type != puppy_info[type].hw_info_hwtype) {
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_UNKNOWN_TYPE);
    }
    if (hwinfo.bl_version < MINIMAL_BOOTLOADER_VERSION) {
        log_error(Puppies, "Puppy's bootloader is too old %04x, buddy wants %04x", hwinfo.bl_version, MINIMAL_BOOTLOADER_VERSION);
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_INCOMPATIBLE_BOOTLODER, hwinfo.bl_version, MINIMAL_BOOTLOADER_VERSION);
    }

    // puppy responded, all is as expected
    return true;
}

void PuppyBootstrap::start_app(PuppyType type, BootloaderProtocol::Address address, uint32_t salt, const fingerprint_t &fingerprint) {
    // start app
    log_info(Puppies, "Starting puppy app");
    flasher.set_address(address);
    BootloaderProtocol::status_t status = flasher.run_app(salt, fingerprint);
    if (status != BootloaderProtocol::COMMAND_OK) {
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_START_APP_ERR);
    }
}

unique_file_ptr PuppyBootstrap::get_firmware(PuppyType type) {
    const char *fw_path = puppy_info[type].fw_path;
    return unique_file_ptr(fopen(fw_path, "rb"));
}

off_t PuppyBootstrap::get_firmware_size(PuppyType type) {
    const char *fw_path = puppy_info[type].fw_path;

    struct stat fs;
    bool success = stat(fw_path, &fs) == 0;
    if (!success) {
        log_info(Puppies, "Firmware not found:  %s", fw_path);
        return 0;
    }

    return fs.st_size;
}

void PuppyBootstrap::flash_firmware(Kennel kennel, fingerprints_t &fw_fingerprints, uint8_t chunk_offset, uint8_t chunk_size, int percent_offset, int percent_span) {
    auto puppy_type = to_puppy_type(kennel);
    unique_file_ptr fw_file = get_firmware(puppy_type);
    off_t fw_size = get_firmware_size(puppy_type);

    if (fw_size == 0 || fw_file.get() == nullptr) {
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_FW_NOT_FOUND, puppy_info[puppy_type].name);
        return;
    }

    flasher.set_address(get_boot_address_for_kennel(kennel));

    progressHook({ percent_offset, FlashingStage::CHECK_FINGERPRINT, puppy_type });

    bool match = fingerprint_match(fw_fingerprints.get_fingerprint(kennel), chunk_offset, chunk_size);
    log_info(Puppies, "Puppy %d-%s fingerprint %s", static_cast<int>(kennel), puppy_info[puppy_type].name, match ? "matched" : "didn't match");

    // if application firmware fingerprint doesn't match, flash it
    if (!match) {
        BootloaderProtocol::status_t result = flasher.write_flash(fw_size, [fw_size, &fw_file, puppy_type, this, percent_offset, percent_span](uint32_t offset, size_t size, uint8_t *out_data) -> bool {
            // update GUI progress bar
            this->progressHook({ static_cast<int>(percent_offset + offset * percent_span / fw_size), FlashingStage::FLASHING, puppy_type });
            log_info(Puppies, "Flashing puppy %s offset %d/%d", puppy_info[puppy_type].name, offset, fw_size);

            // get data
            assert(offset + size <= static_cast<size_t>(fw_size));
            int sret = fseek(fw_file.get(), offset, SEEK_SET);
            assert(sret == 0);
            UNUSED(sret);
            unsigned int ret = fread(out_data, sizeof(uint8_t), size, fw_file.get());
            assert(ret == size);
            UNUSED(ret);

            return true;
        });

        if (result != BootloaderProtocol::COMMAND_OK) {
            fatal_error(ErrCode::ERR_SYSTEM_PUPPY_WRITE_FLASH_ERR, puppy_info[puppy_type].name);
        }

        progressHook({ percent_offset + percent_span, FlashingStage::CHECK_FINGERPRINT, puppy_type });

        // Calculate new fingerprint, salt needs to be changed so the flashing cannot be faked
        HAL_RNG_GenerateRandomNumber(&hrng, &(fw_fingerprints.get_salt(kennel)));
        start_fingerprint_computation(get_boot_address_for_kennel(kennel), fw_fingerprints.get_salt(kennel));

        auto fingerprint_wait_start = ticks_ms();

        calculate_fingerprint(fw_file, fw_size, fw_fingerprints.get_fingerprint(kennel), fw_fingerprints.get_salt(kennel));

        // Check puppy if it finished fingerprint calculation
        wait_for_fingerprint(fingerprint_wait_start);

        // check fingerprint after flashing, to make sure it went well
        if (!fingerprint_match(fw_fingerprints.get_fingerprint(kennel))) {
            fatal_error(ErrCode::ERR_SYSTEM_PUPPY_FINGERPRINT_MISMATCH, puppy_info[puppy_type].name);
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

    return (std::memcmp(&read_fingerprint.data()[offset], &fingerprint.data()[offset], size) == 0); //Compare requested chunk
}

void PuppyBootstrap::start_fingerprint_computation(BootloaderProtocol::Address address, uint32_t salt) {
    flasher.set_address(address);
    flasher.compute_fingerprint(salt);
}

}

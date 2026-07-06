#include "puppies/PuppyBootstrap.hpp"
#include "puppies/BootloaderProtocol.hpp"
#include <bsod.h>
#include <sys/stat.h>
#include "assert.h"
#include <logging/log.hpp>
#include <buddy/bootstrap_state.hpp>
#include <buddy/digest.hpp>
#include <buddy/main.h>
#include <freertos/timing.hpp>
#include "timing.h"
#include <modbus/server_address.hpp>
#include "otp.hpp"
#include <option/has_puppies_bootloader.h>
#include <option/puppy_flash_fw.h>
#include <option/has_dwarf.h>
#include <option/has_puppy_modularbed.h>
#include <option/has_xbuddy_extension.h>
#include <puppies/puppy_crash_dump.hpp>
#include <option/has_indx_head.h>
#include <cstring>
#include <random/random.h>

#if HAS_INDX_HEAD()
    #include <hw_configuration.hpp>
    #include <puppies/INDX.hpp>
#endif

LOG_COMPONENT_REF(Puppies);

namespace buddy::puppies {

using buddy::hw::Pin;

using buddy::BootstrapStage;

static BootstrapStage flashing_stage(PuppyType puppy_type) {
    switch (puppy_type) {
    case DWARF:
#if HAS_DWARF()
        return BootstrapStage::flashing_dwarf;
#else
        break;
#endif
    case MODULARBED:
#if HAS_PUPPY_MODULARBED()
        return BootstrapStage::flashing_modular_bed;
#else
        break;
#endif
    case INDX_HEAD:
#if HAS_INDX_HEAD()
        return BootstrapStage::flashing_indx_head;
#else
        break;
#endif
    }
    bsod_unreachable();
}

static BootstrapStage check_fingerprint_stage(PuppyType puppy_type) {
    switch (puppy_type) {
    case DWARF:
#if HAS_DWARF()
        return BootstrapStage::verifying_dwarf;
#else
        break;
#endif
    case MODULARBED:
#if HAS_PUPPY_MODULARBED()
        return BootstrapStage::verifying_modular_bed;
#else
        break;
#endif
    case INDX_HEAD:
#if HAS_INDX_HEAD()
        return BootstrapStage::verifying_indx_head;
#else
        break;
#endif
    }
    bsod_unreachable();
}

bool PuppyBootstrap::any_dock_supports_crash_dump() {
    return std::ranges::any_of(DOCKS, [](Dock dock) { return get_crash_dump_path(dock).has_value(); });
}

bool PuppyBootstrap::attempt_crash_dump_download(Dock dock, BootloaderProtocol::Address address) {
    const auto crash_dump_path = get_crash_dump_path(dock);
    if (!crash_dump_path) {
        return false;
    }

    flasher.set_address(address);
    std::array<uint8_t, BootloaderProtocol::MAX_RESPONSE_DATA_LEN> buffer;

    return crash_dump::download_dump_into_file(buffer, flasher,
        get_puppy_info(to_puppy_type(dock)).name,
        *crash_dump_path);
}

PuppyBootstrap::BootstrapResult PuppyBootstrap::run(
    [[maybe_unused]] PuppyBootstrap::BootstrapResult minimal_config,
    [[maybe_unused]] unsigned int max_attempts) {
    PuppyBootstrap::BootstrapResult result;
    bootstrap_state_set(0, BootstrapStage::waking_up_puppies);

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
    bootstrap_state_set(10, BootstrapStage::verifying_puppies);
    int percent_per_puppy = 80 / result.discovered_num();
    int percent_base = 20;

    // Select random salt for modular bed and for dwarf
    fingerprints_t fingerprints;
    for (const auto dock : DOCKS) {
        if (to_puppy_type(dock) == DWARF && dock != Dock::DWARF_1) {
            fingerprints.get_salt(dock) = fingerprints.get_salt(Dock::DWARF_1);
        } else {
            fingerprints.get_salt(dock) = rand_u();
        }
    }

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
    for (const auto dock : DOCKS) {
        const auto puppy_type = to_puppy_type(dock);
        if (puppy_type == DWARF && dock != Dock::DWARF_1) {
            fingerprints.get_fingerprint(dock) = fingerprints.get_fingerprint(Dock::DWARF_1);
        } else {
            unique_file_ptr fw_file = get_firmware(puppy_type);
            calculate_fingerprint(fw_file, fingerprints.get_fingerprint(dock), fingerprints.get_salt(dock));
        }
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
        // #error dead code found by automatic analyses (see BFW-5461)
        // Get fingerprint from puppies to start the app
        BootloaderProtocol::status_t result = flasher.get_fingerprint(fingerprints.get_fingerprint(dock));
        if (result != BootloaderProtocol::COMMAND_OK) {
            fatal_error(ErrCode::ERR_SYSTEM_PUPPY_NOT_RESPONDING, to_string(dock));
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

        bootstrap_state_set(percent_base, check_fingerprint_stage(puppy_type));

        attempt_crash_dump_download(dock, address);
    #if PUPPY_FLASH_FW()
        flash_firmware(dock, fingerprints, percent_base, percent_per_puppy);
    #endif
        percent_base += percent_per_puppy;
    }

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
    // #error dead code found by automatic analyses (see BFW-5461)
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

        bootstrap_state_set(0, BootstrapStage::looking_for_puppies);
        log_info(Puppies, "Discovering whats in dock %s %d",
            get_puppy_info(puppy_type).name, std::to_underlying(*dock));

        // Wait for puppy to boot up
        osDelay(5);

        if (is_dynamicly_addressable(puppy_type)) {
            // assign address to all of them
            // this request is no-reply, so there is no issue in sending to multiple puppies
            assign_address(BootloaderProtocol::Address::DEFAULT_ADDRESS, address);

            // delay to make sure that command was sent fully before reset
            osDelay(10);

            // reset, all the not-bootstrapped-yet puppies which we don't care about now
            reset_puppies_range(std::next(dock), DOCKS.end());
            osDelay(5);
        }

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

inline void write_dock_reset_pin(Dock dock, buddy::hw::Pin::State state) {
    using namespace buddy::hw;
    switch (dock) {
#if HAS_DWARF()
    case Dock::DWARF_1:
        dwarf1Reset.write(state);
        break;
    case Dock::DWARF_2:
        dwarf2Reset.write(state);
        break;
    case Dock::DWARF_3:
        dwarf3Reset.write(state);
        break;
    case Dock::DWARF_4:
        dwarf4Reset.write(state);
        break;
    case Dock::DWARF_5:
        dwarf5Reset.write(state);
        break;
    case Dock::DWARF_6:
        dwarf6Reset.write(state);
        break;
#endif
#if HAS_PUPPY_MODULARBED()
    case Dock::MODULAR_BED:
        modular_bed_reset.write(state);
        break;
#endif
#if HAS_INDX_HEAD()
    case Dock::INDX_HEAD:
        // Reset polarity differs between board revisions, Configuration handles that
        if (state == Pin::State::high) {
            Configuration::Instance().activate_indx_head_reset();
        } else {
            Configuration::Instance().deactivate_indx_head_reset();
        }
        break;
#endif
    default:
        std::abort();
    }
}

void PuppyBootstrap::reset_puppies_range(DockIterator begin, DockIterator end) {
    const auto write_puppies_reset_pin = [](DockIterator dockFrom, DockIterator dockTo, Pin::State state) {
        for (auto dock = dockFrom; dock != dockTo; dock = std::next(dock)) {
            write_dock_reset_pin(*dock, state);
        }
    };

    write_puppies_reset_pin(begin, end, Pin::State::high);
    osDelay(10);
    write_puppies_reset_pin(begin, end, Pin::State::low);
}

bool PuppyBootstrap::discover_once(PuppyType type, BootloaderProtocol::Address address) {
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
    if (protocol_version >= 0x0302) { // OTP read was added in protocol 0x0302

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

#if HAS_INDX_HEAD()
        if (type == INDX_HEAD) {
            indx.set_otp(*reinterpret_cast<const OTP_v5 *>(otp));
        }
#endif
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

bool PuppyBootstrap::discover(PuppyType type, BootloaderProtocol::Address address) {
    const auto timeout_ms = 1000;
    const auto start_ms = ticks_ms();
    for (;;) {
        if (discover_once(type, address)) {
            return true;
        }
        if (ticks_diff(ticks_ms(), start_ms) > timeout_ms) {
            return false;
        }
    }
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

void PuppyBootstrap::flash_firmware(Dock dock, fingerprints_t &fw_fingerprints, int percent_offset, int percent_span) {
    auto puppy_type = to_puppy_type(dock);
    unique_file_ptr fw_file = get_firmware(puppy_type);
    off_t fw_size = get_firmware_size(puppy_type);

    if (fw_size == 0 || fw_file.get() == nullptr) {
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_FW_NOT_FOUND, get_puppy_info(puppy_type).name);
        return;
    }

    flasher.set_address(get_boot_address_for_dock(dock));

    bootstrap_state_set(percent_offset, check_fingerprint_stage(puppy_type));

    bool match = fingerprint_match(fw_fingerprints.get_fingerprint(dock), dock);
    log_info(Puppies, "Puppy %d-%s fingerprint %s", static_cast<int>(dock), get_puppy_info(puppy_type).name, match ? "matched" : "didn't match");

    // if application firmware fingerprint doesn't match, flash it
    if (!match) {

        const struct {
            unique_file_ptr &fw_file;
            off_t fw_size;
            int percent_offset;
            int percent_span;
            BootstrapStage flashing_stage;
        } params {
            .fw_file = fw_file,
            .fw_size = fw_size,
            .percent_offset = percent_offset,
            .percent_span = percent_span,
            .flashing_stage = flashing_stage(puppy_type),
        };

        BootloaderProtocol::status_t result = flasher.write_flash(fw_size, [&params](uint32_t offset, size_t size, uint8_t *out_data) -> bool {
            const uint8_t percent = static_cast<uint8_t>(params.percent_offset + offset * params.percent_span / params.fw_size);
            bootstrap_state_set(percent, params.flashing_stage);

            // get data
            assert(offset + size <= static_cast<size_t>(params.fw_size));
            const int sret = fseek(params.fw_file.get(), offset, SEEK_SET);
            if (sret != 0) {
                return false;
            }
            const size_t ret = fread(out_data, sizeof(uint8_t), size, params.fw_file.get());
            if (ret != size) {
                return false;
            }
            return true;
        });

        if (result != BootloaderProtocol::COMMAND_OK) {
            fatal_error(ErrCode::ERR_SYSTEM_PUPPY_WRITE_FLASH_ERR, get_puppy_info(puppy_type).name);
        }

        bootstrap_state_set(percent_offset + percent_span, check_fingerprint_stage(puppy_type));

        // Calculate new fingerprint, salt needs to be changed so the flashing cannot be faked
        fw_fingerprints.get_salt(dock) = rand_u();
        start_fingerprint_computation(get_boot_address_for_dock(dock), fw_fingerprints.get_salt(dock));

        auto fingerprint_wait_start = ticks_ms();

        calculate_fingerprint(fw_file, fw_fingerprints.get_fingerprint(dock), fw_fingerprints.get_salt(dock));

        // Check puppy if it finished fingerprint calculation
        wait_for_fingerprint(fingerprint_wait_start);

        // check fingerprint after flashing, to make sure it went well
        if (!fingerprint_match(fw_fingerprints.get_fingerprint(dock), dock)) {
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

        freertos::yield();
    }
}

void PuppyBootstrap::calculate_fingerprint(unique_file_ptr &file, fingerprint_t &fingerprint, uint32_t salt) {
    Digest digest {
        (std::byte *)fingerprint.data(),
        fingerprint.size(),
    };
    buddy::compute_file_digest_with_retry(fileno(file.get()), salt, digest);
}

bool PuppyBootstrap::fingerprint_match(const fingerprint_t &fingerprint, Dock dock) {
    // read current firmware fingerprint
    fingerprint_t read_fingerprint = { 0 };
    BootloaderProtocol::status_t result = flasher.get_fingerprint(read_fingerprint);
    if (result != BootloaderProtocol::COMMAND_OK) {
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_NOT_RESPONDING, to_string(dock));
    }

    return read_fingerprint == fingerprint;
}

void PuppyBootstrap::start_fingerprint_computation(BootloaderProtocol::Address address, uint32_t salt) {
    flasher.set_address(address);
    flasher.compute_fingerprint(salt);
}

constexpr bool is_equal_address(Dock dock, modbus::ServerAddress server_address) {
    return std::to_underlying(PuppyBootstrap::get_modbus_address_for_dock(dock)) == std::to_underlying(server_address);
}

static_assert(is_equal_address(Dock::XBUDDY_EXTENSION, modbus::ServerAddress::xbuddy_extension));

} // namespace buddy::puppies

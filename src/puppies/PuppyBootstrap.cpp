#include "puppies/PuppyBootstrap.hpp"
#include "puppies/BootloaderProtocol.hpp"
#include <bsod.h>
#include <sys/stat.h>
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
#include <option/has_xl_can.h>
#include <cstring>
#include <random/random.h>

#if HAS_INDX_HEAD()
    #include <hw/xbuddy/hw_configuration.hpp>
    #include <puppies/INDX.hpp>
#endif

#if HAS_XL_CAN()
    #include <puppies/xl_can.hpp>
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
        const auto assignment = run_address_assignment();
        result = assignment.config;
        if (!assignment.contested && is_puppy_config_ok(result, minimal_config)) {
            // done, continue with bootstrap
            break;
        } else {
            if (--max_attempts) {
                log_error(Puppies, assignment.contested ? "Contested dock discovery, will reset all puppies and try again" : "Not enough puppies discovered, will try again");
                continue;
            } else {
                if (assignment.contested) {
                    fatal_error(ErrCode::ERR_SYSTEM_PUPPY_DISCOVER_ERR);
                }
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

PuppyBootstrap::AddressAssignmentResult PuppyBootstrap::run_address_assignment() {
    BootstrapResult result = {};

    // The assign_address broadcast is adopted by every puppy listening at
    // DEFAULT_ADDRESS; the follow-up reset_puppies_range is what evicts the
    // other docks' puppies back to DEFAULT_ADDRESS. That eviction is a
    // one-shot pulse with no acknowledgement (controlled via I2C GPIO expander)
    // so a single missed reset leaves a stray puppy.
    // Answering at this dock's address — discover() then sees a wrong-type
    // or colliding reply. Re-running the whole assign+reset+discover
    // sequence evicts the stray.
    // The caller must reset all puppies and redo the whole assignment.
    constexpr unsigned dock_discover_attempts = 3;

    for (auto dock = DOCKS.begin(); dock != DOCKS.end(); ++dock) {
        auto puppy_type = to_puppy_type(*dock);
        auto address = get_boot_address_for_dock(*dock);

        bootstrap_state_set(0, BootstrapStage::looking_for_puppies);
        log_info(Puppies, "Discovering whats in dock %s %d",
            get_puppy_info(puppy_type).name, std::to_underlying(*dock));

        auto outcome = DiscoverResult::silent;
        for (unsigned attempt = 1; attempt <= dock_discover_attempts; ++attempt) {
            if (attempt > 1) {
                log_warning(Puppies, "Dock %d: discovery attempt %u (re-running assign+reset to evict a stray)",
                    std::to_underlying(*dock), attempt);
            }

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

            outcome = discover(puppy_type, address);
            if (outcome != DiscoverResult::contested) {
                // found, or genuinely silent (empty dock) — re-running
                // assign+reset can only help against a stray, don't burn
                // boot time on an empty dock.
                break;
            }
        }
        if (outcome == DiscoverResult::found) {
            log_info(Puppies, "Dock %d: discovered puppy %s, assigned address: %d",
                std::to_underlying(*dock), get_puppy_info(puppy_type).name, address);
            result.set_dock_occupied(*dock);
        } else if (outcome == DiscoverResult::contested) {
            log_warning(Puppies, "Dock %d: still contested after %u attempts, need to redo the whole assignment",
                std::to_underlying(*dock), dock_discover_attempts);
            return { .config = result, .contested = true };
        } else {
            log_info(Puppies, "Dock %d: no puppy discovered", std::to_underlying(*dock));
        }
    }

    if (!verify_address_assignment(result)) {
        return { .config = result, .contested = true };
    }

    return { .config = result, .contested = false };
}

void PuppyBootstrap::assign_address(BootloaderProtocol::Address current_address, BootloaderProtocol::Address new_address) {
    auto status = flasher.assign_address(current_address, new_address);

    // this is no reply message - so failure is not expected, it would have to fail while writing message
    if (status != BootloaderProtocol::status_t::COMMAND_OK) {
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_ADDR_ASSIGN_ERR);
    }
}

bool PuppyBootstrap::verify_address_assignment(BootstrapResult result) {
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
        return false;
    }
    return true;
}

void PuppyBootstrap::reset_all_puppies() {
    reset_puppies_range(DOCKS.begin(), DOCKS.end());
}

#if HAS_XL_CAN()
/// How long PC13 must sit HIGH for the bridge's Q2 edge network to charge
/// before the MB reset edge can fire reliably. The implicit ~20 ms arming of
/// a plain H->delay->L pulse proved insufficient once the line had floated
/// (bridge parked in its bootloader); 300 ms is the bench-verified margin.
constexpr uint32_t mb_reset_arm_hold_ms = 300;
#endif

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
    #if HAS_XL_CAN()
        if (xl_can.is_enabled()) {
            // XLS: master GPIO modular_bed_reset reaches the bridge NRST,
            // not MB. MB NRST is fired by the bridge's PC13 -> Q2 edge
            // network: it resets on the HIGH->LOW transition of PC13 and
            // arms while PC13 sits HIGH (same procedure as the bridge's own
            // NRST). The 1:1 state mapping keeps reset_puppies_range's
            // H->delay->L producing the arming level and then the reset edge.
            xl_can.set_modular_bed_reset(puppyModbus, state == Pin::State::high);
            if (state == Pin::State::high) {
                // Q2 arming hold, enforced here at the sink so *every*
                // caller arms the network before the falling edge that
                // follows: the first bootstrap pass, the outer retry rounds
                // of PuppyBootstrap::run() (whose generic ~10 ms H phase is
                // below the arming minimum), and verify_address_assignment's
                // unoccupied-dock reset. hal::mmu::init() in the bridge fw
                // drives PC13 LOW at app entry, so no caller may assume a
                // pre-armed line.
                log_info(Puppies, "    MB reset via bridge: arming hold %u ms", static_cast<unsigned>(mb_reset_arm_hold_ms));
                osDelay(mb_reset_arm_hold_ms);
            }
            // Do NOT raise PC13 back HIGH here or anywhere at runtime: Q2
            // disturbs MB NRST on BOTH edges. A rising edge shortly after the
            // falling edge hangs the MB mid-reset; a rising edge after the MB
            // app has started resets it back out of its app. The line parks
            // LOW until the next bootstrap round's arming HIGH write above —
            // pre-discovery, where a spurious MB reset is harmless.
            break;
        }
    #endif
        modular_bed_reset.write(state);
        break;
#endif
#if HAS_INDX_HEAD()
    case Dock::INDX_HEAD:
        // The reset polarity differs between board revisions, Configuration compensates
        Configuration::Instance().write_indx_head_reset(state);
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

PuppyBootstrap::DiscoverResult PuppyBootstrap::discover_once(PuppyType type, BootloaderProtocol::Address address) {
    flasher.set_address(address);

    auto check_status = [](BootloaderProtocol::status_t status) {
        switch (status) {
        case BootloaderProtocol::status_t::COMMAND_OK:
            return DiscoverResult::found;
        case BootloaderProtocol::status_t::NO_RESPONSE:
            return DiscoverResult::silent;
        default:
            // Anything else (INVALID_CRC, INCOMPLETE_RESPONSE, ...) is a garbled
            // reply — typically two puppies answering at once after both adopted
            // the assign_address broadcast.
            log_warning(Puppies, "Puppy discover: garbled reply (status %d)", status);
            return DiscoverResult::contested;
        };
    };

    uint16_t protocol_version;
    if (auto rc = check_status(flasher.get_protocolversion(protocol_version)); rc != DiscoverResult::found) {
        return rc;
    }
    if ((protocol_version & 0xff00) != (BootloaderProtocol::BOOTLOADER_PROTOCOL_VERSION & 0xff00)) // Check major version of bootloader protocol version before anything else
    {
        log_error(Puppies, "Puppy uses incompatible bootloader protocol %04x, buddy wants %04x", protocol_version, BootloaderProtocol::BOOTLOADER_PROTOCOL_VERSION);
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_INCOMPATIBLE_BOOTLODER, protocol_version, BootloaderProtocol::BOOTLOADER_PROTOCOL_VERSION);
    }

    BootloaderProtocol::HwInfo hwinfo;
    if (check_status(flasher.get_hwinfo(hwinfo)) != DiscoverResult::found) {
        log_info(Puppies, "discover_once: get_hwinfo -> no/garbled answer after protocol_version");
        return DiscoverResult::contested;
    }

    // Here it is possible to read raw puppy's OTP before flashing, perhaps to flash a different firmware
    if (protocol_version >= 0x0302) { // OTP read was added in protocol 0x0302

        uint8_t otp[32]; // OTP v5 will fit to 32 Bytes
        if (check_status(flasher.read_otp_cmd(0, otp, 32)) != DiscoverResult::found) {
            log_info(Puppies, "discover_once: read_otp -> no/garbled answer after protocol_version");
            return DiscoverResult::contested;
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
        // The XL-CAN bridge is brought up out-of-band by xl_can_bootstrap(),
        // which reads its own OTP; Dock::XL_CAN never reaches discover_once.
    } // else - older bootloader has revision 0

    if (hwinfo.hw_type != get_puppy_info(type).hw_info_hwtype) {
        log_warning(Puppies, "Puppy at addr 0x%02x has hw_type %u, expected %u (%s) — stray puppy from another dock?",
            std::to_underlying(address), hwinfo.hw_type, get_puppy_info(type).hw_info_hwtype, get_puppy_info(type).name);
        return DiscoverResult::contested;
    }
    if (hwinfo.bl_version < MINIMAL_BOOTLOADER_VERSION) {
        log_error(Puppies, "Puppy's bootloader is too old %04" PRIx32 " buddy wants %04" PRIx32, hwinfo.bl_version, MINIMAL_BOOTLOADER_VERSION);
        fatal_error(ErrCode::ERR_SYSTEM_PUPPY_INCOMPATIBLE_BOOTLODER, hwinfo.bl_version, MINIMAL_BOOTLOADER_VERSION);
    }

    // puppy responded, all is as expected
    return DiscoverResult::found;
}

PuppyBootstrap::DiscoverResult PuppyBootstrap::discover(PuppyType type, BootloaderProtocol::Address address) {
    const auto timeout_ms = 1000;
    const auto start_ms = ticks_ms();
    bool contested = false;
    for (;;) {
        const auto rc = discover_once(type, address);
        if (rc == DiscoverResult::found) {
            return rc;
        }
        contested |= (rc == DiscoverResult::contested);
        if (ticks_diff(ticks_ms(), start_ms) > timeout_ms) {
            return contested ? DiscoverResult::contested : DiscoverResult::silent;
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

    // application firmware already up to date — nothing to flash
    if (fingerprint_match(fw_fingerprints.get_fingerprint(dock), dock)) {
        log_info(Puppies, "Puppy %d-%s fingerprint matched", static_cast<int>(dock), get_puppy_info(puppy_type).name);
        return;
    }
    log_info(Puppies, "Puppy %d-%s fingerprint didn't match, flashing", static_cast<int>(dock), get_puppy_info(puppy_type).name);

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
        debug_assert(offset + size <= static_cast<size_t>(params.fw_size));
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

#if HAS_PUPPY_MODULARBED()
PuppyBootstrap::MbResetCheck PuppyBootstrap::check_mb_reset_controllable() {
    // Reset all puppies, then assign+evict+discover only MODULAR_BED to bring
    // it to its known bootloader address without the rest of the docks.
    reset_all_puppies();

    constexpr BootloaderProtocol::Address mb_address = get_boot_address_for_dock(Dock::MODULAR_BED);

    constexpr auto mb_dock_it = std::ranges::find(DOCKS, Dock::MODULAR_BED);

    // Wait for MB to enter bootloader after reset.
    osDelay(5);

    // Assign address to everyone at DEFAULT_ADDRESS
    assign_address(BootloaderProtocol::Address::DEFAULT_ADDRESS, mb_address);
    osDelay(10);

    // Then evict everything except the puppy at the ModularBed dock
    reset_puppies_range(std::next(mb_dock_it), DOCKS.end());
    static_assert(mb_dock_it == DOCKS.begin(), "Need to reset puppies before mb_dock_it as well");

    // Discover whether MB is actually there.
    const DiscoverResult outcome = discover(MODULARBED, mb_address);
    if (outcome != DiscoverResult::found) {
        log_info(Puppies, "MB reset check: MODULAR_BED not found (outcome=%d)", static_cast<int>(outcome));
        return MbResetCheck::no_mb;
    }

    log_info(Puppies, "MB reset check: MB found at addr 0x%02x, toggling reset line", std::to_underlying(mb_address));

    // Now evict the discovered puppy as well
    reset_puppies_range(mb_dock_it, std::next(mb_dock_it));

    // Probe the previously-assigned address for ~150 ms.  The MB should be
    // gone: NO_RESPONSE throughout → controlled; any answer → uncontrolled.
    constexpr auto probe_window_ms = 150;
    const auto probe_start_ms = ticks_ms();
    flasher.set_address(mb_address);
    uint16_t protocol_version;
    do {
        const auto rc = flasher.get_protocolversion(protocol_version);
        if (rc != BootloaderProtocol::status_t::NO_RESPONSE) {
            log_info(Puppies, "MB reset check: MB still answering at addr 0x%02x after toggle (rc=%d) -> uncontrolled",
                std::to_underlying(mb_address), static_cast<int>(rc));
            return MbResetCheck::uncontrolled;
        }
    } while (ticks_diff(ticks_ms(), probe_start_ms) < probe_window_ms);

    log_info(Puppies, "MB reset check: MB silent after toggle -> controlled");
    return MbResetCheck::controlled;
}
#endif

constexpr bool is_equal_address(Dock dock, modbus::ServerAddress server_address) {
    return std::to_underlying(PuppyBootstrap::get_modbus_address_for_dock(dock)) == std::to_underlying(server_address);
}

static_assert(is_equal_address(Dock::XBUDDY_EXTENSION, modbus::ServerAddress::xbuddy_extension));
static_assert(is_equal_address(Dock::XL_CAN, modbus::ServerAddress::xl_can));

} // namespace buddy::puppies

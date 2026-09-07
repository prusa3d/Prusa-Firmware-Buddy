#pragma once
#include "option/has_dwarf.h"
#include "puppies/BootloaderProtocol.hpp"
#include "unique_file_ptr.hpp"
#include <option/has_puppy_modularbed.h>
#include <puppies/puppy_constants.hpp>
#include <common/utils/algorithm_extensions.hpp>
#include <option/has_indx_head.h>
#include <bit>

namespace buddy::puppies {

/**
 * @brief Start sequence of puppy boards (Detect, Flash, start application)
 *
 */
class PuppyBootstrap {
public:
    /// Minimal puppy bootloader version that works with this bootstrap
    static constexpr uint32_t MINIMAL_BOOTLOADER_VERSION = 294;

    /// @brief Result of puppy bootstrap - indicates which docks are occupied
    struct BootstrapResult {
        uint16_t docks_preset { 0 }; // every bit corresponds with one dock

        // number of detected puppies
        [[nodiscard]] uint8_t discovered_num() const {
            return std::popcount(docks_preset);
        }

        /// sets that this dock is occupied
        void set_dock_occupied(Dock k) {
            docks_preset |= 1 << static_cast<uint16_t>(k);
        }

        /// checks if dock is occupied
        [[nodiscard]] bool is_dock_occupied(Dock k) const {
            return docks_preset & (1 << static_cast<uint16_t>(k));
        }
    };

    /**
     * @brief Constructor.
     * @param BUFFER_SIZE size of buffer in bytes
     * @param buffer buffer for bootloader protocol, needs to be in regular RAM as it is used by DMA
     */
    template <size_t BUFFER_SIZE>
    PuppyBootstrap(std::array<uint8_t, BUFFER_SIZE> &buffer)
        : flasher(buffer.data()) {
        static_assert(BUFFER_SIZE >= BootloaderProtocol::MAX_PACKET_LENGTH, "Buffer needs to be this large");
    }

    /// Start bootstrap procedure
    BootstrapResult run(PuppyBootstrap::BootstrapResult minimal_config, unsigned int max_attempts = 3);

    /// @brief  Returns address on RS485 for bootloader protocol for each dock
    static constexpr BootloaderProtocol::Address get_boot_address_for_dock(Dock dock) {
        return (BootloaderProtocol::Address)((uint8_t)BootloaderProtocol::Address::FIRST_ASSIGNED + (uint8_t)dock);
    }

    /// @brief  Returns address on RS485 for modbus protocol for each dock
    static constexpr BootloaderProtocol::Address get_modbus_address_for_dock(Dock dock) {
        return (BootloaderProtocol::Address)((uint8_t)BootloaderProtocol::Address::MODBUS_OFFSET + (uint8_t)dock);
    }

    /// @brief  This is minimal puppy configuration that is needed for printer to boot up. Minimal puppy config is that we have modular bed & dwarf 1
    static constexpr inline BootstrapResult MINIMAL_PUPPY_CONFIG {
        0
#if HAS_PUPPY_MODULARBED()
            | 1 << static_cast<uint16_t>(Dock::MODULAR_BED)
#endif
#if HAS_DWARF()
            | 1 << static_cast<uint16_t>(Dock::DWARF_1)
#endif
#if HAS_INDX_HEAD()
            | 1 << static_cast<uint16_t>(Dock::INDX_HEAD)
#endif
    };

    [[nodiscard]] static bool any_dock_supports_crash_dump();

#if HAS_PUPPY_MODULARBED()
    /// Result of check_mb_reset_controllable().
    enum class MbResetCheck {
        /// The master GPIO controls the MB reset — genuine XL.
        controlled,
        /// MB kept answering after the toggle — reset line not under master control.
        uncontrolled,
        /// MODULAR_BED was not discovered at all.
        no_mb,
    };

    /**
     * @brief Probe whether the master GPIO actually controls the MB reset line.
     *
     * Assigns the MB to its bootloader address, then toggles the master reset
     * line via reset_puppies_range() over the MODULAR_BED dock (H→10 ms→L).
     * Checks the previously-assigned address for any answer:
     *  - NO_RESPONSE throughout → controlled (MB was disturbed — genuine XL)
     *  - still answering → uncontrolled (line never reached the MB)
     *
     * Polarity-agnostic — works for both level and edge MB reset wiring (see
     * the implementation for details).
     *
     * Known benign imprecision: a stray dwarf that adopted the MB address
     * answers after the toggle and yields a false `uncontrolled` (spurious
     * wiring warning, not a boot abort; rare due to contested-discovery retry).
     * An MB with an incompatible bootloader version fatal_errors inside
     * discover() rather than returning `no_mb`, same as the normal bootstrap.
     *
     * The caller must run PuppyBootstrap::run() afterwards; run() starts with
     * reset_all_puppies() and a fresh address assignment, so this check leaves
     * no persistent state behind.
     */
    MbResetCheck check_mb_reset_controllable();
#endif

private:
    using fingerprint_t = BootloaderProtocol::fingerprint_t;

    /// Helper to index fingerprints by the dock
    class fingerprints_t {
        std::array<fingerprint_t, DOCKS.size()> fingerprints;
        std::array<uint32_t, DOCKS.size()> salts;

    public:
        uint32_t &get_salt(Dock dock) {
            return salts[stdext::index_of(DOCKS, dock)];
        }

        fingerprint_t &get_fingerprint(Dock dock) {
            return fingerprints[stdext::index_of(DOCKS, dock)];
        }
    };

    BootloaderProtocol flasher;
    void reset_all_puppies();
    void reset_puppies_range(DockIterator begin, DockIterator end);

    /// Outcome of probing one address for the expected puppy bootloader.
    /// contested - puppy answered but not the expected puppy, re run assign + reset and retry
    /// silent - the address is not answering at all and retrying is pointless.
    /// found - the expected puppy bootloader answered
    enum class DiscoverResult : uint8_t {
        found,
        silent,
        contested,
    };

    /**
     * @brief Test if puppy bootloader is there and check some info.
     *
     * @param type expecting this type of puppy
     * @param address check puppy with this modbus address
     */
    DiscoverResult discover_once(PuppyType type, BootloaderProtocol::Address address);

    /// Same as discover_once() but polling for a fixed window; reports
    /// `contested` if any poll within the window was contested.
    DiscoverResult discover(PuppyType type, BootloaderProtocol::Address address);

    unique_file_ptr get_firmware(PuppyType type);
    off_t get_firmware_size(PuppyType type);

    /**
     * @brief Check fingerprint and if needed, flash new firmware.
     * @param dock check puppy in this dock
     * @param fw_fingerprints salts already given to puppies and each corresponding fingerprint
     * @param percent_offset start position of the progress trackbar
     * @param percent_span length on the progress trackbar filled with this check
     */
    void flash_firmware(Dock dock, fingerprints_t &fw_fingerprints, int percent_offset, int percent_span);

    /**
     * @brief Tell puppy to check fingerprint and start application.
     * @param type not used now
     * @param address puppy's modbus address
     * @param salt use this salt for fingerprint calculation
     * @param fingerprint puppy will check this fingerprint before starting the app
     */
    void start_app(PuppyType type, BootloaderProtocol::Address address, uint32_t salt, const fingerprint_t &fingerprint);

    /**
     * @brief Wait for puppy to finish fingerprint calculation.
     * Puppy's address needs to be set by flasher.set_address(address) before calling this.
     * @param calculation_start time of ticks_ms() when the calculation was started.
     */
    void wait_for_fingerprint(uint32_t calculation_start);

    /**
     * @brief Calculate fingerprint of a puppy's firmware.
     * @param file this firmware
     * @param fingerprint output fingerprint
     * @param salt add this salt before the app firmware
     */
    void calculate_fingerprint(unique_file_ptr &file, fingerprint_t &fingerprint, uint32_t salt);

    /**
     * @brief Check chunk of fingerprint from puppy.
     * @param fingerprint fingerprint to compare
     * @param dock which dock is being checked
     * @return true if fingerprint matches
     */
    bool fingerprint_match(const fingerprint_t &fingerprint, Dock dock);

    /// Result of one address-assignment sweep over all docks.
    struct AddressAssignmentResult {
        BootstrapResult config;
        /// A dock stayed contested even after the per-dock retries — a stray
        /// the forward-only eviction cannot reach (e.g. a rebooted puppy from
        /// an already assigned dock). Only reset_all_puppies() and a fresh
        /// assignment evicts it, so the sweep was aborted early.
        bool contested = false;
    };

    AddressAssignmentResult run_address_assignment();
    void assign_address(BootloaderProtocol::Address current_address, BootloaderProtocol::Address new_address);
    bool is_puppy_config_ok(BootstrapResult result, BootstrapResult minimal_config);
    bool verify_address_assignment(BootstrapResult result);

    /**
     * @brief Tell puppies to start fingerprint calculation.
     * @param address puppy's address
     * @param salt add this salt into sha before the app
     */
    void start_fingerprint_computation(BootloaderProtocol::Address address, uint32_t salt);

    /**
     * @brief Downloads crash dump from a puppy if present.
     *
     * @return true if successfully downloaded a crash dump.
     */
    bool attempt_crash_dump_download(Dock dock, BootloaderProtocol::Address address);
};
} // namespace buddy::puppies

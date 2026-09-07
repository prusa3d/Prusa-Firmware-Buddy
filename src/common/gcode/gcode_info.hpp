/**
 * @file gcode_info.hpp
 * @author Michal Rudolf
 * @brief Structure that extracts and holds gcode comment info
 * @date 2021-03-25
 */
#pragma once

#include <utils/color.hpp>
#include <option/has_indx.h>
#include <option/has_mmu2.h>
#include <option/has_gcode_compatibility.h>
#include <common/filament.hpp>
#include <common/hw_check.hpp>
#include <config_store/store_instance.hpp>
#include "gcode_buffer.hpp"
#include <gcode_reader_interface.hpp>
#include <array>
#include <feature/print_area.h>
#include <utils/compact_optional.hpp>
#include <utils/tristate.hpp>
#include <feature/compatibility_checks/gcode_compatibility.hpp>
#include <utils/storage/strong_index_array.hpp>
#include <tool_index.hpp>
#include <bsod/bsod.h>
#include <buddy/filename_defs.hpp>

// these strings are meant NOT to be translated
namespace gcode_info {
inline constexpr const char *time = "estimated printing time (normal mode)";
inline constexpr const char *filament_type = "filament_type";
inline constexpr const char *extruder_colour = "extruder_colour";
inline constexpr const char *filament_mm = "filament used [mm]";
inline constexpr const char *filament_g = "filament used [g]";
#if EXTRUDERS > 1
inline constexpr const char *filament_wipe_tower_g = "total filament used for wipe tower [g]";
#endif
inline constexpr const char *printer = "printer_model";
inline constexpr const char *m862 = "M862";
inline constexpr const char *m115 = "M115";
inline constexpr const char *m555 = "M555";
inline constexpr const char *m486 = "M486";
inline constexpr const char *m140_set_bed_temp = "M140";
inline constexpr const char *m190_wait_bed_temp = "M190";
inline constexpr const char *m104_set_hotend_temp = "M104";
inline constexpr const char *m109_wait_hotend_temp = "M109";
#if EXTRUDERS > 1
inline constexpr const char *total_toolchanges = "total toolchanges";
#endif
}; // namespace gcode_info

/// When initializing the heavy work is done in start_load, load and end_load functions,
/// if you need to offload it to another thread, you can check the progress using
/// start_load_result, can_be_printed and is_loaded.
///
/// Currently it can be done by sending PREFETCH_SIGNAL_GCODE_INFO_INIT signal to media_prefetch thread.
/// Check code in PrintPreview::Loop for an example.
class GCodeInfo {
public:
    // GCODE_LEVEL from PrusaGcodeSuite.hpp, but we don't want to include the header.
    static constexpr uint32_t gcode_level = 2;

    // search this many g-code at the beginning of the file for the various g-codes (M862.x nozzle size, bed heating, nozzle heating)
    static constexpr size_t search_first_x_gcodes = 200;

    using time_buff = std::array<char, 16>;

    struct ExtruderInfo {
        CompactOptional<float, NAN> filament_used_g; /**< stores how much filament will be used for this print (weight) */
        CompactOptional<float, NAN> filament_used_mm; /**< stores how much filament will be used for this print (distance) */
        CompactOptional<float, NAN> nozzle_diameter; /**< stores diameter of nozzle*/

        CompactOptional<Color, COLOR_NONE> extruder_colour; /**< stores colour of extruder*/
        FilamentTypeParameters::Name filament_name; /**< stores string representation of filament type */

        // Tristate::other represents std::nullopt/data not present in the gcode
        Tristate requires_hardened_nozzle = Tristate::other;
        Tristate requires_high_flow_nozzle = Tristate::other;

        inline bool used() const {
            /// At least this much filament [g] to be considered used (just purge is about 0.06 g on both XL and MK3)
            static constexpr float FILAMENT_USED_MIN_G = 0.0003f;
            return filament_used_g.value_or(0) > FILAMENT_USED_MIN_G;
        }

        /// @brief Value for this extruder was given in G-code
        inline bool given() const {
            return filament_used_g.has_value();
        }
    };

    // we keep old array size instead of GcodeToolIndex::count because of weak indexing (see definition of GcodeToolIndex::count)
    using GCodePerExtruderInfo = StrongIndexArray<ExtruderInfo, EXTRUDERS, GcodeToolIndex, GcodeToolIndex::to_raw_static, strong_index_array::AllowWeakIndexing::yes>;

private:
    // atomic flags to signal to other thread, the progress of gcode loading
    std::atomic<bool> is_loaded_ = false; ///< did the load() function finish?
    std::atomic<bool> is_printable_ = false; ///< is it valid for print?, checked by gcode reader "valid_for_print" function

    std::atomic<const char *> error_str_ = nullptr; ///< If there is an error, this variable can be used to report the error string

#if HAS_E2EE_SUPPORT()
    mutable freertos::Mutex identity_mutex;
    std::optional<e2ee::IdentityInfo> identity_info;
#endif

    GCodePerExtruderInfo per_extruder_info; ///< Info about G-code for each extruder

    // Struct with all the data, so that we can safely reset everything by just info_ = {}
    struct GenericInfo {
        time_buff printing_time { '?', '\0' }; ///< Stores string representation of printing time left
#if EXTRUDERS > 1
        std::optional<float> filament_wipe_tower_g = { std::nullopt }; ///< Grams of filament used for wipe tower
#endif
        bool sliced_with_input_shaper_ = false; ///< True if gcode was sliced with input shaper
#if HAS_INDX()
        bool sliced_with_indx_lock_ = false; ///< True if gcode was sliced with INDX lock support
#endif
        bool has_preview_thumbnail_ = false; ///< True if gcode has preview thumbnail
        bool has_progress_thumbnail_ = false; ///< True if gcode has progress thumbnail
        bool filament_described = false; ///< Filament info was found in gcode's comments
        bool new_firmware_available = false;

        /// Failure of some checks can be determined directly during GCodeInfo scan
        buddy::gcode_compatibility::ChecksTraits<buddy::gcode_compatibility::GeneralCheck>::Bitset failed_gcode_checks;

        InplaceString<sizeof("99.99.99-alpha99+999999")> latest_fw_version;

        std::optional<uint16_t> bed_preheat_temp { std::nullopt }; ///< Holds bed preheat temperature
        std::optional<PrintArea::rect_t> bed_preheat_area { std::nullopt }; ///< Holds bed preheat area
        std::optional<uint16_t> hotend_preheat_temp { std::nullopt }; ///< Holds hotend preheat temperatureF
#if EXTRUDERS > 1
        std::optional<uint16_t> total_toolchanges { std::nullopt }; ///< Total number of toolchanges over the whole print (INDX nozzle-cleaner wastebin fill estimation)
#endif
    };
    GenericInfo info_;

public:
    /**
     * Reset loaded gcode info to empty value
     */
    void reset_info();

    const time_buff &get_printing_time() const { return info_.printing_time; } ///< Get string representation of printing time left
    bool is_loaded() const { return is_loaded_; }

    inline bool has_error() const { return error_str_; } ///< Returns whether there is an (unrecoverable) error detected. The error message can then be obtained using error_str
    inline const char *error_str() const { return error_str_; } ///< If there is any reportable error, returns it. Otherwise returns nullptr.

    inline void set_error(const char *error) {
        debug_assert(error);
        error_str_ = error;
    }

#if HAS_E2EE_SUPPORT()
    bool has_identity_info() const {
        std::unique_lock lock(identity_mutex);
        return identity_info.has_value();
    }
    e2ee::IdentityInfo get_identity_info() const {
        std::unique_lock lock(identity_mutex);
        return identity_info.value();
    }

    void set_identity_info(e2ee::IdentityInfo info) {
        std::unique_lock lock(identity_mutex);
        identity_info = info;
    }
#endif

    const GenericInfo &info() const {
        return info_;
    }

    bool has_preview_thumbnail() const { return info_.has_preview_thumbnail_; } ///< Check if file has preview thumbnail
    bool has_progress_thumbnail() const { return info_.has_progress_thumbnail_; } ///< Check if file has progress thumbnail
    bool has_filament_described() const { return info_.filament_described; } ///< Check if file has filament described
    const GCodePerExtruderInfo &get_per_extruder_info() const { return per_extruder_info; } ///< Get info about G-code for each extruder
    const std::optional<uint16_t> &get_bed_preheat_temp() const { return info_.bed_preheat_temp; } ///< Get info about bed preheat temperature
    const std::optional<PrintArea::rect_t> &get_bed_preheat_area() const { return info_.bed_preheat_area; } ///< Get info about G-preheat area
    inline const std::optional<uint16_t> &get_hotend_preheat_temp() const { return info_.hotend_preheat_temp; }
#if EXTRUDERS > 1
    std::optional<uint16_t> get_total_toolchanges() const { return info_.total_toolchanges; } ///< Total toolchanges parsed from gcode metadata (std::nullopt if absent)
    std::optional<float> get_filament_wipe_tower_g() const { return info_.filament_wipe_tower_g; } ///< filament used for wipe tower
#endif

    /**
     * @brief Check if gcode is sliced with singletool profile.
     * @return true if singletool
     *
     * Sliced for multitool:  ; filament used [g] = 0.34, 0.00, 0.00, 0.00, 0.00
     * Sliced for singletool: ; filament used [g] = 0.34
     */
    bool is_singletool_gcode() const;

    [[deprecated("Use ToolIndex overload")]]
    const ExtruderInfo &get_extruder_info(uint8_t extruder) const {
        debug_assert(extruder < std::size(per_extruder_info));
        return per_extruder_info[extruder];
    }

    /**
     * @brief Get info about G-code for given extruder
     * @param[in] extruder - extruder number [indexed from 0]
     * @return ExtruderInfo for given extruder
     */
    const ExtruderInfo &get_extruder_info(GcodeToolIndex tool) const {
        return per_extruder_info[tool];
    }

    /// Calls \param callback for each extruder that is used for the print
    void for_each_used_extruder(const stdext::inplace_function<void(GcodeToolIndex, VirtualToolIndex, const ExtruderInfo &info)> &callback);

    /** Get static instance of the singleton
     */
    static GCodeInfo &getInstance();

    /** Get number of used extruders
     */
    int UsedExtrudersCount() const;

    /**
     * @brief Get number of extruders given in G-code.
     * @return how many extruders are written in G-code
     */
    int GivenExtrudersCount() const;

    /// Returns LFN of the file (without path) - display purposes
    const char *GetGcodeFilename();

    /// Returns SFN filepath - referencing purposes, do not display
    const char *GetGcodeFilepath();

    /// Set the filename (LFN) and filepath (SFN) of the gcode we're going to store the info for in GCodeInfo
    /// The strings get copied into member variables, so no lifetime requirements.
    void set_gcode_file(const char *filepath_sfn, const char *filename_lfn);

    /**
     * @brief Checks if the file still exists and can be potentially printed.
     * Softer version of check_valid_for_print, performs more basic checks.
     * Does not set \c is_printable to true on success, you gotta \c check_valid_for_print for that.
     * @return false on failure, sets \c is_printable to false and updates \c error_str
     */
    bool check_still_valid();

    /**
     * @brief Check if file is ready for print. Updates \c is_printable and \c error_str.
     * @param file_reader gcode file reader, it cannot be accessed by other threads at the same time
     */
    bool check_valid_for_print(IGcodeReader &reader);

    /**
     * @brief Check the printable flag.
     *
     * To be used concurently to `check_valid_for_print`,
     * which does the real checking
     */
    bool can_be_printed() { return is_printable_; }

    /**
     * @brief Sets up gcode file and sets up info member variables for print preview.
     * @note start_load and end_load shall be called before&after
     * @param reader gcode file reader, it cannot be accessed by other threads at the same time
     */
    void load(IGcodeReader &reader);

private:
    /** Iterate over items separated by some delimeter character */
    std::optional<std::string_view> iterate_items(std::span<char> &buffer, char separator);

    /// stores current gcode file path
    /// SFN filepath (used for referencing the file)
    std::array<char, filename_defs::path_buffer_size> gcode_file_path = { '\0' };

    /// stores current gcode file name
    /// LFN filename (used for display)
    std::array<char, filename_defs::filename_buffer_size> gcode_file_name = { '\0' };

#ifdef UNITTESTS
public:
#endif
    GCodeInfo();

private:
    GCodeInfo(const GCodeInfo &) = delete;

    void parse_m555(GcodeBuffer::String cmd);
    void parse_m862(GcodeBuffer::String cmd);
    void parse_gcode(GcodeBuffer::String cmd, uint32_t &gcode_counter);
    void parse_comment(GcodeBuffer::String cmd, bool plaintext_gcodes);
    bool is_up_to_date(const char *new_version);
};

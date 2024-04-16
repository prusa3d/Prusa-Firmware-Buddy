/**
 * @file gcode_info.hpp
 * @author Michal Rudolf
 * @brief Structure that extracts and holds gcode comment info
 * @date 2021-03-25
 */
#pragma once

#include "option/has_gui.h"
#if HAS_GUI()
    #include "guitypes.hpp"
#endif
#include "i18n.h"
#include "marlin_stubs/PrusaGcodeSuite.hpp"
#include <option/has_toolchanger.h>
#include <config_store/store_instance.hpp>
#include "gcode_buffer.hpp"
#include "gcode_reader_any.hpp"
#include <array>
#include <feature/print_area.h>

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
inline constexpr const char *m140_set_bed_temp = "M140";
inline constexpr const char *m190_wait_bed_temp = "M190";
inline constexpr const char *m104_set_hotend_temp = "M104";
inline constexpr const char *m109_wait_hotend_temp = "M109";
}; // namespace gcode_info

/// When initializing the heavy work is done in start_load, load and end_load functions,
/// if you need to offload it to another thread, you can check the progress using
/// start_load_result, can_be_printed and is_loaded.
///
/// Currently it can be done by sending PREFETCH_SIGNAL_GCODE_INFO_INIT signal to media_prefetch thread.
/// Check code in PrintPreview::Loop for an example.
class GCodeInfo {
public:
    enum class StartLoadResult {
        None,
        Started,
        Failed
    };
    static constexpr uint32_t gcode_level = GCODE_LEVEL;

#if PRINTER_IS_PRUSA_MK4
    static constexpr std::array<const char *, 3> printer_compatibility_list = { PRINTER_MODEL, "MK3.9" }; ///< Basic compatibility for M862.3 G-code
#else
    static constexpr std::array<const char *, 1> printer_compatibility_list = { PRINTER_MODEL }; ///< Basic compatibility for M862.3 G-code
#endif

    static constexpr std::array<const char *, 1> supported_features = { "Input shaper" };

    // search this many g-code at the beginning of the file for the various g-codes (M862.x nozzle size, bed heating, nozzle heating)
    static constexpr size_t search_first_x_gcodes = 200;

    using time_buff = std::array<char, 16>;
    using filament_buff = std::array<char, 8>;

    struct ExtruderInfo {
        struct Colour {
            uint8_t red;
            uint8_t green;
            uint8_t blue;
        };

        std::optional<filament_buff> filament_name; /**< stores string representation of filament type */
        std::optional<float> filament_used_g; /**< stores how much filament will be used for this print (weight) */
        std::optional<float> filament_used_mm; /**< stores how much filament will be used for this print (distance) */
        std::optional<float> nozzle_diameter; /**< stores diameter of nozzle*/
        std::optional<Colour> extruder_colour; /**< stores colour of extruder*/

        inline bool used() const {
            /// At least this much filament [g] to be considered used (just purge is about 0.06 g on both XL and MK3)
            static constexpr float FILAMENT_USED_MIN_G = 0.03;
            return filament_used_g.has_value() && filament_used_g.value() > FILAMENT_USED_MIN_G;
        }

        /// @brief Value for this extruder was given in G-code
        inline bool given() const {
            return filament_used_g.has_value();
        }
    };

    struct ValidPrinterSettings {
        class Feature {
            bool wrong { false };
            HWCheckSeverity severity;

        public:
            Feature(HWCheckSeverity severity_)
                : severity(severity_) {}

            HWCheckSeverity get_severity() const { return severity; }
            bool is_valid() const { return !wrong || severity == HWCheckSeverity::Ignore; }
            bool is_fatal() const { return wrong && severity == HWCheckSeverity::Abort; }
            void fail() { wrong = true; }
        };

        Feature wrong_tools { HWCheckSeverity::Abort }; // Tools that are used, are not connected (toolchanger only). Can be handled by tools mapping screen
        Feature wrong_nozzle_diameter { config_store().hw_check_nozzle.get() }; // M862.1 disagree (or M862.10 - M862.15 for multihotend gcode). Can be handled by tools mapping screen
        Feature wrong_printer_model { config_store().hw_check_model.get() }; // M862.2 or M862.3 or printer_model (from comments) disagree
        Feature wrong_gcode_level { config_store().hw_check_gcode.get() }; // M862.5 disagree
        Feature wrong_firmware { config_store().hw_check_firmware.get() }; // M862.4 Px.yy.z disagrees
        Feature mk3_compatibility_mode { config_store().hw_check_compatibility.get() };
        Feature outdated_firmware { config_store().hw_check_firmware.get() }; // M115 Ux.yy.z disagrees (TODO: Separate EEVAR?)
        bool unsupported_features { false };
        char unsupported_features_text[37] { "" };
        void add_unsupported_feature(const char *feature, size_t length);

        /**
         * @brief Firmware version read from G-code M115 Ux.yy.z.
         */
        struct GcodeFwVersion {
            unsigned major = 0;
            unsigned minor = 0;
            unsigned patch = 0;
            unsigned build_number = 0;
        };

        char latest_fw_version[sizeof("99.99.99-alpha99+999999")];

        /**
         * @brief Are all printer setting valid?
         * @param is_tools_mapping_possible Whether to check all settings or only those not handled by tools mapping screen
         * @return true if all Feature wrong_xxx.is_valid()
         */
        bool is_valid(bool is_tools_mapping_possible = false) const;

        /**
         * @brief Is any printer setting fatal?
         * @param is_tools_mapping_possible Whether to check all settings or only those not handled by tools mapping screen
         * @return true if any Feature wrong_xxx.is_fatal()
         */
        bool is_fatal(bool is_tools_mapping_possible = false) const;
    };

    using GCodePerExtruderInfo = std::array<ExtruderInfo, EXTRUDERS>;

private:
    uint32_t printer_model_code; ///< model code (see printer_model2code())

    // atomic flags to signal to other thread, the progress of gcode loading
    std::atomic<bool> is_loaded_ = false; ///< did the load() function finish?
    std::atomic<StartLoadResult> start_load_result_ = {}; ///< None if nt started yet, Failed - opening gcode failed, Started - success
    std::atomic<bool> is_printable_ = false; ///< is it valid for print?, checked by gcode reader "valid_for_print" function

    std::atomic<const char *> error_str_ = nullptr; ///< If there is an error, this variable can be used to report the error string

    time_buff printing_time; ///< Stores string representation of printing time left
#if EXTRUDERS > 1
    std::optional<float> filament_wipe_tower_g = { std::nullopt }; ///< Grams of filament used for wipe tower
#endif
    bool has_preview_thumbnail_; ///< True if gcode has preview thumbnail
    bool has_progress_thumbnail_; ///< True if gcode has progress thumbnail
    bool filament_described; ///< Filament info was found in gcode's comments
    ValidPrinterSettings valid_printer_settings; ///< Info about matching hardware
    GCodePerExtruderInfo per_extruder_info; ///< Info about G-code for each extruder
    std::optional<uint16_t> bed_preheat_temp { std::nullopt }; ///< Holds bed preheat temperature
    std::optional<PrintArea::rect_t> bed_preheat_area { std::nullopt }; ///< Holds bed preheat area
    std::optional<uint16_t> hotend_preheat_temp { std::nullopt }; ///< Holds hotend preheat temperature

public:
    const time_buff &get_printing_time() const { return printing_time; } ///< Get string representation of printing time left
    bool is_loaded() const { return is_loaded_; } ///< Check if file has preview thumbnail

    inline bool has_error() const { return error_str_; } ///< Returns whether there is an (unrecoverable) error detected. The error message can then be obtained using error_str
    inline const char *error_str() const { return error_str_; } ///< If there is any reportable error, returns it. Otherwise returns nullptr.

    bool has_preview_thumbnail() const { return has_preview_thumbnail_; } ///< Check if file has preview thumbnail
    bool has_progress_thumbnail() const { return has_progress_thumbnail_; } ///< Check if file has progress thumbnail
    bool has_filament_described() const { return filament_described; } ///< Check if file has filament described
    const ValidPrinterSettings &get_valid_printer_settings() const { return valid_printer_settings; } ///< Get info about matching hardware
    const GCodePerExtruderInfo &get_per_extruder_info() const { return per_extruder_info; } ///< Get info about G-code for each extruder
    const std::optional<uint16_t> &get_bed_preheat_temp() const { return bed_preheat_temp; } ///< Get info about bed preheat temperature
    const std::optional<PrintArea::rect_t> &get_bed_preheat_area() const { return bed_preheat_area; } ///< Get info about G-preheat area
    inline const std::optional<uint16_t> &get_hotend_preheat_temp() const { return hotend_preheat_temp; }
#if EXTRUDERS > 1
    std::optional<float> get_filament_wipe_tower_g() const { return filament_wipe_tower_g; } ///< filament used for wipe tower
#endif

    /**
     * @brief Check if gcode is sliced with singletool profile.
     * @return true if singletool
     *
     * Sliced for multitool:  ; filament used [g] = 0.34, 0.00, 0.00, 0.00, 0.00
     * Sliced for singletool: ; filament used [g] = 0.34
     */
    bool is_singletool_gcode() const;

    /**
     * @brief Get info about G-code for given extruder
     * @param[in] extruder - extruder number [indexed from 0]
     * @return ExtruderInfo for given extruder
     */
    const ExtruderInfo &get_extruder_info(uint8_t extruder) const {
        assert(extruder < std::size(per_extruder_info));
        return per_extruder_info[extruder];
    }

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

    /** Set variables for gcode filename and filepath
     *  @param[in] fname - aquired filename
     *  @param[in] fpath - aquired filepath
     */
    void Init(const char *fname, const char *fpath);

    /** Get static variable gcode filename
     *  @param[in] fname - aquired filename
     */
    const char *GetGcodeFilename();

    /** Get static variable gcode filepath
     *  @param[in] fpath - aquired filename
     */
    const char *GetGcodeFilepath();

    /**
     * @brief Start loading of gcode (open file).
     * @param file_reader gcode file reader, it cannot be accessed by other threads at the same time
     */
    bool start_load(AnyGcodeFormatReader &file_reader);

    /**
     * @brief End loading of gcode (close file).
     * @param file_reader gcode file reader, it cannot be accessed by other threads at the same time
     */
    void end_load(AnyGcodeFormatReader &file_reader);

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
    bool check_valid_for_print(AnyGcodeFormatReader &file_reader);

    /**
     * @brief Checks validity of the file (possibly CRC and such).
     * Returns if the file is valid. Updates error_str if the file is not valid.
     * @param file_reader gcode file reader, it cannot be accessed by other threads at the same time
     */
    bool verify_file(AnyGcodeFormatReader &file_reader);

    /**
     * @brief Check the printable flag.
     *
     * To be used concurently to `check_valid_for_print`,
     * which does the real checking
     */
    bool can_be_printed() { return is_printable_; }

    /**
     * @brief Check the result of starting the load.
     *
     * To be used concurently to `start_load`,
     * which does the starting.
     */
    StartLoadResult start_load_result() { return start_load_result_; }

    /**
     * @brief Sets up gcode file and sets up info member variables for print preview.
     * @note start_load and end_load shall be called before&after
     * @param file_reader gcode file reader, it cannot be accessed by other threads at the same time
     */
    void load(AnyGcodeFormatReader &file_reader);

    /** Evaluates tool compatibility*/
    void EvaluateToolsValid();

    /** Getter for printer_model_code
     */
    uint32_t getPrinterModelCode() const;

private:
    /**
     * @brief Parse G-code file for comments and info codes.
     * This cannot be run from Marlin thread, because it takes too long for watchdog.
     * @param[in] reader - gcode file reader reference
     */
    void PreviewInit(IGcodeReader &reader);

    /** Iterate over items separated by some delimeter character */
    std::optional<std::span<char>> iterate_items(std::span<char> &buffer, char separator);

    const char *gcode_file_path; /**< stores current gcode file path */
    const char *gcode_file_name; /**< stores current gcode file name */

#if HAS_GUI()
    /** Set static variable for gcode filename
     *  @param[in] file - gcode file reference
     *  @param[in] size - thumbnail wanted size
     *  @return True - if has thumbnail with those size parameters
     */
    bool hasThumbnail(IGcodeReader &reader, size_ui16_t size);
#endif
    GCodeInfo();
    GCodeInfo(const GCodeInfo &) = delete;

    void parse_m555(GcodeBuffer::String cmd);
    void parse_m862(GcodeBuffer::String cmd);
    void parse_gcode(GcodeBuffer::String cmd, uint32_t &gcode_counter);
    void parse_comment(GcodeBuffer::String cmd);
    bool is_up_to_date(const char *new_version);

    /**
     * @brief Test printer model with a list of compatible models.
     * @tparam SIZE size of the compatibility_list
     * @param printer printer model to test
     * @param compatibility_list list of compatible models
     * @return true if printer is compatible with any of the models in compatibility_list
     */
    template <std::size_t SIZE>
    bool is_printer_compatible(const GcodeBuffer::String &printer, const std::array<const char *, SIZE> &compatibility_list) {
        return std::any_of(begin(compatibility_list),
            end(compatibility_list),
            [&](const auto &v) { return printer == v; });
    }

    /**
     * Reset loaded gcode info to empty value
     */
    void reset_info();
};

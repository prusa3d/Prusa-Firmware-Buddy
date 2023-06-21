/**
 * @file gcode_info.hpp
 * @author Michal Rudolf
 * @brief Structure that extracts and holds gcode comment info
 * @date 2021-03-25
 */
#pragma once

#include "config_features.h"
#include "guitypes.hpp"
#include "i18n.h"
#include "marlin_stubs/PrusaGcodeSuite.hpp"
#include <option/has_toolchanger.h>
#include <configuration_store.hpp>

#include <array>

// these strings are meant NOT to be translated
namespace gcode_info {
inline constexpr const char *time = "estimated printing time (normal mode)";
inline constexpr const char *filament_type = "filament_type";
inline constexpr const char *filament_mm = "filament used [mm]";
inline constexpr const char *filament_g = "filament used [g]";
inline constexpr const char *printer = "printer_model";
inline constexpr const char *m862 = "M862.";
inline constexpr const char *m115 = "M115";
};

constexpr uint32_t printer_model2code(const char *model) {
    struct {
        const char *model;
        uint32_t code;
    } models[] = {
        { "MK1", 100 },
        { "MK2", 200 },
        { "MK2MM", 201 },
        { "MK2S", 202 },
        { "MK2SMM", 203 },
        { "MK2.5", 250 },
        { "MK2.5MMU2", 20250 },
        { "MK2.5S", 252 },
        { "MK2.5SMMU2S", 20252 },
        { "MK3", 300 },
        { "MK3MMU2", 20300 },
        { "MK3S", 302 },
        { "MK3SMMU2S", 20302 },
        { "MK3.5", 130 },
        { "MK3.5MMU3", 30130 },
        { "MINI", 120 },
        { "MK4", 130 },
        { "MK4MMU3", 30130 },
        { "iX", 160 },
        { "XL", 170 },
    };

    for (auto &m : models) {
        if (std::string_view(m.model) == model) {
            return m.code;
        }
    }
    assert(false);
}

class GCodeInfo {
public:
    static constexpr const uint32_t printer_model_code = printer_model2code(PRINTER_MODEL);
    static constexpr uint32_t gcode_level = GCODE_LEVEL;
    static constexpr std::array<const char *, 1> printer_compatibility_list = { PRINTER_MODEL }; ///< Basic compatibility for M862.3 G-code

    /// Extended compatibility list for "; printer_model = ???" G-code comment
#if PRINTER_IS_PRUSA_XL
    // XL is compatible with multitool models XL2 .. XL5
    static constexpr std::array<const char *, 5> printer_extended_compatibility_list = { PRINTER_MODEL, PRINTER_MODEL "2", PRINTER_MODEL "3", PRINTER_MODEL "4", PRINTER_MODEL "5" };
#elif PRINTER_IS_PRUSA_MK4
    static constexpr std::array<const char *, 1> printer_extended_compatibility_list = { PRINTER_MODEL }; // TODO: { PRINTER_MODEL, PRINTER_MODEL "IS" };
#else  /*PRINTER_IS*/
    static constexpr auto printer_extended_compatibility_list = printer_compatibility_list;
#endif /*PRINTER_IS*/

    using time_buff = std::array<char, 16>;
    using filament_buff = std::array<char, 8>;

    enum class GI_INIT_t {
        THUMBNAIL, ///< Init only thumbnail
        FULL,      ///< Scan file for info G-code and comments (takes a long time)
    };

    struct ExtruderInfo {
        std::optional<filament_buff> filament_name; /**< stores string representation of filament type */
        std::optional<float> filament_used_g;       /**< stores how much filament will be used for this print (weight) */
        std::optional<float> filament_used_mm;      /**< stores how much filament will be used for this print (distance) */
        std::optional<float> nozzle_diameter;       /**< stores diameter of nozzle*/

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

        Feature wrong_tools { HWCheckSeverity::Abort };                         // Tools that are used, are not connected (toolchanger only)
        Feature wrong_nozzle_diameter { config_store().hw_check_nozzle.get() }; // M862.1 disagree (or M862.10 - M862.15 for multihotend gcode)
        Feature wrong_printer_model { config_store().hw_check_model.get() };    // M862.2 or M862.3 or printer_model (from comments) disagree
        Feature wrong_gcode_level { config_store().hw_check_gcode.get() };      // M862.5 disagree
        Feature wrong_firmware { config_store().hw_check_firmware.get() };      // M862.4 Px.yy.z disagrees
        Feature mk3_compatibility_mode { config_store().hw_check_compatibility.get() };
        Feature outdated_firmware { config_store().hw_check_firmware.get() };   // M115 Ux.yy.z disagrees (TODO: Separate EEVAR?)
        bool unsupported_features { false };
        char unsupported_features_text[37] { "" };
        void add_unsupported_feature(const char *feature, size_t length);

        /**
         * @brief Firmware version read from G-code M115 Ux.yy.z.
         */
        struct GcodeFwVersion {
            int major = 0;
            int minor = 0;
            int patch = 0;
        };

        GcodeFwVersion gcode_fw_version;
        GcodeFwVersion latest_fw_version;

        /**
         * @brief Are all printer setting valid?
         * @return true if all Feature wrong_xxx.is_valid()
         */
        bool is_valid() const;

        /**
         * @brief Is any printer setting fatal?
         * @return true if any Feature wrong_xxx.is_fatal()
         */
        bool is_fatal() const;
    };

    using GCodePerExtruderInfo = std::array<ExtruderInfo, EXTRUDERS>;

private:
    osMutexDef(file_mutex);
    osMutexId file_mutex_id;                     ///< Mutex to protect file access
    FILE *file;                                  ///< G-code file, nullptr == not opened

    time_buff printing_time;                     ///< Stores string representation of printing time left
    bool preview_thumbnail;                      ///< True if gcode has preview thumbnail
    bool progress_thumbnail;                     ///< True if gcode has progress thumbnail
    bool filament_described;                     ///< Filament info was found in gcode's comments
    ValidPrinterSettings valid_printer_settings; ///< Info about matching hardware
    GCodePerExtruderInfo per_extruder_info;      ///< Info about G-code for each extruder

public:
    /**
     * @brief Get file that locks itself.
     */
    class FileLender {
        osMutexId &file_mutex_id; ///< Mutex to protect file access

    public:
        FILE *&file; ///< gcode file, nullptr == not opened

        FileLender(FILE *&file_, osMutexId &file_mutex_id_)
            : file_mutex_id(file_mutex_id_)
            , file(file_) {
            [[maybe_unused]] auto res = osMutexWait(file_mutex_id, osWaitForever);
            assert(res == osOK);
        }

        ~FileLender() {
            [[maybe_unused]] auto res = osMutexRelease(file_mutex_id);
            assert(res == osOK);
        }

        operator FILE *() const { return file; }
    };

    [[nodiscard]] FileLender get_file() { return FileLender(file, file_mutex_id); }                   ///< Get file that locks itself
    bool is_file_open() const { return file != nullptr; }                                             ///< Check if file is open
    const time_buff &get_printing_time() const { return printing_time; }                              ///< Get string representation of printing time left
    bool has_preview_thumbnail() const { return preview_thumbnail; }                                  ///< Check if file has preview thumbnail
    bool has_progress_thumbnail() const { return progress_thumbnail; }                                ///< Check if file has progress thumbnail
    bool has_filament_described() const { return filament_described; }                                ///< Check if file has filament described
    const ValidPrinterSettings &get_valid_printer_settings() const { return valid_printer_settings; } ///< Get info about matching hardware
    const GCodePerExtruderInfo &get_per_extruder_info() const { return per_extruder_info; }           ///< Get info about G-code for each extruder

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

    /** Sets up gcode file and sets up info member variables for print preview
     */
    void initFile(GI_INIT_t init);

    /** Closes gcode file
     */
    void deinitFile();

private:
    /**
     * @brief Parse G-code file for comments and info codes.
     * This cannot be run from Marlin thread, because it takes too long for watchdog.
     */
    void PreviewInit();

    /** Evaluates tool compatibility*/
    void EvaluateToolsValid();

    class Buffer {
    public:
        // first 80 characters of line is sufficient for all GCodeInfo purposes
        // read_line() discards the rest
        using Container = std::array<char, 81>;

        FILE &file;
        Container buffer;

    public:
        class String {
        public:
            Container::iterator begin;
            Container::iterator end;

            void skip_ws();

            /// @brief Skip nonwhitespace characters
            void skip_nws();
            void trim();

            void skip(size_t amount);
            template <class Cond>
            void skip(Cond cond) {
                while (begin != end && cond(*begin)) {
                    ++begin;
                }
            }

            char front() { return *begin; }
            char pop_front() { return begin == end ? '\0' : *begin++; }

            bool is_empty() { return begin == end; }
            uint32_t get_uint() { return atol(&*begin); }
            float get_float() { return atof(&*begin); };
            String get_string();

            bool operator==(const char *str) const { return std::equal(begin, end, str); }
            bool if_heading_skip(const char *str);

            char *c_str() { return &*begin; }
            size_t len() { return end - begin; }
        };

        Buffer(FILE &file);
        bool read_line(); // read first 80 characters, discard the rest

        String line;
    };

    const char *gcode_file_path; /**< stores current gcode file path */
    const char *gcode_file_name; /**< stores current gcode file name */

    /** Set static variable for gcode filename
     *  @param[in] file - gcode file reference
     *  @param[in] size - thumbnail wanted size
     *  @return True - if has thumbnail with those size parameters
     */
    bool hasThumbnail(FILE *file, size_ui16_t size);
    GCodeInfo();
    GCodeInfo(const GCodeInfo &) = delete;

    void parse_gcode(Buffer::String cmd, uint32_t &gcode_counter);
    void parse_comment(Buffer::String cmd);
    bool is_up_to_date(GCodeInfo::ValidPrinterSettings::GcodeFwVersion &parsed, const char *new_version);

    /**
     * @brief Test printer model with a list of compatible models.
     * @tparam SIZE size of the compatibility_list
     * @param printer printer model to test
     * @param compatibility_list list of compatible models
     * @return true if printer is compatible with any of the models in compatibility_list
     */
    template <std::size_t SIZE>
    bool is_printer_compatible(const Buffer::String &printer, const std::array<const char *, SIZE> &compatibility_list) {
        return std::any_of(begin(compatibility_list),
            end(compatibility_list),
            [&](const auto &v) { return printer == v; });
    }

    // Search this many last bytes for "metadata" comments.
    // With increasing size of the comment section, this will have to be increased either
    static constexpr const size_t search_last_x_bytes = 50000;

    // search this many g-code at the beginning of the file for the M862.x g-codes
    static constexpr const uint32_t search_first_x_gcodes = 200;
};

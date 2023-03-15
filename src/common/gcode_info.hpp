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
#include "eeprom.h"
#include "marlin_stubs/PrusaGcodeSuite.hpp"
#include <option/has_toolchanger.h>

#include <array>

// these strings are meant NOT to be translated
namespace gcode_info {
static constexpr const char *time = "estimated printing time (normal mode)";
static constexpr const char *filament_type = "filament_type";
static constexpr const char *filament_mm = "filament used [mm]";
static constexpr const char *filament_g = "filament used [g]";
static constexpr const char *printer = "printer_model";
static constexpr const char *m862 = "M862.";
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
        { "MINI", 120 },
        { "MK404", 130 },
        { "IXL", 160 },
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
private:
public:
    static constexpr const uint32_t printer_model_code = printer_model2code(PRINTER_MODEL);
    static constexpr uint32_t gcode_level = GCODE_LEVEL;
    static constexpr const char *printer_model = PRINTER_MODEL;

    using time_buff = std::array<char, 16>;
    using filament_buff = std::array<char, 8>;

    enum class GI_INIT_t {
        PREVIEW,
        PRINT,
    };

    struct ExtruderInfo {
        std::optional<filament_buff> filament_name; /**< stores string representation of filament type */
        std::optional<float> filament_used_g;       /**< stores how much filament will be used for this print (weight) */
        std::optional<float> filament_used_mm;      /**< stores how much filament will be used for this print (distance) */

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
        enum class Severity : uint8_t {
            Ignore = 0,
            Warning = 1,
            Abort = 2
        };

        class Feature {
            bool wrong { false };
            Severity severity;

        public:
            Feature(eevar_id id)
                : severity(static_cast<Severity>(eeprom_get_ui8(id))) {}

            Severity get_severity() const { return severity; }
            bool is_valid() const { return !wrong || severity == Severity::Ignore; }
            bool is_fatal() const { return wrong && severity == Severity::Abort; }
            void set(bool v) { wrong = v; }
        };

        std::vector<Feature> wrong_nozzle_diameters { HOTENDS, eevar_id::EEVAR_HWCHECK_NOZZLE }; // M862.1 disagree (or M862.10 - M862.15 for multihotend gcode)
        Feature wrong_printer_model { eevar_id::EEVAR_HWCHECK_MODEL };                           // M862.2 or M862.3 or printer_model (from comments) disagree
        Feature wrong_firmware_version { eevar_id::EEVAR_HWCHECK_FIRMW };                        // M862.4 disagree
        Feature wrong_gcode_level { eevar_id::EEVAR_HWCHECK_GCODE };                             // M862.5 disagree

        /**
         * @brief Are all nozzle diameters valid?
         * @return true if all wrong_nozzle_diameters[e].is_valid()
         */
        bool nozzle_diameters_valid() const;

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

    // TODO private
    FILE *file;                  /**< gcode file, nullptr == not opened */
    time_buff printing_time;     /**< stores string representation of printing time left */
    bool has_preview_thumbnail;  /**< true if gcode has preview thumbnail */
    bool has_progress_thumbnail; /**< true if gcode has progress thumbnail */
    bool filament_described;     /**< filament info was found in gcode's comments */
    ValidPrinterSettings valid_printer_settings;

    GCodePerExtruderInfo per_extruder_info;

    /** Get static instance of the singleton
     */
    static GCodeInfo &getInstance();

    /** Get number of used extruders
     */
    int UsedExtrudersCount();

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

    static void PreviewInit(FILE &file, time_buff &printing_time, GCodePerExtruderInfo &per_extruder_info,
        bool &filament_described, ValidPrinterSettings &valid_printer_settings);

private:
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

            bool operator==(const char *str) { return std::equal(begin, end, str); }
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

    static void parse_gcode(Buffer::String cmd, uint32_t &gcode_counter, GCodeInfo::ValidPrinterSettings &valid_printer_settings);
    static void parse_comment(Buffer::String cmd, time_buff &printing_time, GCodePerExtruderInfo &per_extruder_info, bool &filament_described, GCodeInfo::ValidPrinterSettings &valid_printer_settings);

    // Search this many last bytes for "metadata" comments.
    // With increasing size of the comment section, this will have to be increased either
    static constexpr const size_t search_last_x_bytes = 25000;

    // search this many g-code at the beginning of the file for the M862.x g-codes
    static constexpr const uint32_t search_first_x_gcodes = 200;
};

/**
 * @file gcode_info.hpp
 * @author Michal Rudolf
 * @brief Structure that extracts and holds gcode comment info
 * @date 2021-03-25
 */
#pragma once

#include "guitypes.hpp"
#include "i18n.h"

#include <array>

//these strings are meant NOT to be translated
namespace gcode_info {
static constexpr const char *time = "estimated printing time (normal mode)";
static constexpr const char *filament_type = "filament_type";
static constexpr const char *filament_mm = "filament used [mm]";
static constexpr const char *filament_g = "filament used [g]";
static constexpr const char *printer = "printer_model";
};

class GCodeInfo {
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

public:
    using time_buff = std::array<char, 16>;
    using filament_buff = std::array<char, 8>;

    enum class GI_INIT_t {
        PREVIEW,
        PRINT,
    };

    // TODO private
    FILE *file;                  /**< gcode file, nullptr == not opened */
    time_buff printing_time;     /**< stores string representation of printing time left */
    filament_buff filament_type; /**< stores string representation of filament type */
    unsigned filament_used_g;    /**< stores how much filament will be used for this print (weight) */
    unsigned filament_used_mm;   /**< stores how much filament will be used for this print (distance) */
    bool has_preview_thumbnail;  /**< true if gcode has preview thumbnail */
    bool has_progress_thumbnail; /**< true if gcode has progress thumbnail */
    bool filament_described;     /**< filament info was found in gcode's comments */
    bool valid_printer_settings; /**< validation check result */

    /** Get static instance of the singleton
     */
    static GCodeInfo &getInstance();

    /** Check if settings were validated successfully
     */
    bool IsSettingsValid();

    /** Set static variable for gcode filename
     *  @param[in] fname - aquired filename
     */
    void SetGcodeFilename(const char *fname);

    /** Set static variable for gcode filepath
     *  @param[in] fpath - aquired filename
     */
    void SetGcodeFilepath(const char *fpath);

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

    static void PreviewInit(FILE &file, time_buff &printing_time, filament_buff &filament_type,
        unsigned &filament_used_g, unsigned &filament_used_mm,
        bool &filament_described, bool &valid_printer_settings);
};

/**
 * @file gcode_info.hpp
 * @author Michal Rudolf
 * @brief Structure that extracts and holds gcode comment info
 * @date 2021-03-25
 */
#pragma once

#include "guitypes.hpp"
#include "i18n.h"

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
    bool valid_printer_settings; /**< validation check result */

    /** Set static variable for gcode filename
     *  @param[in] file - gcode file reference
     *  @param[in] size - thumbnail wanted size
     *  @return True - if has thumbnail with those size parameters
     */
    bool hasThumbnail(FILE *file, size_ui16_t size);
    GCodeInfo();
    GCodeInfo(const GCodeInfo &) = delete;

public:
    enum class GI_INIT_t {
        PREVIEW,
    };
    size_t static constexpr filament_type_len = 8;
    FILE *file;                 /**< gcode file */
    bool file_opened;           /**< stores if file is opened */
    char printing_time[16];     /**< stores string representation of printing time left */
    char filament_type[8];      /**< stores string representation of filament type */
    unsigned filament_used_g;   /**< stores how much filament will be used for this print (weight) */
    unsigned filament_used_mm;  /**< stores how much filament will be used for this print (distance) */
    bool has_preview_thumbnail; /**< true if gcode has preview thumbanil */
    bool filament_described;    /**< filament info was found in gcode's comments */

    /** Get static instance of the singleton
     */
    static GCodeInfo &getInstance();

    /** Check if settings were validated succesfuly
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
    bool initFile(GI_INIT_t init = GI_INIT_t::PREVIEW);

    /** Closes gcode file
     */
    void deinitFile();
};

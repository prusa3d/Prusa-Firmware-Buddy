#include "gcode_info.hpp"
#include "gcode_file.h"
#include "GuiDefaults.hpp"
#include "gcode_thumb_decoder.h"
#include "str_utils.hpp"

GCodeInfo &GCodeInfo::getInstance() {
    static GCodeInfo instance;
    return instance;
}

bool GCodeInfo::IsSettingsValid() {
    return valid_printer_settings;
}

void GCodeInfo::Init(const char *fname, const char *fpath) {
    gcode_file_name = fname;
    gcode_file_path = fpath;
}

const char *GCodeInfo::GetGcodeFilename() {
    return gcode_file_name;
}

const char *GCodeInfo::GetGcodeFilepath() {
    return gcode_file_path;
}

bool GCodeInfo::hasThumbnail(FILE *file, size_ui16_t size) {
    FILE f = { 0 };
    bool thumbnail_valid = false;
    GCodeThumbDecoder gd(file, size.w, size.h, true);
    if (f_gcode_thumb_open(&gd, &f) == 0) {
        char buffer;
        thumbnail_valid = fread((void *)&buffer, 1, 1, &f) > 0;
        f_gcode_thumb_close(&f);
    }
    return thumbnail_valid;
}

GCodeInfo::GCodeInfo()
    : gcode_file_path(nullptr)
    , gcode_file_name(nullptr)
    , file(nullptr)
    , printing_time { "?" }
    , filament_type { "?" }
    , filament_used_g(0)
    , filament_used_mm(0)
    , has_preview_thumbnail(false)
    , has_progress_thumbnail(false)
    , filament_described(false)
    , valid_printer_settings(true) // why true ???
{
}

void GCodeInfo::initFile(GI_INIT_t init) {
    deinitFile();
    if (!gcode_file_path || (file = fopen(gcode_file_path, "r")) == nullptr) {
        return;
    }
    // thumbnail presence check
    has_preview_thumbnail = hasThumbnail(file, GuiDefaults::PreviewThumbnailRect.Size());

    has_progress_thumbnail = hasThumbnail(file, GuiDefaults::ProgressThumbnailRect.Size());

    if (init == GI_INIT_t::PREVIEW && file)
        PreviewInit(*file, printing_time, filament_type, filament_used_g, filament_used_mm, filament_described, valid_printer_settings);
}

void GCodeInfo::PreviewInit(FILE &file, time_buff &printing_time, filament_buff &filament_type,
    unsigned &filament_used_g, unsigned &filament_used_mm,
    bool &filament_described, bool &valid_printer_settings) {
    // TODO read this value from comment, if it does not contain it, it must be added!!!
    if (fseek(&file, -f_gcode_search_last_x_bytes, SEEK_END) != 0) {
        fseek(&file, 0, SEEK_SET);
    }
    char name_buffer[64];
    char value_buffer[32];
    CStrEqual name_comparer(name_buffer, sizeof(name_buffer));

    while (f_gcode_get_next_comment_assignment(
        &file, name_buffer, sizeof(name_buffer), value_buffer, sizeof(value_buffer))) {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
        if (name_comparer(gcode_info::time)) {
            snprintf(printing_time.begin(), printing_time.size(), "%s", value_buffer);
        } else if (name_comparer(gcode_info::filament_type)) {
            snprintf(filament_type.begin(), filament_type.size(), "%s", value_buffer);
            filament_described = true;
        } else if (name_comparer(gcode_info::filament_mm)) {
            sscanf(value_buffer, "%u", &filament_used_mm);
        } else if (name_comparer(gcode_info::filament_g)) {
            sscanf(value_buffer, "%u", &filament_used_g);
        } else if (name_comparer(gcode_info::printer)) {
            if (strncmp(value_buffer, PRINTER_MODEL, sizeof(value_buffer)) == 0) {
                valid_printer_settings = true; // GCODE settings suits this printer model
            } else {
                valid_printer_settings = false; // GCODE is for another Prusa printer model
            }
        }
#pragma GCC diagnostic pop
    }
}

void GCodeInfo::deinitFile() {
    if (file) {
        fclose(file);
        file = nullptr;
        has_preview_thumbnail = false;
        has_progress_thumbnail = false;
        filament_described = false;
        valid_printer_settings = true;
        filament_type[0] = 0;
        printing_time[0] = 0;
        filament_used_g = 0;
        filament_used_mm = 0;
    }
}

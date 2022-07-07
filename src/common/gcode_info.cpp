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

void GCodeInfo::SetGcodeFilename(const char *fname) {
    gcode_file_name = fname;
}

const char *GCodeInfo::GetGcodeFilename() {
    return gcode_file_name;
}

void GCodeInfo::SetGcodeFilepath(const char *fpath) {
    gcode_file_path = fpath;
}

const char *GCodeInfo::GetGcodeFilepath() {
    return gcode_file_path;
}

bool GCodeInfo::hasThumbnail(FILE *file, size_ui16_t size) {
    FILE f = { 0 };
    bool thumbnail_valid = false;
    GCodeThumbDecoder gd(file, size.w, size.h);
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
    , valid_printer_settings(true)
    , file_opened(true)
    , printing_time("?") // not a standard, but GCC allow this
    , filament_type("?") // not a standard, but GCC allow this
    , filament_used_g(0)
    , filament_used_mm(0)
    , has_preview_thumbnail(false)
    , filament_described(false) {
}

bool GCodeInfo::initFile(GI_INIT_t init) {
    deinitFile();
    if (!gcode_file_path || (file = fopen(gcode_file_path, "r")) == nullptr) {
        return false;
    }
    file_opened = true;
    // thumbnail presence check
    has_preview_thumbnail = hasThumbnail(file, { 0, 0 });

    if (init == GI_INIT_t::PREVIEW) {
        // TODO read this value from comment, if it does not contain it, it must be added!!!
        const int search_last_x_bytes = 14000; // With increasing size of the comment section, this will have to be increased either
        if (fseek(file, -search_last_x_bytes, SEEK_END) != 0) {
            fseek(file, 0, SEEK_SET);
        }
        char name_buffer[64];
        char value_buffer[32];
        CStrEqual name_comparer(name_buffer, sizeof(name_buffer));

        while (f_gcode_get_next_comment_assignment(
            file, name_buffer, sizeof(name_buffer), value_buffer, sizeof(value_buffer))) {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
            if (name_comparer(gcode_info::time)) {
                snprintf(printing_time, sizeof(printing_time), "%s", value_buffer);
            } else if (name_comparer(gcode_info::filament_type)) {
                snprintf(filament_type, sizeof(filament_type), "%s", value_buffer);
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
    return true;
}

void GCodeInfo::deinitFile() {
    if (file_opened) {
        fclose(file);
        file_opened = false;
        has_preview_thumbnail = false;
    }
}

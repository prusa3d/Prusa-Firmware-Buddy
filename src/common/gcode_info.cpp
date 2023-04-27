#include "gcode_info.hpp"
#include "gcode_file.h"
#include "GuiDefaults.hpp"
#include "gcode_thumb_decoder.h"
#include "str_utils.hpp"
#include <cstring>
#include <option/developer_mode.h>
#include <Marlin/src/module/motion.h>
#include <version.h>

#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif /*HAS_TOOLCHANGER()*/

GCodeInfo &GCodeInfo::getInstance() {
    static GCodeInfo instance;
    return instance;
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
    FILE f {};
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
    : file(nullptr)
    , printing_time { "?" }
    , has_preview_thumbnail(false)
    , has_progress_thumbnail(false)
    , filament_described(false)
    , per_extruder_info()
    , gcode_file_path(nullptr)
    , gcode_file_name(nullptr) {
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
        PreviewInit(*file, printing_time, per_extruder_info, filament_described, valid_printer_settings);
}

int GCodeInfo::UsedExtrudersCount() {
    int count = 0;
    for (const auto &extruder_info : per_extruder_info) {
        if (extruder_info.used()) {
            count += 1;
        }
    }
    return count;
}

void GCodeInfo::deinitFile() {
    if (file) {
        fclose(file);
        file = nullptr;
        has_preview_thumbnail = false;
        has_progress_thumbnail = false;
        filament_described = false;
        valid_printer_settings = ValidPrinterSettings();
        per_extruder_info.fill({});
        printing_time[0] = 0;
    }
}

bool GCodeInfo::ValidPrinterSettings::nozzle_diameters_valid() const {
    HOTEND_LOOP() {
#if HAS_TOOLCHANGER()
        if (!prusa_toolchanger.is_tool_enabled(e)) {
            continue;
        }
#endif /*HAS_TOOLCHANGER()*/
        if (!wrong_nozzle_diameters[e].is_valid()) {
            return false;
        }
    }

    return true;
}

bool GCodeInfo::ValidPrinterSettings::is_valid() const {
    return nozzle_diameters_valid() && wrong_printer_model.is_valid() && wrong_gcode_level.is_valid() && wrong_firmware.is_valid() && mk3_compatibility_mode.is_valid();
}

bool GCodeInfo::ValidPrinterSettings::is_fatal() const {
    HOTEND_LOOP() {
#if HAS_TOOLCHANGER()
        if (!prusa_toolchanger.is_tool_enabled(e)) {
            continue;
        }
#endif /*HAS_TOOLCHANGER()*/
        if (wrong_nozzle_diameters[e].is_fatal()) {
            return true;
        }
    }
    return wrong_printer_model.is_fatal() || wrong_gcode_level.is_fatal() || wrong_firmware.is_fatal() || mk3_compatibility_mode.is_fatal();
}

GCodeInfo::Buffer::Buffer(FILE &file)
    : file(file) {}

bool GCodeInfo::Buffer::read_line() {
    line.begin = begin(buffer);
    line.end = begin(buffer);

    for (;;) {
        int c = fgetc(&file);
        if (c == EOF) {
            return !line.is_empty();
        }
        if (c == '\r' || c == '\n') {
            if (line.is_empty()) {
                continue; // skip blank lines
            } else {
                // null terminate => safe atof+atol
                (line.end == end(buffer)) ? *(line.end - 1) : *line.end = '\0';
                return true;
            }
        }
        if (line.end != end(buffer)) {
            *line.end++ = c;
        }
    }
}

void GCodeInfo::Buffer::String::skip(size_t amount) {
    begin += std::min(amount, static_cast<size_t>(end - begin));
}

void GCodeInfo::Buffer::String::skip_ws() {
    skip([](auto c) -> bool { return isspace(c); });
}

void GCodeInfo::Buffer::String::skip_nws() {
    skip([](auto c) -> bool { return !isspace(c); });
}

void GCodeInfo::Buffer::String::trim() {
    skip_ws();
    while (begin != end && *(end - 1) == ' ') {
        --end;
    }
}

GCodeInfo::Buffer::String GCodeInfo::Buffer::String::get_string() {
    skip_ws();
    if (begin != end && *begin == '"') {
        auto quote = std::find(begin + 1, end, '"');
        if (quote != end) {
            return String(begin + 1, quote);
        }
    }
    return String(end, end);
}

bool GCodeInfo::Buffer::String::if_heading_skip(const char *str) {
    for (auto it = begin;; ++it, ++str) {
        if (*str == '\0') {
            begin = it;
            return true;
        }
        if (it == end || *it != *str) {
            return false;
        }
    }
}

void GCodeInfo::parse_version(GCodeInfo::ValidPrinterSettings &valid_printer_settings, const char *version) {
    // Parse version from G-code and from this firmware
    // Parse and store version from G-code to be displayed later
    ValidPrinterSettings::GcodeFwVersion &g_fw = valid_printer_settings.gcode_fw_version;
    if (sscanf(version, "%d.%d.%d", &g_fw.major, &g_fw.minor, &g_fw.patch) != 3) {
        return;
    }

    // Parse version of this firmware
    ValidPrinterSettings::GcodeFwVersion v_fw;
    if (sscanf(project_version, "%d.%d.%d", &v_fw.major, &v_fw.minor, &v_fw.patch) != 3) {
        bsod("Internal project_version cannot be parsed with \"%d.%d.%d\"");
    }

    if (g_fw.major > v_fw.major) { // Major is higher
        valid_printer_settings.wrong_firmware.fail();
    }

    if (g_fw.major == v_fw.major && g_fw.minor > v_fw.minor) { // Minor is higher
        valid_printer_settings.wrong_firmware.fail();
    }

    if (g_fw.major == v_fw.major && g_fw.minor == v_fw.minor && g_fw.patch > v_fw.patch) { // Patch is higher
        valid_printer_settings.wrong_firmware.fail();
    }

    // Ignore everything behind patch number
}

void GCodeInfo::parse_gcode(GCodeInfo::Buffer::String cmd, uint32_t &gcode_counter, GCodeInfo::ValidPrinterSettings &valid_printer_settings) {
    cmd.skip_ws();
    if (cmd.front() == ';' || cmd.is_empty()) {
        return;
    }
    gcode_counter++;

    // skip line number if present
    if (cmd.front() == 'N') {
        cmd.skip(2u);
        cmd.skip([](auto c) -> bool { return isdigit(c) || isspace(c); });
    }

    if (cmd.if_heading_skip(gcode_info::m862)) {
        char subcode = cmd.pop_front();
        cmd.skip_ws();

        // Default is current tool or first
#if HAS_TOOLCHANGER()
        uint8_t tool = (active_extruder != PrusaToolChanger::MARLIN_NO_TOOL_PICKED) ? active_extruder : 0;
#else  /*HAS_TOOLCHANGER()*/
        uint8_t tool = 0;
#endif /*HAS_TOOLCHANGER()*/

        // Parse parameters
        float p_diameter = NAN;
        while (!cmd.is_empty()) {
            char letter = cmd.pop_front();
            if (letter == 'T') {
                tool = cmd.get_uint(); // Check particular tool (only for M862.1)
            } else if (letter == 'P') {
                switch (subcode) {
                case '1':
                    p_diameter = cmd.get_float(); // Only store value in case Tx comes later
                    break;
                case '3': {
#if ENABLED(GCODE_COMPATIBILITY_MK3)
                    if (strncmp(cmd.get_string().c_str(), "MK3", 3) == 0) {
                        valid_printer_settings.mk3_compatibility_mode.fail();
                    }
#endif
                    if (cmd.get_string() != printer_model) {
                        valid_printer_settings.wrong_printer_model.fail();
                    }
                    break;
                }
                case '4':
                    parse_version(valid_printer_settings, cmd.c_str());
                    break;
                case '2':
                    if (cmd.get_uint() != printer_model_code) {
                        valid_printer_settings.wrong_printer_model.fail();
                    }
                    break;
                case '5':
                    if (cmd.get_uint() != gcode_level) {
                        valid_printer_settings.wrong_gcode_level.fail();
                    }
                    break;
                }
            }
            cmd.skip_nws();
            cmd.skip_ws();
        }

        // Check nozzle diameter
        if (!isnan(p_diameter) && tool < HOTENDS) {
            static_assert(EEPROM_MAX_TOOL_COUNT >= HOTENDS, "Not enough nozzles stored in EEPROM");
            if (p_diameter != eeprom_get_nozzle_dia(tool)) {
                valid_printer_settings.wrong_nozzle_diameters[tool].fail();
            }
        }
    }

    // Parse M115 Ux.yy.z for newer firmware
    if (cmd.if_heading_skip(gcode_info::m115)) {
        cmd.skip_ws();
        if (cmd.pop_front() == 'U') {
            *(cmd.end - 1) = '\0'; // Terminate string if not already

            parse_version(valid_printer_settings, cmd.c_str());
        }
    }
}

void GCodeInfo::parse_comment(GCodeInfo::Buffer::String comment, time_buff &printing_time, GCodePerExtruderInfo &per_extruder_info, bool &filament_described, GCodeInfo::ValidPrinterSettings &valid_printer_settings) {
    comment.skip_ws();
    if (comment.pop_front() != ';') {
        return;
    }
    auto equal = std::find(comment.begin, comment.end, '=');

    if (equal == comment.end) { // not found
        return;
    }
    auto name = Buffer::String(comment.begin, equal - 1);
    auto val = Buffer::String(equal + 1, comment.end);
    name.trim();
    val.trim();

    if (name == gcode_info::time) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
        snprintf(printing_time.begin(), printing_time.size(), "%s", val.c_str());
#pragma GCC diagnostic pop
    } else {
        bool is_filament_type = name == gcode_info::filament_type;
        bool is_filament_used_mm = name == gcode_info::filament_mm;
        bool is_filament_used_g = name == gcode_info::filament_g;

        if (is_filament_type || is_filament_used_g || is_filament_used_mm) {
            std::span<char> value(val.c_str(), val.len());
            int extruder = 0;
            while (std::optional<std::span<char>> item = f_gcode_iter_items(value, is_filament_type ? ';' : ',')) {
                if (item.has_value() == false) {
                    break;
                }
                if (is_filament_type) {
                    filament_buff filament_name;
                    snprintf(filament_name.begin(), filament_name.size(), "%.*s", item->size(), item->data());
                    per_extruder_info[extruder].filament_name = filament_name;
                    filament_described = true;
                } else if (is_filament_used_mm) {
                    float filament_used_mm;
                    sscanf(item->data(), "%f", &filament_used_mm);
                    per_extruder_info[extruder].filament_used_mm = filament_used_mm;
                } else if (is_filament_used_g) {
                    float filament_used_g;
                    sscanf(item->data(), "%f", &filament_used_g);
                    per_extruder_info[extruder].filament_used_g = filament_used_g;
                }
                extruder++;
            }
        } else if (name == gcode_info::printer) {
            if (val != printer_model) {
                valid_printer_settings.wrong_printer_model.fail();
            }
        }
    }
}

void GCodeInfo::PreviewInit(FILE &file, time_buff &printing_time, GCodePerExtruderInfo &per_extruder_info, bool &filament_described, GCodeInfo::ValidPrinterSettings &valid_printer_settings) {
    valid_printer_settings = ValidPrinterSettings(); // reset to valid state
    per_extruder_info = {};                          // Reset extruder info

    fseek(&file, 0, SEEK_SET);
    Buffer buffer(file);

    // parse first 'f_gcode_search_first_x_gcodes' g-codes from the beginning of the file
    for (uint32_t gcode_counter = 0; gcode_counter < search_first_x_gcodes && buffer.read_line();) {
        parse_comment(buffer.line, printing_time, per_extruder_info, filament_described, valid_printer_settings);
        parse_gcode(buffer.line, gcode_counter, valid_printer_settings);
    }

    // parse last f_gcode_search_last_x_bytes at the end of the file
    if (fseek(&file, -search_last_x_bytes, SEEK_END) != 0) {
        fseek(&file, 0, SEEK_SET);
    }
    [[maybe_unused]] uint32_t gcode_counter = 0;
    while (buffer.read_line()) {
        parse_comment(buffer.line, printing_time, per_extruder_info, filament_described, valid_printer_settings);
        parse_gcode(buffer.line, gcode_counter, valid_printer_settings);
    }
}

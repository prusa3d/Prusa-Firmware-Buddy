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

#include <configuration_store.hpp>

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
    : file_mutex_id(osMutexCreate(osMutex(file_mutex)))
    , file(nullptr)
    , printing_time { "?" }
    , preview_thumbnail(false)
    , progress_thumbnail(false)
    , filament_described(false)
    , per_extruder_info()
    , gcode_file_path(nullptr)
    , gcode_file_name(nullptr) {
}

void GCodeInfo::initFile(GI_INIT_t init) {
    deinitFile();

    auto fl = FileLender(file, file_mutex_id); // Lock the file

    if (!gcode_file_path || (file = fopen(gcode_file_path, "r")) == nullptr) {
        return;
    }

    // thumbnail presence check
    if (file) {
        preview_thumbnail = hasThumbnail(file, GuiDefaults::PreviewThumbnailRect.Size());
    }

    if (file) {
        progress_thumbnail = hasThumbnail(file, GuiDefaults::ProgressThumbnailRect.Size());
    }

    // scan info G-codes and comments
    if (init == GI_INIT_t::FULL && file) {
        PreviewInit();
    }
}

int GCodeInfo::UsedExtrudersCount() const {
    return std::count_if(per_extruder_info.begin(), per_extruder_info.end(),
        [](auto &info) { return info.used(); });
}

int GCodeInfo::GivenExtrudersCount() const {
    return std::count_if(per_extruder_info.begin(), per_extruder_info.end(),
        [](auto &info) { return info.given(); });
}

void GCodeInfo::deinitFile() {
    auto fl = FileLender(file, file_mutex_id); // Lock the file

    if (file) {
        fclose(file);
        file = nullptr;
        preview_thumbnail = false;
        progress_thumbnail = false;
        filament_described = false;
        valid_printer_settings = ValidPrinterSettings();
        per_extruder_info.fill({});
        printing_time[0] = 0;
    }
}

void GCodeInfo::EvaluateToolsValid() {
    HOTEND_LOOP() {
        // do not check this nozzle if not used in print
        if (!per_extruder_info[e].used())
            continue;

#if HAS_TOOLCHANGER()
        // tool is used in gcode, but not enabled in printer
        if (!prusa_toolchanger.is_tool_enabled(e)) {
            valid_printer_settings.wrong_tools.fail();
        }
#endif

        // nozzle diameter of this tool in gcode is different then printer has
        if (per_extruder_info[e].nozzle_diameter.has_value() && per_extruder_info[e].nozzle_diameter != config_store().get_nozzle_diameter(e)) {
            valid_printer_settings.wrong_nozzle_diameter.fail();
        }
    }
}

void GCodeInfo::ValidPrinterSettings::add_unsupported_feature(const char *feature, size_t length) {
    unsupported_features = true;
    size_t occupied = strlen(unsupported_features_text);
    size_t free = sizeof(unsupported_features_text) - occupied;
    if (occupied == 0) {
        strncpy(unsupported_features_text, feature, min(length, free));
    } else if (free > length + 2) {
        strcat(unsupported_features_text, ", ");
        strncat(unsupported_features_text, feature, length);
    } else {
        strcpy(unsupported_features_text + sizeof(unsupported_features_text) - 4, "...");
    }
}

bool GCodeInfo::ValidPrinterSettings::is_valid() const {
    return wrong_tools.is_valid() && wrong_nozzle_diameter.is_valid() && wrong_printer_model.is_valid() && wrong_gcode_level.is_valid() && wrong_firmware.is_valid() && mk3_compatibility_mode.is_valid() && !unsupported_features;
}

bool GCodeInfo::ValidPrinterSettings::is_fatal() const {
    return wrong_tools.is_fatal() || wrong_nozzle_diameter.is_fatal() || wrong_printer_model.is_fatal() || wrong_gcode_level.is_fatal() || wrong_firmware.is_fatal() || mk3_compatibility_mode.is_fatal() || unsupported_features;
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

bool GCodeInfo::is_up_to_date(GCodeInfo::ValidPrinterSettings::GcodeFwVersion &parsed, const char *new_version_string) {
    // Parse and store version from G-code
    if (sscanf(new_version_string, "%d.%d.%d", &parsed.major, &parsed.minor, &parsed.patch) != 3) {
        return true;
    }

    // Parse version of this firmware
    ValidPrinterSettings::GcodeFwVersion v_fw;
    if (sscanf(project_version, "%d.%d.%d", &v_fw.major, &v_fw.minor, &v_fw.patch) != 3) {
        bsod("Internal project_version cannot be parsed with \"%d.%d.%d\"");
    }

    if (parsed.major > v_fw.major) { // Major is higher
        return false;
    }

    if (parsed.major == v_fw.major && parsed.minor > v_fw.minor) { // Minor is higher
        return false;
    }

    if (parsed.major == v_fw.major && parsed.minor == v_fw.minor && parsed.patch > v_fw.patch) { // Patch is higher
        return false;
    }

    // Ignore everything behind patch number
    return true;
}

void GCodeInfo::parse_gcode(GCodeInfo::Buffer::String cmd, uint32_t &gcode_counter) {
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

                    // Check basic printer model as MK4 or XL
                    if (!is_printer_compatible(cmd.get_string(), printer_compatibility_list)) {
                        valid_printer_settings.wrong_printer_model.fail();
                    }
                    break;
                }
                case '4':
                    // Parse M862.4 for minimal required firmware version
                    if (!is_up_to_date(valid_printer_settings.gcode_fw_version, cmd.c_str())) {
                        valid_printer_settings.wrong_firmware.fail();
                    }
                    break;
                case '2':
                    if (cmd.get_uint() != printer_model_code) {
                        valid_printer_settings.wrong_printer_model.fail();
                    }
                    break;
                case '5':
                    if (cmd.get_uint() > gcode_level) {
                        valid_printer_settings.wrong_gcode_level.fail();
                    }
                    break;
                case '6':
                    auto compare = [](GCodeInfo::Buffer::String &a, const char *b) {
                        for (char *c = a.begin;; ++c, ++b) {
                            if (c == a.end || *b == '\0')
                                return c == a.end && *b == '\0';
                            if (toupper(*c) != toupper(*b))
                                return false;
                        }
                        return *b == '\0';
                    };
                    auto find = [&](GCodeInfo::Buffer::String feature) {
                        for (auto &f : PrusaGcodeSuite::m862_6SupportedFeatures)
                            if (compare(feature, f))
                                return true;
                        return false;
                    };
                    auto feature = cmd.get_string();
                    feature.trim();
                    if (!find(feature))
                        valid_printer_settings.add_unsupported_feature(feature.begin, feature.end - feature.begin);
                    break;
                }
            }
            cmd.skip_nws();
            cmd.skip_ws();
        }

        // store nozzle diameter
        if (!isnan(p_diameter) && tool < EXTRUDERS) {
            per_extruder_info[tool].nozzle_diameter = p_diameter;
        }
    }

    // Parse M115 Ux.yy.z for newer firmware info
    if (cmd.if_heading_skip(gcode_info::m115)) {
        cmd.skip_ws();
        if (cmd.pop_front() == 'U') {
            *(cmd.end - 1) = '\0'; // Terminate string if not already

            if (!is_up_to_date(valid_printer_settings.latest_fw_version, cmd.c_str())) {
                valid_printer_settings.outdated_firmware.fail();
            }
        }
    }
}

void GCodeInfo::parse_comment(GCodeInfo::Buffer::String comment) {
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
            // Check model with possible extensions as MK4, MK4IS, XL or XL5
            if (!is_printer_compatible(val, printer_extended_compatibility_list)) {
                valid_printer_settings.wrong_printer_model.fail();
            }
        }
    }
}

void GCodeInfo::PreviewInit() {
    valid_printer_settings = ValidPrinterSettings(); // reset to valid state
    per_extruder_info = {};                          // Reset extruder info

    fseek(file, 0, SEEK_SET);
    Buffer buffer(*file);

    // parse first 'f_gcode_search_first_x_gcodes' g-codes from the beginning of the file
    for (uint32_t gcode_counter = 0; gcode_counter < search_first_x_gcodes && buffer.read_line();) {
        parse_comment(buffer.line);
        parse_gcode(buffer.line, gcode_counter);
    }

    // parse last f_gcode_search_last_x_bytes at the end of the file
    if (fseek(file, -search_last_x_bytes, SEEK_END) != 0) {
        fseek(file, 0, SEEK_SET);
    }
    uint32_t gcode_counter = 0;
    while (buffer.read_line()) {
        parse_comment(buffer.line);
        parse_gcode(buffer.line, gcode_counter);
    }
    // now that we have parsed all values, evaluate information about tools
    EvaluateToolsValid();
}

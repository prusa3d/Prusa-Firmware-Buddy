#include "gcode_info.hpp"
#include "gcode_file.h"
#include "GuiDefaults.hpp"
#include "gcode_thumb_decoder.h"
#include "str_utils.hpp"
#include <cstring>
#include <option/developer_mode.h>
#include <Marlin/src/module/motion.h>
#include <version.h>
#include <option/has_mmu2.h>

#if ENABLED(PRUSA_MMU2)
    #include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_mk4.h"
#endif
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif /*HAS_TOOLCHANGER()*/

#include <config_store/store_instance.hpp>

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

uint32_t printer_model2code(const char *model) {
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
        { "MK3.5", 230 },
        { "MK3.5MMU3", 30230 },
        { "MK3.9", 210 },
        { "MK3.9MMU3", 30210 },
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
    return 0;
}

GCodeInfo::GCodeInfo()
    : printer_model_code(printer_model2code(PRINTER_MODEL))
    , file_mutex_id(osMutexCreate(osMutex(file_mutex)))
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

uint32_t GCodeInfo::getPrinterModelCode() const {
    return printer_model_code;
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
        if (per_extruder_info[e].nozzle_diameter.has_value()) {
            float nozzle_diameter_distance = per_extruder_info[e].nozzle_diameter.value() - config_store().get_nozzle_diameter(e);
            if (nozzle_diameter_distance > 0.001f || nozzle_diameter_distance < -0.001f) {
                valid_printer_settings.wrong_nozzle_diameter.fail();
            }
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

bool GCodeInfo::is_up_to_date(const char *new_version_string) {
    // Parse version from G-code
    // supported formats: MAJOR.MINOR.PATCH, MAJOR.MINOR.PATCH+BUILD_NUMBER, MAJOR.MINOR.PATCH-PRERELEASE+BUILD_NUMBER
    // only MAJOR, MINOR, PATH, BUILD_NUMBER are used for version comparison,
    ValidPrinterSettings::GcodeFwVersion parsed;
    if (sscanf(new_version_string, "%u.%u.%u", &parsed.major, &parsed.minor, &parsed.patch) != 3) {
        return true;
    }
    if (auto *plus = strchr(new_version_string, '+'); !plus || sscanf(plus, "%u", &parsed.build_number) != 1) {
        parsed.build_number = 0;
    }

    if (parsed.major > project_version_major) { // Major is higher
        return false;
    }

    if (parsed.major == project_version_major && parsed.minor > project_version_minor) { // Minor is higher
        return false;
    }

    if (parsed.major == project_version_major && parsed.minor == project_version_minor && parsed.patch > project_version_patch) { // Patch is higher
        return false;
    }

    if (parsed.major == project_version_major && parsed.minor == project_version_minor && parsed.patch == project_version_patch && parsed.build_number && parsed.build_number > unsigned(project_build_number)) { // Suffix is higher
        return false;
    }

    // Ignore everything behind suffix number
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
                    if (strncmp(cmd.get_string().c_str(), "MK3", 3) == 0 && strncmp(cmd.get_string().c_str(), "MK3.", 4) != 0) { // second condition due to MK3.5 & MK3.9
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
                    if (!is_up_to_date(cmd.c_str())) {
                        valid_printer_settings.wrong_firmware.fail();
                    }
                    break;
                case '2':
#if PRINTER_IS_PRUSA_MK4
                    if (!config_store().xy_motors_400_step.get()) {
                        printer_model_code = printer_model2code(MMU2::mmu2.Enabled() ? "MK3.9MMU3" : "MK3.9");
                    } else {
                        printer_model_code = printer_model2code(PRINTER_MODEL);
                    }
#endif
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
            // Terminate string if not already
            // cmd.end is a pointer to 1 character past the end of the string in the zero-terminated pre-allocated buffer, so this is safe
            if (!*cmd.end) {
                *cmd.end = '\0';
            }

            if (!is_up_to_date(cmd.c_str())) {
                valid_printer_settings.outdated_firmware.fail();
                strncpy(valid_printer_settings.latest_fw_version, cmd.c_str(), min(sizeof(valid_printer_settings.latest_fw_version), cmd.len()));
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
            while (std::optional<std::span<char>> item = iterate_items(value, is_filament_type ? ';' : ',')) {
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

    // parse first 'gcode::search_first_x_gcodes' g-codes from the beginning of the file
    for (uint32_t gcode_counter = 0; gcode_counter < gcode::search_first_x_gcodes && buffer.read_line();) {
        parse_comment(buffer.line);
        parse_gcode(buffer.line, gcode_counter);
    }

    // parse last search_last_x_bytes at the end of the file
    if (fseek(file, -gcode::search_last_x_bytes, SEEK_END) != 0) {
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

std::optional<std::span<char>> GCodeInfo::iterate_items(std::span<char> &buffer, char separator) {
    // skip leading spaces
    while (buffer[0] && isspace(*buffer.data())) {
        buffer = buffer.subspan(1);
    }

    // find end of the item
    size_t item_length = 0;
    for (; item_length < buffer.size(); item_length++) {
        if (buffer[item_length] == 0 || buffer[item_length] == separator) {
            break;
        }
    }
    std::span<char> next_buffer = buffer.subspan(buffer[item_length] == separator ? item_length + 1 : item_length);

    // strip trailing whitespaces
    while (item_length && isspace(buffer[item_length - 1])) {
        item_length--;
    }

    auto item = buffer.subspan(0, item_length);
    buffer = next_buffer;
    if (item_length == 0) {
        return std::nullopt;
    }
    return item;
}

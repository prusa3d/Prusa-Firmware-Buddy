#include "gcode_info.hpp"
#include "option/has_gui.h"
#if HAS_GUI()
    #include <guiconfig/GuiDefaults.hpp>
#endif
#include "gcode_thumb_decoder.h"
#include <cstring>
#include <option/developer_mode.h>
#include <Marlin/src/module/motion.h>
#include <version.h>
#include <tools_mapping.hpp>
#include <module/prusa/spool_join.hpp>
#include "mutable_path.hpp"
#include "log.h"
#include <option/has_mmu2.h>

LOG_COMPONENT_REF(Buddy);

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

#if HAS_GUI()
bool GCodeInfo::hasThumbnail(IGcodeReader &reader, size_ui16_t size) {
    return reader.stream_thumbnail_start(size.w, size.h, IGcodeReader::ImgType::QOI);
}
#endif

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
    , printing_time { "?" }
    , has_preview_thumbnail_(false)
    , has_progress_thumbnail_(false)
    , filament_described(false)
    , per_extruder_info()
    , gcode_file_path(nullptr)
    , gcode_file_name(nullptr) {
}

bool GCodeInfo::start_load(AnyGcodeFormatReader &file_reader) {
    reset_info();

    file_reader.open(gcode_file_path);
    if (file_reader.is_open()) {
        start_load_result_ = StartLoadResult::Started;
        check_valid_for_print(file_reader); // This only updates is_valid, will change over the prefetch change
        return true;

    } else {
        error_str_ = N_("Failed to open file");
        start_load_result_ = StartLoadResult::Failed;
        return false;
    }
}

void GCodeInfo::end_load(AnyGcodeFormatReader &file_reader) {
    file_reader.close();
}

bool GCodeInfo::check_still_valid() {
    if (!transfers::is_valid_file_or_transfer(GetGcodeFilepath())) {
        error_str_ = N_("File removed or transfer aborted");
        is_printable_ = false;
        return false;
    }

    return !has_error();
}

bool GCodeInfo::check_valid_for_print(AnyGcodeFormatReader &file_reader) {
    assert(file_reader.is_open());
    auto &reader = *file_reader.get();

    transfers::Transfer::Path path(GetGcodeFilepath());
    reader.update_validity(path);
    is_printable_ = reader.valid_for_print();

    if (reader.has_error()) {
        error_str_ = reader.error_str();
    }

    return is_printable_;
}

bool GCodeInfo::verify_file(AnyGcodeFormatReader &file_reader) {
    assert(file_reader.is_open());

    log_info(Buddy, "Starting file verify...");

    // TODO: enable CRC verification, but now its disabled because it takes ages due to slow USB read and suboptimal implementation
    // For now we're only doing quick verification
    if (auto result = file_reader.get()->verify_file(IGcodeReader::FileVerificationLevel::quick); !result) {
        error_str_ = result.error_str;
        log_info(Buddy, "File verify FAIL: %s", result.error_str);
        return false;
    }

    log_info(Buddy, "File verify OK");
    return true;
}

void GCodeInfo::load(AnyGcodeFormatReader &file_reader) {
    assert(file_reader.is_open()); // assert file is open
#if HAS_GUI()
    has_preview_thumbnail_ = hasThumbnail(*file_reader.get(), GuiDefaults::PreviewThumbnailRect.Size());
    has_progress_thumbnail_ = hasThumbnail(*file_reader.get(), GuiDefaults::ProgressThumbnailRect.Size());
    if (!has_progress_thumbnail_) {
        has_progress_thumbnail_ = hasThumbnail(*file_reader.get(), { GuiDefaults::OldSlicerProgressImgWidth, GuiDefaults::ProgressThumbnailRect.Height() });
    }
#endif
    // scan info G-codes and comments
    PreviewInit(*file_reader.get());
    is_loaded_ = true;
}

int GCodeInfo::UsedExtrudersCount() const {
    return std::count_if(per_extruder_info.begin(), per_extruder_info.end(),
        [](auto &info) { return info.used(); });
}

int GCodeInfo::GivenExtrudersCount() const {
    return std::count_if(per_extruder_info.begin(), per_extruder_info.end(),
        [](auto &info) { return info.given(); });
}

void GCodeInfo::reset_info() {
    is_loaded_ = false;
    is_printable_ = false;
    has_preview_thumbnail_ = false;
    has_progress_thumbnail_ = false;
    filament_described = false;
    valid_printer_settings = ValidPrinterSettings();
    per_extruder_info.fill({});
    printing_time[0] = 0;
    start_load_result_ = StartLoadResult::None;
    error_str_ = {};
}

uint32_t GCodeInfo::getPrinterModelCode() const {
    return printer_model_code;
}

void GCodeInfo::EvaluateToolsValid() {
    EXTRUDER_LOOP() { // e == gcode_tool
        // do not check this nozzle if not used in print
        if (!per_extruder_info[e].used()) {
            continue;
        }

        auto physical_tool = tools_mapping::to_physical_tool(e);
        if (physical_tool == tools_mapping::no_tool) {
            // used but nothing prints this, so teeechnically it's ok from the POV of tool/nozzle
            continue;
        }

#if HAS_TOOLCHANGER()
        // tool is used in gcode, but not enabled in printer
        if (!prusa_toolchanger.is_tool_enabled(physical_tool)) {
            valid_printer_settings.wrong_tools.fail();
        }
#endif

        // nozzle diameter of this tool in gcode is different then printer has
        if (per_extruder_info[e].nozzle_diameter.has_value()) {
            auto do_nozzle_check = [&](uint8_t hotend) {
                assert(hotend < HOTENDS);
                float nozzle_diameter_distance = std::abs(per_extruder_info[e].nozzle_diameter.value() - config_store().get_nozzle_diameter(hotend));
                if (nozzle_diameter_distance > 0.001f) {
                    valid_printer_settings.wrong_nozzle_diameter.fail();
                }
            };

#if ENABLED(SINGLENOZZLE)
            do_nozzle_check(0);
#else
            tools_mapping::execute_on_whole_chain(physical_tool,
                [&](uint8_t physical) {
                    do_nozzle_check(physical); // here should be map to hotend from this extruder but the #if ENABLED(SINGLENOZZLE) should be enough for now
                });
#endif
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

bool GCodeInfo::ValidPrinterSettings::is_valid(bool is_tools_mapping_possible) const {
    return wrong_printer_model.is_valid() && wrong_gcode_level.is_valid() && wrong_firmware.is_valid() && mk3_compatibility_mode.is_valid() && !unsupported_features
        && (is_tools_mapping_possible // if is_possible -> always true -> handled by tools_mapping screen
            || (wrong_tools.is_valid() && wrong_nozzle_diameter.is_valid()));
}

bool GCodeInfo::ValidPrinterSettings::is_fatal(bool is_tools_mapping_possible) const {
    return wrong_printer_model.is_fatal() || wrong_gcode_level.is_fatal() || wrong_firmware.is_fatal() || mk3_compatibility_mode.is_fatal()
        || (!is_tools_mapping_possible // if is_possible -> always false -> handled by tools_mapping screen
            && (wrong_tools.is_fatal() || wrong_nozzle_diameter.is_fatal()));
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

void GCodeInfo::parse_m555(GcodeBuffer::String cmd) {
    // parses print area into bed_preheat_area.
    cmd.skip_ws();
    bed_preheat_area = PrintArea::rect_t::max();

    // W and H arguments require X and Y to be set, to know that flags are required
    bool x_was_set { false };
    bool y_was_set { false };
    bool w_was_set { false };
    bool h_was_set { false };

    //  We don't have order guaranteed; W and H are parsed into a temporary and set later
    float w_to_set { 0.0 };
    float h_to_set { 0.0 };

    while (!cmd.is_empty()) {
        switch (cmd.pop_front()) {
        case 'X':
            bed_preheat_area->a.x = cmd.get_float();
            x_was_set = true;
            break;
        case 'W':
            w_to_set = cmd.get_float();
            w_was_set = true;
            break;
        case 'Y':
            bed_preheat_area->a.y = cmd.get_float();
            y_was_set = true;
            break;
        case 'H':
            h_to_set = cmd.get_float();
            h_was_set = true;
            break;
        }

        cmd.skip_nws();
        cmd.skip_ws();
    }

    if (x_was_set && w_was_set) {
        bed_preheat_area->b.x = bed_preheat_area->a.x + w_to_set;
    }

    if (y_was_set && h_was_set) {
        bed_preheat_area->b.y = bed_preheat_area->a.y + h_to_set;
    }
}

void GCodeInfo::parse_m862(GcodeBuffer::String cmd) {
    {
        // format is M862.x, so remove dot
        char dot = cmd.pop_front();
        if (dot != '.') {
            return;
        }
    }

    char subcode = cmd.pop_front();
    cmd.skip_ws();

    // Parse parameters
    [[maybe_unused]] uint8_t tool = 0; // Default is first tool
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
                if (cmd.get_uint() != printer_model_code && cmd.get_uint() != printer_model2code("MK3.9")) {
#else
                if (cmd.get_uint() != printer_model_code) {
#endif
                    valid_printer_settings.wrong_printer_model.fail();
                }
                break;
            case '5':
                if (cmd.get_uint() > gcode_level) {
                    valid_printer_settings.wrong_gcode_level.fail();
                }
                break;
            case '6':
                auto compare = [](GcodeBuffer::String &a, const char *b) {
                    for (char *c = a.begin;; ++c, ++b) {
                        if (c == a.end || *b == '\0') {
                            return c == a.end && *b == '\0';
                        }
                        if (toupper(*c) != toupper(*b)) {
                            return false;
                        }
                    }
                    return *b == '\0';
                };
                auto find = [&](GcodeBuffer::String feature) {
                    for (auto &f : supported_features) {
                        if (compare(feature, f)) {
                            return true;
                        }
                    }
                    return false;
                };
                auto feature = cmd.get_string();
                feature.trim();

#if ENABLED(PRUSA_MMU2)
                if (MMU2::mmu2.Enabled() && compare(feature, "MMU3")) {
                    break;
                }
#endif

                if (!find(feature)) {
                    valid_printer_settings.add_unsupported_feature(feature.begin, feature.end - feature.begin);
                }
                break;
            }
        }
        cmd.skip_nws();
        cmd.skip_ws();
    }

#if ENABLED(PRUSA_MMU2)
    // Store nozzle diameter - MMU-equipped printers have only one nozzle diameter for all tools/slots
    // Makes the pre-print screen hide the nozzle sizes, which is both good and bad at the same time
    // -> "?.??" is gone, but no actual diameter is shown anymore - that can be tweaked further on the visualization side.
    // Here, we must set the correct nozzle diameter for all tools if specified.
    if (!isnan(p_diameter)) {
        EXTRUDER_LOOP() {
            per_extruder_info[e].nozzle_diameter = p_diameter;
        }
    }
#else
    // store nozzle diameter per tool
    if (!isnan(p_diameter) && tool < EXTRUDERS) {
        per_extruder_info[tool].nozzle_diameter = p_diameter;
    }
#endif
}

void GCodeInfo::parse_gcode(GcodeBuffer::String cmd, uint32_t &gcode_counter) {
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

    if (cmd.skip_gcode(gcode_info::m862)) {
        parse_m862(cmd);
    }

    // Parse M115 Ux.yy.z for newer firmware info
    else if (cmd.skip_gcode(gcode_info::m115)) {
        if (cmd.pop_front() == 'U') {
            // Terminate string if not already
            // cmd.end is a pointer to 1 character past the end of the string in the zero-terminated pre-allocated buffer, so this is safe
            if (!*cmd.end) {
                *cmd.end = '\0';
            }

            if (!is_up_to_date(cmd.c_str())) {
                valid_printer_settings.outdated_firmware.fail();
                strlcpy(valid_printer_settings.latest_fw_version, cmd.c_str(), min(sizeof(valid_printer_settings.latest_fw_version), cmd.len()));
                // Cut the string at the comment start
                char *comment_start = strchr(valid_printer_settings.latest_fw_version, ';');
                if (comment_start) {
                    *comment_start = '\0';
                }
            }
        }
    }

    else if (cmd.skip_gcode(gcode_info::m555)) {
        parse_m555(cmd);
    }

    else if ((cmd.skip_gcode(gcode_info::m140_set_bed_temp) || cmd.skip_gcode(gcode_info::m190_wait_bed_temp)) && cmd.skip_to_param('S')) {
        bed_preheat_temp = cmd.get_uint();
    }

    else if ((cmd.skip_gcode(gcode_info::m104_set_hotend_temp) || cmd.skip_gcode(gcode_info::m109_wait_hotend_temp)) && cmd.skip_to_param('S')) {
        // Consider the maximum found value found in the gcode (search_first_x_gcodes)
        // This is because there can be lower preheating for ABL
        hotend_preheat_temp = std::max<uint16_t>(cmd.get_uint(), hotend_preheat_temp.value_or(0));
    }
}

void GCodeInfo::parse_comment(GcodeBuffer::String comment) {
    auto [name, val] = comment.parse_metadata();
    if (name.begin == nullptr || val.begin == nullptr) {
        // not a metadata
        return;
    }

    if (name == gcode_info::time) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
        snprintf(printing_time.begin(), printing_time.size(), "%s", val.c_str());
#pragma GCC diagnostic pop

    } else {
        const bool is_filament_type = (name == gcode_info::filament_type);
        const bool is_filament_used_mm = (name == gcode_info::filament_mm);
        const bool is_filament_used_g = (name == gcode_info::filament_g);
        const bool is_extruder_colour = (name == gcode_info::extruder_colour);

        if (is_filament_type || is_filament_used_g || is_filament_used_mm || is_extruder_colour) {
            std::span<char> value(val.c_str(), val.len());
            size_t extruder = 0;
            while (std::optional<std::span<char>> item = iterate_items(value, is_filament_type || is_extruder_colour ? ';' : ',')) {
                if (!item.has_value()) {
                    break;
                } else if (extruder >= per_extruder_info.size()) {
                    continue;
                } else if (is_filament_type) {
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

                } else if (is_extruder_colour) {
                    uint32_t red;
                    uint32_t green;
                    uint32_t blue;
                    // uint8_t doesn't work properly, so tmps are uint32_t
                    if (sscanf(item->data(), "#%02lX%02lX%02lX", &red, &green, &blue) == 3) {
                        per_extruder_info[extruder].extruder_colour = {
                            .red = static_cast<uint8_t>(red),
                            .green = static_cast<uint8_t>(green),
                            .blue = static_cast<uint8_t>(blue),
                        };
                    }
                }
                extruder++;
            }
        }
#if EXTRUDERS > 1
        else if (name == gcode_info::filament_wipe_tower_g) {
            // load amount of material used filament for wipe tower
            float temp;
            sscanf(val.c_str(), "%f", &temp);
            filament_wipe_tower_g = temp;
        }
#endif
    }
}

void GCodeInfo::PreviewInit(IGcodeReader &reader) {
    valid_printer_settings = ValidPrinterSettings(); // reset to valid state
    per_extruder_info = {}; // Reset extruder info
#if EXTRUDERS > 1
    filament_wipe_tower_g = std::nullopt;
#endif

    GcodeBuffer buffer;

    // parse metadata
    if (reader.stream_metadata_start()) {
        while (true) {
            auto res = reader.stream_get_line(buffer);

            // valid_for_print should is supposed to make sure that file is downloaded-enough to not run out of bounds here.
            assert(res != IGcodeReader::Result_t::RESULT_OUT_OF_RANGE);
            if (res != IGcodeReader::Result_t::RESULT_OK) {
                break;
            }

            parse_comment(buffer.line);
        }

    } else {
        log_error(Buddy, "Metadata in gcode not found");
    }

    // parse first few gcodes
    if (reader.stream_gcode_start()) {
        uint32_t gcode_counter = 0;
        while (true) {
            // valid_for_print should is supposed to make sure that file is downloaded-enough to not run out of bounds here.
            auto res = reader.stream_get_line(buffer);
            assert(res != IGcodeReader::Result_t::RESULT_OUT_OF_RANGE);
            if (res != IGcodeReader::Result_t::RESULT_OK || gcode_counter >= search_first_x_gcodes) {
                break;
            }

            parse_gcode(buffer.line, gcode_counter);
        }
    }
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

bool GCodeInfo::is_singletool_gcode() const {
    // Tool 0 needs to be given in comments and used
    if (!per_extruder_info[0].used()) {
        return false;
    }

    // Other tools need to not be given in comments at all
    for (uint8_t e = 1; e < std::size(per_extruder_info); e++) {
        if (per_extruder_info[e].given()) {
            return false;
        }
    }

    return true;
}

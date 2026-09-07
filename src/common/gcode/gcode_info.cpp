#include "gcode_info.hpp"
#include "option/has_gui.h"
#if HAS_GUI()
    #include <common/thumbnail_sizes.hpp>
#endif
#include <algorithm>
#include <cstring>
#include <option/developer_mode.h>
#include <version.h>
#include <tools_mapping.hpp>
#include "mutable_path.hpp"
#include <logging/log.hpp>
#include <option/has_indx.h>
#include <option/has_mmu2.h>
#include <version/version.hpp>
#include "common/printer_model.hpp"
#include <gcode_reader_plaintext.hpp>

LOG_COMPONENT_REF(Buddy);

#if HAS_MMU2()
    #include "Marlin/src/feature/prusa/MMU2/mmu2_mk4.h"
#endif

#include <option/has_spool_join.h>
#if HAS_SPOOL_JOIN()
    #include <module/prusa/spool_join.hpp>
#endif

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif

#include <config_store/store_instance.hpp>
#include <bsod/bsod.h>

GCodeInfo &GCodeInfo::getInstance() {
    static GCodeInfo instance;
    return instance;
}

const char *GCodeInfo::GetGcodeFilename() {
    return gcode_file_name.data();
}

const char *GCodeInfo::GetGcodeFilepath() {
    return gcode_file_path.data();
}

void GCodeInfo::set_gcode_file(const char *filepath_sfn, const char *filename_lfn) {
    strlcpy(gcode_file_path.data(), filepath_sfn, gcode_file_path.size());
    strlcpy(gcode_file_name.data(), filename_lfn, gcode_file_name.size());
}

GCodeInfo::GCodeInfo() {
}

bool GCodeInfo::check_still_valid() {
    if (!transfers::is_valid_file_or_transfer(GetGcodeFilepath())) {
        error_str_ = N_("File removed or transfer aborted");
        is_printable_ = false;
        return false;
    }

    return !has_error();
}

bool GCodeInfo::check_valid_for_print(IGcodeReader &reader) {
    reader.update_validity(GetGcodeFilepath());
    // Simpler check, that does not do all the things needed
    // to fully verify encrypted gcodes, that one is done in
    // the prefetch only
    is_printable_ = reader.valid_for_print(false);

    if (reader.has_error()) {
        error_str_ = reader.error_str();
    }
#if HAS_E2EE_SUPPORT()
    if (reader.has_identity_info()) {
        set_identity_info(reader.get_identity_info());
    }
#endif

    return is_printable_;
}

void GCodeInfo::load(IGcodeReader &reader) {
    // Note: We are still checking integrity while printing, i.e. when media
    //       prefetch calls stream_gcode_start(). Ignoring CRC check here in
    //       print preview screen saves around 800ms which is quite noticable.
    const bool ignore_crc = true;

    // Perform pre-indexing, if applicable for this type. Seeking will be faster.
    IGcodeReader::Index index;
    static_assert(index.thumbnails.size() >= 3);
    index.thumbnails[0] = { thumbnail_sizes::preview_thumbnail_width, thumbnail_sizes::preview_thumbnail_height, IGcodeReader::ImgType::QOI };
    index.thumbnails[1] = { thumbnail_sizes::progress_thumbnail_width, thumbnail_sizes::progress_thumbnail_height, IGcodeReader::ImgType::QOI };
    index.thumbnails[2] = { thumbnail_sizes::old_progress_thumbnail_width, thumbnail_sizes::progress_thumbnail_height, IGcodeReader::ImgType::QOI };
    reader.generate_index(index, ignore_crc);

#if HAS_GUI()
    info_.has_preview_thumbnail_ = IGcodeReader::Index::present(index.thumbnails[0].position);
    info_.has_progress_thumbnail_ = IGcodeReader::Index::present(index.thumbnails[1].position) || IGcodeReader::Index::present(index.thumbnails[2].position);
#endif

    // If we didn't get any thumbnails in the index, it means they are mixed into the metadata.
    // This is a workaround way how to check that the reader is PlainGCodeReader (we don't have RTTI)
    const bool plaintext_gcodes = std::find_if(index.thumbnails.begin(), index.thumbnails.end(), [](const auto &t) { return t.position == IGcodeReader::Index::not_indexed; }) != index.thumbnails.end();

    GcodeBuffer buffer;

    // parse metadata
    if (reader.stream_metadata_start(&index)) {
        while (true) {
            auto res = reader.stream_get_line(buffer, IGcodeReader::Continuations::Discard);

            // valid_for_print should is supposed to make sure that file is downloaded-enough to not run out of bounds here.
            debug_assert(res != IGcodeReader::Result_t::RESULT_OUT_OF_RANGE);
            if (res != IGcodeReader::Result_t::RESULT_OK) {
                break;
            }

            parse_comment(buffer.line, plaintext_gcodes);
        }

    } else {
        log_warning(Buddy, "Metadata in gcode not found");
    }

    // parse first few gcodes
    const uint32_t offset = 0;
    if (reader.stream_gcode_start(offset, ignore_crc, &index) == IGcodeReader::Result_t::RESULT_OK) {
        uint32_t gcode_counter = 0;
        while (true) {
            // valid_for_print should is supposed to make sure that file is downloaded-enough to not run out of bounds here.
            auto res = reader.stream_get_line(buffer, IGcodeReader::Continuations::Discard);
            debug_assert(res != IGcodeReader::Result_t::RESULT_OUT_OF_RANGE);
            if (res != IGcodeReader::Result_t::RESULT_OK || gcode_counter >= search_first_x_gcodes) {
                break;
            }

            parse_gcode(buffer.line, gcode_counter);
        }
    }

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
    error_str_ = {};
#if HAS_E2EE_SUPPORT()
    identity_info = std::nullopt;
#endif

    info_ = {};
    per_extruder_info.fill({});
}

bool GCodeInfo::is_up_to_date(const char *new_version_string) {
    // Parse version from G-code
    // supported formats: MAJOR.MINOR.PATCH, MAJOR.MINOR.PATCH+BUILD_NUMBER, MAJOR.MINOR.PATCH-PRERELEASE+BUILD_NUMBER
    // only MAJOR, MINOR, PATH, BUILD_NUMBER are used for version comparison,
    struct {
        unsigned major = 0;
        unsigned minor = 0;
        unsigned patch = 0;
        unsigned build_number = 0;
    } parsed;

    if (sscanf(new_version_string, "%u.%u.%u", &parsed.major, &parsed.minor, &parsed.patch) != 3) {
        return true;
    }
    if (auto *plus = strchr(new_version_string, '+'); !plus || sscanf(plus, "%u", &parsed.build_number) != 1) {
        parsed.build_number = 0;
    }

    if (parsed.major > version::project_version_major) { // Major is higher
        return false;
    }

    if (parsed.major == version::project_version_major && parsed.minor > version::project_version_minor) { // Minor is higher
        return false;
    }

    if (parsed.major == version::project_version_major && parsed.minor == version::project_version_minor && parsed.patch > version::project_version_patch) { // Patch is higher
        return false;
    }

    if (parsed.major == version::project_version_major && parsed.minor == version::project_version_minor && parsed.patch == version::project_version_patch && parsed.build_number && parsed.build_number > unsigned(version::project_build_number)) { // Suffix is higher
        return false;
    }

    // Ignore everything behind suffix number
    return true;
}

void GCodeInfo::parse_m555(GcodeBuffer::String cmd) {
    auto &bed_preheat_area = info_.bed_preheat_area;

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
    using Check = buddy::gcode_compatibility::GeneralCheck;

    {
        // format is M862.x, so remove dot
        char dot = cmd.pop_front();
        if (dot != '.') {
            return;
        }
    }

    char subcode = cmd.pop_front();
    cmd.skip_ws();

    const auto check_compatibility = [&](const PrinterModelInfo *gcode_printer) {
        // Unknown gcode printer, sayonara!
        if (!gcode_printer) {
            info_.failed_gcode_checks.set(Check::printer_model);
            return;
        }
        const PrinterGCodeCompatibilityReport compatibility = PrinterModelInfo::current().gcode_compatibility_report(*gcode_printer);

        // If there isn't full compatibility of the gcode, report wrong printer model
        if (compatibility != PrinterGCodeCompatibilityReport { .is_compatible = true }) {
#if HAS_GCODE_COMPATIBILITY()
            if (compatibility.is_compatible) {
                info_.failed_gcode_checks.set(Check::gcode_compatibility_mode);
            } else
#endif
            {
                info_.failed_gcode_checks.set(Check::printer_model);
            }
        }
    };

    // Parse parameters
    [[maybe_unused]] uint8_t tool = 0; // Default is first tool
    std::optional<float> p_diameter;
    std::optional<bool> requires_hardened_nozzle, requires_high_flow_nozzle;
    while (!cmd.is_empty()) {
        const char letter = cmd.pop_front();
        if (letter == 'T') {
            tool = cmd.get_uint(); // Check particular tool (only for M862.1)
        } else if (letter == 'P') {
            switch (subcode) {

            case '1':
                if (auto val = cmd.get_float(); std::isfinite(val)) {
                    p_diameter = val; // Only store value in case Tx comes later
                }
                break;

            case '3': {
                const auto gcode_printer_str = cmd.get_string();
                check_compatibility(PrinterModelInfo::from_id_str(std::string_view(gcode_printer_str.begin, gcode_printer_str.end)));
                break;
            }

            case '4':
                // Parse M862.4 for minimal required firmware version
                if (!is_up_to_date(cmd.c_str())) {
                    info_.failed_gcode_checks.set(Check::minimum_fw_version);
                }
                break;

            case '2':
                check_compatibility(PrinterModelInfo::from_gcode_check_code(cmd.get_uint()));
                break;

            case '5':
                if (cmd.get_uint() > gcode_level) {
                    info_.failed_gcode_checks.set(Check::gcode_level);
                }
                break;

            case '6': {
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
                auto feature = cmd.get_string();
                feature.trim();

#if HAS_MMU2()
                if (compare(feature, "MMU3")) {
                    if (!MMU2::mmu2.Enabled()) {
                        info_.failed_gcode_checks.set(Check::mmu);
                    }
                    break;
                }
#endif
                if (compare(feature, "Input Shaper")) {
                    info_.sliced_with_input_shaper_ = true;
                    break;
                }
#if HAS_INDX()
                if (compare(feature, "INDX lock")) {
                    info_.sliced_with_indx_lock_ = true;
                    break;
                }
#endif

                log_error(Buddy, "Unsupported feature: %s", feature.c_str());
                info_.failed_gcode_checks.set(Check::unsupported_features);
                break;
            }
            }
        } else if (letter == 'A') {
            switch (subcode) {

            case '1':
                requires_hardened_nozzle = cmd.get_uint();
                break;

            default:
                break;
            }
        } else if (letter == 'F') {
            switch (subcode) {

            case '1':
                requires_high_flow_nozzle = cmd.get_uint();
                break;

            default:
                break;
            }
        }
        cmd.skip_nws();
        cmd.skip_ws();
    }

    const auto visit_tool = [&](const auto &visitor) {
#if HAS_MMU2()
        // MMU-equipped printers have only one nozzle diameter for all tools/slots
        // Makes the pre-print screen hide the nozzle sizes, which is both good and bad at the same time
        // -> "?.??" is gone, but no actual diameter is shown anymore - that can be tweaked further on the visualization side.
        // Here, we must set the correct nozzle diameter for all tools if specified.
        for (int8_t e = 0; e < EXTRUDERS; e++) {
            visitor(per_extruder_info[e]);
        }
#else
        if (tool < per_extruder_info.size()) {
            visitor(per_extruder_info[tool]);
        }
#endif
    };

    if (p_diameter.has_value()) {
        visit_tool([&](auto &info) { info.nozzle_diameter = *p_diameter; });
    }
    if (requires_hardened_nozzle.has_value()) {
        visit_tool([&](auto &info) { info.requires_hardened_nozzle = *requires_hardened_nozzle; });
    }
    if (requires_high_flow_nozzle.has_value()) {
        visit_tool([&](auto &info) { info.requires_high_flow_nozzle = *requires_high_flow_nozzle; });
    }
}

void GCodeInfo::parse_gcode(GcodeBuffer::String cmd, uint32_t &gcode_counter) {
    cmd.skip_ws();
    if (cmd.is_empty() || cmd.front() == ';') {
        return;
    }
    gcode_counter++;

    // skip line number if present
    if (cmd.front() == 'N') {
        cmd.skip(static_cast<size_t>(2));
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
                info_.new_firmware_available = true;
                strlcpy(info_.latest_fw_version.data(), cmd.c_str(), std::min(info_.latest_fw_version.capacity(), cmd.len() + 1 /* +1 for the null terminator */));

                // Cut the string at the comment start
                char *comment_start = strchr(info_.latest_fw_version.data(), ';');
                if (comment_start) {
                    *comment_start = '\0';
                }
            }
        }
    }

    else if (cmd.skip_gcode(gcode_info::m486)) {
        // Do not count M486 towards search_first_x_gcodes limit
        // M486 is emiited multiple times for each object in the gcode
        // Meaning that if the gcode has enough objects, we would drain the limit
        // without ever getting to the checks
        // BFW-7269
        gcode_counter--;
    }

    else if (cmd.skip_gcode(gcode_info::m555)) {
        parse_m555(cmd);
    }

    else if ((cmd.skip_gcode(gcode_info::m140_set_bed_temp) || cmd.skip_gcode(gcode_info::m190_wait_bed_temp)) && cmd.skip_to_param('S')) {
        info_.bed_preheat_temp = cmd.get_uint();
    }

    else if ((cmd.skip_gcode(gcode_info::m104_set_hotend_temp) || cmd.skip_gcode(gcode_info::m109_wait_hotend_temp)) && cmd.skip_to_param('S')) {
        // Consider the maximum found value found in the gcode (search_first_x_gcodes)
        // This is because there can be lower preheating for ABL
        info_.hotend_preheat_temp = std::max<uint16_t>(cmd.get_uint(), info_.hotend_preheat_temp.value_or(0));
    }
}

void GCodeInfo::parse_comment(GcodeBuffer::String comment, bool plaintext_gcodes) {
    comment.skip_ws();

#if HAS_GUI()
    if (plaintext_gcodes) {
        if (const auto details = PlainGcodeReader::thumbnail_details(comment); details != std::nullopt) {
            auto check = [&](uint16_t w_exp, uint16_t h_exp, bool &has_thumbnail) {
                if (details->width == w_exp && details->height == h_exp && details->type == IGcodeReader::ImgType::QOI) {
                    has_thumbnail = true;
                }
            };
            check(thumbnail_sizes::preview_thumbnail_width, thumbnail_sizes::preview_thumbnail_height, info_.has_preview_thumbnail_);
            check(thumbnail_sizes::progress_thumbnail_width, thumbnail_sizes::progress_thumbnail_height, info_.has_progress_thumbnail_);
            check(thumbnail_sizes::old_progress_thumbnail_width, thumbnail_sizes::progress_thumbnail_height, info_.has_progress_thumbnail_);

            return;
        }
    }
#else
    // #error dead code found by automatic analyses (see BFW-5461)
    (void)plaintext_gcodes;
#endif

    auto [name, val] = comment.parse_metadata();
    if (name.begin == nullptr || val.begin == nullptr) {
        // not a metadatum
        return;
    }

    if (name == gcode_info::time) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
        snprintf(info_.printing_time.begin(), info_.printing_time.size(), "%s", val.c_str());
#pragma GCC diagnostic pop

    } else {
        const bool is_filament_type = (name == gcode_info::filament_type);
        const bool is_filament_used_mm = (name == gcode_info::filament_mm);
        const bool is_filament_used_g = (name == gcode_info::filament_g);
        const bool is_extruder_colour = (name == gcode_info::extruder_colour);

        if (is_filament_type || is_filament_used_g || is_filament_used_mm || is_extruder_colour) {
            std::span<char> value(val.c_str(), val.len());
            size_t extruder = 0;
            while (const auto item = iterate_items(value, is_filament_type || is_extruder_colour ? ';' : ',')) {
                if (extruder >= per_extruder_info.size()) {
                    continue;

                } else if (is_filament_type) {
                    FilamentTypeParameters::Name filament_name;
                    snprintf(filament_name.data(), filament_name.capacity(), "%.*s", static_cast<int>(item->size()), item->data());
                    per_extruder_info[extruder].filament_name = filament_name;
                    info_.filament_described = true;

                } else if (is_filament_used_mm) {
                    float val;
                    sscanf(item->data(), "%f", &val);
                    if (std::isfinite(val)) {
                        per_extruder_info[extruder].filament_used_mm = val;
                    }

                } else if (is_filament_used_g) {
                    float val;
                    sscanf(item->data(), "%f", &val);
                    if (std::isfinite(val)) {
                        per_extruder_info[extruder].filament_used_g = val;
                    }

                } else if (is_extruder_colour) {
                    per_extruder_info[extruder].extruder_colour = Color::from_string(*item);
                }
                extruder++;
            }
        }
#if EXTRUDERS > 1
        else if (name == gcode_info::filament_wipe_tower_g) {
            // load amount of material used filament for wipe tower
            float temp;
            sscanf(val.c_str(), "%f", &temp);
            info_.filament_wipe_tower_g = temp;
        } else if (name == gcode_info::total_toolchanges) {
            // Number of toolchanges over the whole print; used to estimate INDX nozzle-cleaner wastebin fill.
            uint16_t tmp = 0;
            if (sscanf(val.c_str(), "%hu", &tmp) == 1) {
                info_.total_toolchanges = tmp;
            }
        }
#endif
    }
}

std::optional<std::string_view> GCodeInfo::iterate_items(std::span<char> &buffer, char separator) {
    // skip leading spaces
    while (buffer.size() > 0 && buffer[0] && isspace(*buffer.data())) {
        buffer = buffer.subspan(1);
    }

    // find end of the item
    size_t item_length = 0;
    for (; item_length < buffer.size(); item_length++) {
        if (buffer[item_length] == 0 || buffer[item_length] == separator) {
            break;
        }
    }
    std::span<char> next_buffer = buffer.subspan((item_length < buffer.size() && buffer[item_length] == separator) ? item_length + 1 : item_length);

    // strip trailing whitespaces
    while (item_length && isspace(buffer[item_length - 1])) {
        item_length--;
    }

    const auto result = item_length ? std::make_optional(std::string_view(buffer.begin(), buffer.begin() + item_length)) : std::nullopt;
    buffer = next_buffer;
    return result;
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

void GCodeInfo::for_each_used_extruder(const stdext::inplace_function<void(GcodeToolIndex, VirtualToolIndex, const ExtruderInfo &info)> &callback) {
    for (auto gcode_tool : GcodeToolIndex::all()) {
        const auto &info = get_extruder_info(gcode_tool);
        if (!info.used()) {
            continue;
        }

        const uint8_t tool_index = tools_mapping::to_physical_tool(gcode_tool.to_raw());
        if (tool_index == tools_mapping::no_tool) {
            continue;
        }

        callback(gcode_tool, VirtualToolIndex::from_raw(tool_index), info);
    }
}

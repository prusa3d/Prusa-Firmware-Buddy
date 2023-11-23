#include "convert.hpp"
#include "binarize/binarize.hpp"

#include <boost/beast/core/detail/base64.hpp>

#include <optional>
#include <functional>
#include <charconv>
#include <memory>

namespace bgcode {
using namespace core;
using namespace binarize;
namespace convert {

class GCodeReader
{
public:
    struct GCodeLine
    {
        std::string raw;
        void reset() { raw.clear(); }
    };

    GCodeReader(FILE& file) : m_file(file) {}

    typedef std::function<void(GCodeReader&, const GCodeLine&)> ParseLineCallback;
    typedef std::function<void(const char*, const char*)> InternalParseLineCallback;

    // Returns false if reading the file failed.
    bool parse(ParseLineCallback callback) {
        GCodeLine gline;
        m_parsing = true;
        return parse_internal([this, &gline, callback](const char* begin, const char* end) {
              gline.reset();
              parse_line(begin, end, gline, callback);
        });
    }

    void quit_parsing() { m_parsing = false; }

private:
    FILE& m_file;
    bool m_parsing{ false };

    bool parse_internal(InternalParseLineCallback parse_line_callback) {
        // Read the input stream 64kB at a time, extract lines and process them.
        std::vector<char> buffer(65536 * 10, 0);
        // Line buffer.
        std::string gcode_line;
        size_t file_pos = 0;
        for (;;) {
            size_t cnt_read = ::fread(buffer.data(), 1, buffer.size(), &m_file);
            if (::ferror(&m_file)) {
                m_parsing = false;
                return false;
            }
            bool eof = cnt_read == 0;
            auto it = buffer.begin();
            auto it_bufend = buffer.begin() + cnt_read;
            while (it != it_bufend || (eof && !gcode_line.empty())) {
                // Find end of line.
                bool eol = false;
                auto it_end = it;
                for (; it_end != it_bufend && !(eol = *it_end == '\r' || *it_end == '\n'); ++it_end)
                    ; // silence -Wempty-body
                // End of line is indicated also if end of file was reached.
                eol |= eof && it_end == it_bufend;
                if (eol) {
                    if (gcode_line.empty())
                        parse_line_callback(&(*it), &(*it_end));
                    else {
                        gcode_line.insert(gcode_line.end(), it, it_end);
                        parse_line_callback(gcode_line.c_str(), gcode_line.c_str() + gcode_line.size());
                        gcode_line.clear();
                    }
                    if (!m_parsing)
                        // The callback wishes to exit.
                        return true;
                }
                else
                    gcode_line.insert(gcode_line.end(), it, it_end);
                // Skip EOL.
                it = it_end;
                if (it != it_bufend && *it == '\r')
                    ++it;
                if (it != it_bufend && *it == '\n')
                    ++it;
            }
            if (eof)
                break;
            file_pos += cnt_read;
        }
        m_parsing = false;
        return true;
    }

    const char* parse_line(const char* ptr, const char* end, GCodeLine& gline, ParseLineCallback callback) {
        std::pair<const char*, const char*> cmd;
        const char* line_end = parse_line_internal(ptr, end, gline, cmd);
        callback(*this, gline);
        return line_end;
    }

    const char* parse_line_internal(const char* ptr, const char* end, GCodeLine& gline, std::pair<const char*, const char*>& command) {
        // command and args
        const char* c = ptr;
        {
            // Skip the whitespaces.
            command.first = skip_whitespaces(c);
            // Skip the command.
            c = command.second = skip_word(command.first);
            // Up to the end of line or comment.
            while (!is_end_of_gcode_line(*c)) {
                // Skip whitespaces.
                c = skip_whitespaces(c);
                if (is_end_of_gcode_line(*c))
                    break;
                // Skip the rest of the word.
                c = skip_word(c);
            }
        }

        // Skip the rest of the line.
        for (; !is_end_of_line(*c); ++c);

        // Copy the raw string including the comment, without the trailing newlines.
        if (c > ptr)
            gline.raw.assign(ptr, c);

        // Skip the trailing newlines.
        if (*c == '\r')
            ++c;
        if (*c == '\n')
            ++c;

        return c;
    }

    static bool        is_whitespace(char c) { return c == ' ' || c == '\t'; }
    static bool        is_end_of_line(char c) { return c == '\r' || c == '\n' || c == 0; }
    static bool        is_end_of_gcode_line(char c) { return c == ';' || is_end_of_line(c); }
    static bool        is_end_of_word(char c) { return is_whitespace(c) || is_end_of_gcode_line(c); }
    static const char* skip_whitespaces(const char* c) {
        for (; is_whitespace(*c); ++c)
            ; // silence -Wempty-body
        return c;
    }
    static const char* skip_word(const char* c) {
        for (; !is_end_of_word(*c); ++c)
            ; // silence -Wempty-body
        return c;
    }
};

static std::string_view trim(const std::string_view& str)
{
    if (str.empty())
        return std::string_view();
    size_t start = 0;
    while (start < str.size() - 1 && (str[start] == ' ' || str[start] == '\t')) { ++start; }
    size_t end = str.size() - 1;
    while (end > 0 && (str[end] == ' ' || str[end] == '\t')) { --end; }
    if ((start == end && (str[end] == ' ' || str[end] == '\t')) || (start > end))
        return std::string_view();
    else
        return std::string_view(&str[start], end - start + 1);
}

static std::string_view uncomment(const std::string_view& str)
{
    return (!str.empty() && str[0] == ';') ? trim(str.substr(1)) : str;
}

template<typename Integer>
static void to_int(const std::string_view& str, Integer& out) {
    const std::from_chars_result result = std::from_chars(str.data(), str.data() + str.size(), out);
    if (result.ec == std::errc::invalid_argument || result.ec == std::errc::result_out_of_range)
        out = 0;
}

BGCODE_CONVERT_EXPORT EResult from_ascii_to_binary(FILE& src_file, FILE& dst_file, const BinarizerConfig& config)
{
    using namespace std::literals;
    static constexpr const std::string_view GeneratedByPrusaSlicer = "generated by PrusaSlicer"sv;

    static constexpr const std::string_view PrinterModel = "printer_model"sv;
    static constexpr const std::string_view FilamentType = "filament_type"sv;
    static constexpr const std::string_view NozzleDiameter = "nozzle_diameter"sv;
    static constexpr const std::string_view BedTemperature = "bed_temperature"sv;
    static constexpr const std::string_view BrimWidth = "brim_width"sv;
    static constexpr const std::string_view FillDensity = "fill_density"sv;
    static constexpr const std::string_view LayerHeight = "layer_height"sv;
    static constexpr const std::string_view Temperature = "temperature"sv;
    static constexpr const std::string_view Ironing = "ironing"sv;
    static constexpr const std::string_view SupportMaterial = "support_material"sv;
    static constexpr const std::string_view MaxLayerZ = "max_layer_z"sv;
    static constexpr const std::string_view ExtruderColour = "extruder_colour"sv;
    static constexpr const std::string_view FilamentUsedMm = "filament used [mm]"sv;
    static constexpr const std::string_view FilamentUsedG = "filament used [g]"sv;
    static constexpr const std::string_view EstimatedPrintingTimeNormal = "estimated printing time (normal mode)"sv;
    static constexpr const std::string_view FilamentUsedCm3 = "filament used [cm3]"sv;
    static constexpr const std::string_view FilamentCost = "filament cost"sv;
    static constexpr const std::string_view TotalFilamentUsedG = "total filament used [g]"sv;
    static constexpr const std::string_view TotalFilamentCost = "total filament cost"sv;
    static constexpr const std::string_view EstimatedPrintingTimeSilent = "estimated printing time (silent mode)"sv;
    static constexpr const std::string_view Estimated1stLayerPrintingTimeNormal = "estimated first layer printing time (normal mode)"sv;
    static constexpr const std::string_view Estimated1stLayerPrintingTimeSilent = "estimated first layer printing time (silent mode)"sv;

    static constexpr const std::string_view ThumbnailPNGBegin = "thumbnail begin"sv;
    static constexpr const std::string_view ThumbnailPNGEnd = "thumbnail end"sv;
    static constexpr const std::string_view ThumbnailJPGBegin = "thumbnail_JPG begin"sv;
    static constexpr const std::string_view ThumbnailJPGEnd = "thumbnail_JPG end"sv;
    static constexpr const std::string_view ThumbnailQOIBegin = "thumbnail_QOI begin"sv;
    static constexpr const std::string_view ThumbnailQOIEnd = "thumbnail_QOI end"sv;

    static constexpr const std::string_view PrusaSlicerConfig = "prusaslicer_config"sv;

    auto search_metadata_value = [&](const std::string_view& str, const std::string_view& key) {
        std::string ret;
        if (str.find(key) == 0) {
            const size_t pos = str.find("=");
            if (pos != std::string_view::npos)
                ret = trim(str.substr(pos + 1));
        }
        return ret;
    };
    auto extract_thumbnail_rect = [](const std::string_view& str) {
        std::pair<uint16_t, uint16_t> ret = { 0, 0 };
        const size_t pos = str.find('x');
        if (pos != std::string_view::npos) {
            to_int(str.substr(0, pos), ret.first);
            to_int(str.substr(pos + 1), ret.second);
        }
      return ret;
    };

    EResult res = is_valid_binary_gcode(src_file);
    if (res == EResult::Success)
        return EResult::AlreadyBinarized;

    Binarizer binarizer;
    binarizer.set_enabled(true);
    BinaryData& binary_data = binarizer.get_binary_data();

    std::string printer_model;
    std::string filament_type;
    std::string nozzle_diameter;
    std::string bed_temperature;
    std::string brim_width;
    std::string fill_density;
    std::string layer_height;
    std::string temperature;
    std::string ironing;
    std::string support_material;
    std::string max_layer_z;
    std::string extruder_colour;
    std::string filament_used_mm;
    std::string filament_used_g;
    std::string estimated_printing_time_normal;
    std::string filament_used_cm3;
    std::string filament_cost;
    std::string total_filament_used_g;
    std::string total_filament_cost;
    std::string estimated_printing_time_silent;
    std::string estimated_1st_layer_printing_time_normal;
    std::string estimated_1st_layer_printing_time_silent;

    std::optional<EThumbnailFormat> reading_thumbnail;
    size_t curr_thumbnail_data_size = 0;
    size_t curr_thumbnail_data_loaded = 0;

    bool producer_found = false;
    bool reading_config = false;

    std::vector<size_t> processed_lines;

    EResult parse_res = EResult::Success;
    GCodeReader parser(src_file);
    size_t lines_counter = 0;
    if (!parser.parse([&](GCodeReader& r, const GCodeReader::GCodeLine& line) {
        if (parse_res != EResult::Success)
            r.quit_parsing();

        const std::string_view sv_line = uncomment(trim(line.raw));
        if (sv_line.empty()) {
            processed_lines.emplace_back(lines_counter++);
            return;
        }

        // update file metadata
        size_t pos = sv_line.find(GeneratedByPrusaSlicer);
        if (pos != std::string_view::npos) {
            std::string_view version = trim(sv_line.substr(pos + GeneratedByPrusaSlicer.length()));
            pos = version.find(" ");
            if (pos != std::string_view::npos)
                version = version.substr(0, pos);
            binary_data.file_metadata.raw_data.emplace_back("Producer", "PrusaSlicer " + std::string(version));
            producer_found = true;
            processed_lines.emplace_back(lines_counter++);
            return;
        }

        // collect print + printer metadata
        // to keep the proper order they will be set into binary_data later
        auto collect_metadata = [&](const std::string_view& key, std::string& value, bool shared_in_config = false) {
            const std::string str = search_metadata_value(sv_line, key);
            if (!str.empty()) {
                if (value.empty())
                    value = str;
                if (!shared_in_config) {
                    processed_lines.emplace_back(lines_counter++);
                    return true;
                }
                if (shared_in_config && !reading_config) {
                    processed_lines.emplace_back(lines_counter++);
                    return true;
                }
            }
            return false;
        };

        if (collect_metadata(PrinterModel, printer_model, true)) return;
        if (collect_metadata(FilamentType, filament_type, true)) return;
        if (collect_metadata(NozzleDiameter, nozzle_diameter, true)) return;
        if (collect_metadata(BedTemperature, bed_temperature, true)) return;
        if (collect_metadata(BrimWidth, brim_width, true)) return;
        if (collect_metadata(FillDensity, fill_density, true)) return;
        if (collect_metadata(LayerHeight, layer_height, true)) return;
        if (collect_metadata(Temperature, temperature, true)) return;
        if (collect_metadata(Ironing, ironing, true)) return;
        if (collect_metadata(SupportMaterial, support_material, true)) return;
        if (collect_metadata(MaxLayerZ, max_layer_z)) return;
        if (collect_metadata(ExtruderColour, extruder_colour, true)) return;
        if (collect_metadata(FilamentUsedMm, filament_used_mm)) return;
        if (collect_metadata(FilamentUsedG, filament_used_g)) return;
        if (collect_metadata(EstimatedPrintingTimeNormal, estimated_printing_time_normal)) return;
        if (collect_metadata(FilamentUsedCm3, filament_used_cm3)) return;
        if (collect_metadata(FilamentCost, filament_cost)) return;
        if (collect_metadata(TotalFilamentUsedG, total_filament_used_g)) return;
        if (collect_metadata(TotalFilamentCost, total_filament_cost)) return;
        if (collect_metadata(EstimatedPrintingTimeSilent, estimated_printing_time_silent)) return;
        if (collect_metadata(Estimated1stLayerPrintingTimeNormal, estimated_1st_layer_printing_time_normal)) return;
        if (collect_metadata(Estimated1stLayerPrintingTimeSilent, estimated_1st_layer_printing_time_silent)) return;

        // update slicer metadata
        if (!reading_config) {
            if (search_metadata_value(sv_line, PrusaSlicerConfig) == "begin") {
                reading_config = true;
                processed_lines.emplace_back(lines_counter++);
                return;
            }
        }
        else {
            if (search_metadata_value(sv_line, PrusaSlicerConfig) == "end") {
                reading_config = false;
                processed_lines.emplace_back(lines_counter++);
                return;
            }
            else {
                const size_t pos = sv_line.find("=");
                if (pos == std::string_view::npos) {
                    parse_res = EResult::InvalidAsciiGCodeFile;
                    return;
                }
                const std::string_view key = trim(sv_line.substr(0, pos));
                const std::string_view value = trim(sv_line.substr(pos + 1));
                if (key.empty()) {
                    parse_res = EResult::InvalidAsciiGCodeFile;
                    return;
                }
                binary_data.slicer_metadata.raw_data.emplace_back(std::string(key), std::string(value));
                processed_lines.emplace_back(lines_counter++);
                return;
            }
        }

        // update thumbnails
        if (!reading_thumbnail.has_value()) {
            std::string_view sv_thumbnail_str;
            if (sv_line.find(ThumbnailPNGBegin) == 0) {
                reading_thumbnail = EThumbnailFormat::PNG;
                sv_thumbnail_str = trim(sv_line.substr(ThumbnailPNGBegin.size()));
            }
            else if (sv_line.find(ThumbnailJPGBegin) == 0) {
                reading_thumbnail = EThumbnailFormat::JPG;
                sv_thumbnail_str = trim(sv_line.substr(ThumbnailJPGBegin.size()));
            }
            else if (sv_line.find(ThumbnailQOIBegin) == 0) {
                reading_thumbnail = EThumbnailFormat::QOI;
                sv_thumbnail_str = trim(sv_line.substr(ThumbnailQOIBegin.size()));
            }
            if (reading_thumbnail.has_value()) {
                ThumbnailBlock& thumbnail = binary_data.thumbnails.emplace_back(ThumbnailBlock());
                thumbnail.params.format = (uint16_t)*reading_thumbnail;
                pos = sv_thumbnail_str.find(" ");
                if (pos == std::string_view::npos) {
                    parse_res = EResult::InvalidAsciiGCodeFile;
                    return;
                }
                const std::string_view sv_rect_str = trim(sv_thumbnail_str.substr(0, pos));
                std::pair<uint16_t, uint16_t> rect = extract_thumbnail_rect(sv_rect_str);
                if (rect.first == 0 || rect.second == 0) {
                    parse_res = EResult::InvalidAsciiGCodeFile;
                    return;
                }
                thumbnail.params.width = rect.first;
                thumbnail.params.height = rect.second;
                const std::string_view sv_data_size_str = trim(sv_thumbnail_str.substr(pos + 1));
                size_t data_size;
                to_int(sv_data_size_str, data_size);
                if (data_size == 0) {
                    parse_res = EResult::InvalidAsciiGCodeFile;
                    return;
                }
                curr_thumbnail_data_size = data_size;
                curr_thumbnail_data_loaded = 0;
                thumbnail.data.resize(data_size);
                processed_lines.emplace_back(lines_counter++);
                return;
            }
        }
        else {
            bool thumbnail_end = false;
            if (sv_line.find(ThumbnailPNGEnd) == 0) {
                if (*reading_thumbnail != EThumbnailFormat::PNG) {
                    parse_res = EResult::InvalidAsciiGCodeFile;
                    return;
                }
                thumbnail_end = true;
            }
            else if (sv_line.find(ThumbnailJPGEnd) == 0) {
                if (*reading_thumbnail != EThumbnailFormat::JPG) {
                    parse_res = EResult::InvalidAsciiGCodeFile;
                    return;
                }
                thumbnail_end = true;
            }
            else if (sv_line.find(ThumbnailQOIEnd) == 0) {
                if (*reading_thumbnail != EThumbnailFormat::QOI) {
                    parse_res = EResult::InvalidAsciiGCodeFile;
                    return;
                }
                thumbnail_end = true;
            }

            if (thumbnail_end) {
                reading_thumbnail.reset();
                if (curr_thumbnail_data_loaded != curr_thumbnail_data_size) {
                    parse_res = EResult::InvalidAsciiGCodeFile;
                    return;
                }
                ThumbnailBlock& thumbnail = binary_data.thumbnails.back();
                if (thumbnail.data.size() > curr_thumbnail_data_loaded)
                    thumbnail.data.resize(curr_thumbnail_data_loaded);
                std::string decoded;
                decoded.resize(boost::beast::detail::base64::decoded_size(thumbnail.data.size()));
                decoded.resize(boost::beast::detail::base64::decode((void*)&decoded[0], (const char*)thumbnail.data.data(), thumbnail.data.size()).first);
                thumbnail.data.clear();
                thumbnail.data.insert(thumbnail.data.end(), decoded.begin(), decoded.end());
                processed_lines.emplace_back(lines_counter++);
                return;
            }
            else {
                if (curr_thumbnail_data_loaded + sv_line.size() > curr_thumbnail_data_size) {
                    parse_res = EResult::InvalidAsciiGCodeFile;
                    return;
                }
                ThumbnailBlock& thumbnail = binary_data.thumbnails.back();
                thumbnail.data.insert(thumbnail.data.begin() + curr_thumbnail_data_loaded, sv_line.begin(), sv_line.end());
                curr_thumbnail_data_loaded += sv_line.size();
                processed_lines.emplace_back(lines_counter++);
                return;
            }
        }

        ++lines_counter;
    }))
        return EResult::ReadError;

    if (parse_res != EResult::Success)
        // propagate error
        return parse_res;

    if (reading_config)
        return EResult::InvalidAsciiGCodeFile;

    if (reading_thumbnail.has_value())
        return EResult::InvalidAsciiGCodeFile;

    if (!producer_found)
        return EResult::InvalidAsciiGCodeFile;

    auto append_metadata = [](std::vector<std::pair<std::string, std::string>>& dst, const std::string& key, const std::string& value) {
        if (!value.empty()) dst.emplace_back(key, value);
    };

    // update printer metadata
    append_metadata(binary_data.printer_metadata.raw_data, std::string(PrinterModel), printer_model);
    append_metadata(binary_data.printer_metadata.raw_data, std::string(FilamentType), filament_type);
    append_metadata(binary_data.printer_metadata.raw_data, std::string(NozzleDiameter), nozzle_diameter);
    append_metadata(binary_data.printer_metadata.raw_data, std::string(BedTemperature), bed_temperature);
    append_metadata(binary_data.printer_metadata.raw_data, std::string(BrimWidth), brim_width);
    append_metadata(binary_data.printer_metadata.raw_data, std::string(FillDensity), fill_density);
    append_metadata(binary_data.printer_metadata.raw_data, std::string(LayerHeight), layer_height);
    append_metadata(binary_data.printer_metadata.raw_data, std::string(Temperature), temperature);
    append_metadata(binary_data.printer_metadata.raw_data, std::string(Ironing), ironing);
    append_metadata(binary_data.printer_metadata.raw_data, std::string(SupportMaterial), support_material);
    append_metadata(binary_data.printer_metadata.raw_data, std::string(MaxLayerZ), max_layer_z);
    append_metadata(binary_data.printer_metadata.raw_data, std::string(ExtruderColour), extruder_colour);
    append_metadata(binary_data.printer_metadata.raw_data, std::string(FilamentUsedMm), filament_used_mm);
    append_metadata(binary_data.printer_metadata.raw_data, std::string(FilamentUsedCm3), filament_used_cm3);
    append_metadata(binary_data.printer_metadata.raw_data, std::string(FilamentUsedG), filament_used_g);
    append_metadata(binary_data.printer_metadata.raw_data, std::string(FilamentCost), filament_cost);
    append_metadata(binary_data.printer_metadata.raw_data, std::string(EstimatedPrintingTimeNormal), estimated_printing_time_normal);

    // update print metadata
    append_metadata(binary_data.print_metadata.raw_data, std::string(FilamentUsedMm), filament_used_mm);
    append_metadata(binary_data.print_metadata.raw_data, std::string(FilamentUsedCm3), filament_used_cm3);
    append_metadata(binary_data.print_metadata.raw_data, std::string(FilamentUsedG), filament_used_g);
    append_metadata(binary_data.print_metadata.raw_data, std::string(FilamentCost), filament_cost);
    append_metadata(binary_data.print_metadata.raw_data, std::string(TotalFilamentUsedG), total_filament_used_g);
    append_metadata(binary_data.print_metadata.raw_data, std::string(TotalFilamentCost), total_filament_cost);
    append_metadata(binary_data.print_metadata.raw_data, std::string(EstimatedPrintingTimeNormal), estimated_printing_time_normal);
    append_metadata(binary_data.print_metadata.raw_data, std::string(EstimatedPrintingTimeSilent), estimated_printing_time_silent);
    append_metadata(binary_data.print_metadata.raw_data, std::string(Estimated1stLayerPrintingTimeNormal), estimated_1st_layer_printing_time_normal);
    append_metadata(binary_data.print_metadata.raw_data, std::string(Estimated1stLayerPrintingTimeSilent), estimated_1st_layer_printing_time_silent);

    res = binarizer.initialize(dst_file, config);
    if (res != EResult::Success)
        // propagate error
        return res;

    // reparse the file to extract the gcode
    rewind(&src_file);
    parse_res = EResult::Success;
    lines_counter = 0;
    if (!parser.parse([&](GCodeReader& r, const GCodeReader::GCodeLine& line) {
        if (parse_res != EResult::Success)
            r.quit_parsing();

        if (std::find(processed_lines.begin(), processed_lines.end(), lines_counter) == processed_lines.end())
            binarizer.append_gcode(line.raw + "\n");

        ++lines_counter;
    }))
        return EResult::ReadError;

    if (parse_res != EResult::Success)
        // propagate error
        return parse_res;

    res = binarizer.finalize();
    if (res != EResult::Success)
        // propagate error
        return res;

    return EResult::Success;
}

BGCODE_CONVERT_EXPORT EResult from_binary_to_ascii(FILE& src_file, FILE& dst_file, bool verify_checksum)
{
    // initialize buffer for checksum calculation, if verify_checksum is true
    std::vector<uint8_t> checksum_buffer;
    if (verify_checksum)
        checksum_buffer.resize(65535);

    auto write_line = [&](const std::string& line) {
        const size_t wsize = fwrite(line.data(), 1, line.length(), &dst_file);
        return !ferror(&dst_file) && wsize == line.length();
    };

    auto write_metadata = [&](const std::vector<std::pair<std::string, std::string>>& data) {
        for (const auto& [key, value] : data) {
            if (!write_line("; " + key + " = " + value + "\n"))
                return false;
        }
        return !ferror(&dst_file);
    };

    EResult res = is_valid_binary_gcode(src_file, true);
    if (res != EResult::Success)
        // propagate error
        return res;

    fseek(&src_file, 0, SEEK_END);
    const long file_size = ftell(&src_file);
    rewind(&src_file);

    //
    // read file header
    //
    FileHeader file_header;
    res = read_header(src_file, file_header, nullptr);
    if (res != EResult::Success)
        // propagate error
        return res;

    //
    // convert file metadata block
    //
    BlockHeader block_header;
    res = read_next_block_header(src_file, file_header, block_header, checksum_buffer.data(), checksum_buffer.size());
    if (res != EResult::Success)
        // propagate error
        return res;
    if ((EBlockType)block_header.type != EBlockType::FileMetadata)
        return EResult::InvalidSequenceOfBlocks;
    FileMetadataBlock file_metadata_block;
    res = file_metadata_block.read_data(src_file, file_header, block_header);
    if (res != EResult::Success)
        // propagate error
        return res;
    auto producer_it = std::find_if(file_metadata_block.raw_data.begin(), file_metadata_block.raw_data.end(),
        [](const std::pair<std::string, std::string>& item) { return item.first == "Producer"; });
    const std::string producer_str = (producer_it != file_metadata_block.raw_data.end()) ? producer_it->second : "Unknown";
    if (!write_line("; generated by " + producer_str + "\n\n\n"))
        return EResult::WriteError;

    //
    // convert printer metadata block
    //
    res = read_next_block_header(src_file, file_header, block_header, checksum_buffer.data(), checksum_buffer.size());
    if (res != EResult::Success)
        // propagate error
        return res;
    if ((EBlockType)block_header.type != EBlockType::PrinterMetadata)
        return EResult::InvalidSequenceOfBlocks;
    PrinterMetadataBlock printer_metadata_block;
    res = printer_metadata_block.read_data(src_file, file_header, block_header);
    if (res != EResult::Success)
        // propagate error
        return res;
    if (!write_metadata(printer_metadata_block.raw_data))
        return EResult::WriteError;

    //
    // convert thumbnail blocks
    //
    long restore_position = ftell(&src_file);
    res = read_next_block_header(src_file, file_header, block_header, checksum_buffer.data(), checksum_buffer.size());
    if (res != EResult::Success)
        // propagate error
        return res;
    while ((EBlockType)block_header.type == EBlockType::Thumbnail) {
        ThumbnailBlock thumbnail_block;
        res = thumbnail_block.read_data(src_file, file_header, block_header);
        if (res != EResult::Success)
            // propagate error
            return res;
        static constexpr const size_t max_row_length = 78;
        std::string encoded;
        encoded.resize(boost::beast::detail::base64::encoded_size(thumbnail_block.data.size()));
        encoded.resize(boost::beast::detail::base64::encode((void*)encoded.data(), (const void*)thumbnail_block.data.data(), thumbnail_block.data.size()));
        std::string format;
        switch ((EThumbnailFormat)thumbnail_block.params.format)
        {
        default:
        case EThumbnailFormat::PNG: { format = "thumbnail"; break; }
        case EThumbnailFormat::JPG: { format = "thumbnail_JPG"; break; }
        case EThumbnailFormat::QOI: { format = "thumbnail_QOI"; break; }
        }
        if (!write_line("\n;\n; " + format + " begin " + std::to_string(thumbnail_block.params.width) + "x" + std::to_string(thumbnail_block.params.height) +
            " " + std::to_string(encoded.length()) + "\n"))
            return EResult::WriteError;
        while (encoded.size() > max_row_length) {
            if (!write_line("; " + encoded.substr(0, max_row_length) + "\n"))
                return EResult::WriteError;
            encoded = encoded.substr(max_row_length);
        }
        if (encoded.size() > 0) {
            if (!write_line("; " + encoded + "\n"))
                return EResult::WriteError;
        }
        if (!write_line("; " + format + " end\n;\n"))
            return EResult::WriteError;

        restore_position = ftell(&src_file);
        res = read_next_block_header(src_file, file_header, block_header, checksum_buffer.data(), checksum_buffer.size());
        if (res != EResult::Success)
            // propagate error
            return res;
    }

    //
    // convert gcode blocks
    //
    auto remove_empty_lines = [](const std::string& data) {
        std::string ret;
        auto begin_it = data.begin();
        auto end_it = data.begin();
        while (end_it != data.end()) {
            while (end_it != data.end() && *end_it != '\n') {
                ++end_it;
            }

          const size_t pos = std::distance(data.begin(), begin_it);
          const size_t line_length = std::distance(begin_it, end_it);
          const std::string_view original_line(&data[pos], line_length);
          const std::string_view reduced_line = uncomment(trim(original_line));
          if (!reduced_line.empty())
              ret += std::string(original_line) + "\n";
          begin_it = ++end_it;
        }

        return ret;
    };

    if (!write_line("\n"))
        return EResult::WriteError;
    res = skip_block(src_file, file_header, block_header);
    if (res != EResult::Success)
        // propagate error
        return res;
    res = read_next_block_header(src_file, file_header, block_header, EBlockType::GCode, checksum_buffer.data(), checksum_buffer.size());
    if (res != EResult::Success)
        // propagate error
        return res;
    while ((EBlockType)block_header.type == EBlockType::GCode) {
        GCodeBlock block;
        res = block.read_data(src_file, file_header, block_header);
        if (res != EResult::Success)
            // propagate error
            return res;
        const std::string out_str = remove_empty_lines(block.raw_data);
        if (!out_str.empty()) {
            if (!write_line(out_str))
                return EResult::WriteError;
        }
        if (ftell(&src_file) == file_size)
            break;
        res = read_next_block_header(src_file, file_header, block_header, checksum_buffer.data(), checksum_buffer.size());
        if (res != EResult::Success)
            // propagate error
            return res;
    }

    //
    // convert print metadata block
    //
    fseek(&src_file, restore_position, SEEK_SET);
    res = read_next_block_header(src_file, file_header, block_header, checksum_buffer.data(), checksum_buffer.size());
    if (res != EResult::Success)
        // propagate error
        return res;
    if ((EBlockType)block_header.type != EBlockType::PrintMetadata)
        return EResult::InvalidSequenceOfBlocks;
    PrintMetadataBlock print_metadata_block;
    res = print_metadata_block.read_data(src_file, file_header, block_header);
    if (res != EResult::Success)
        // propagate error
        return res;
    if (!write_line("\n"))
        return EResult::WriteError;
    if (!write_metadata(print_metadata_block.raw_data))
        return EResult::WriteError;

    //
    // convert slicer metadata block
    //
    res = read_next_block_header(src_file, file_header, block_header, checksum_buffer.data(), checksum_buffer.size());
    if (res != EResult::Success)
        // propagate error
        return res;
    if ((EBlockType)block_header.type != EBlockType::SlicerMetadata)
        return EResult::InvalidSequenceOfBlocks;
    SlicerMetadataBlock slicer_metadata_block;
    res = slicer_metadata_block.read_data(src_file, file_header, block_header);
    if (res != EResult::Success)
        // propagate error
        return res;
    if (!write_line("\n; prusaslicer_config = begin\n"))
        return EResult::WriteError;
    if (!write_metadata(slicer_metadata_block.raw_data))
        return EResult::WriteError;
    if (!write_line("; prusaslicer_config = end\n\n"))
        return EResult::WriteError;

    return EResult::Success;
}

} // namespace core
} // namespace bgcode

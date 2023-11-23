#include "convert/convert.hpp"

#include <string>
#include <string_view>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <stdlib.h>
#include <boost/nowide/cstdio.hpp>

using namespace bgcode::core;
using namespace bgcode::binarize;
using namespace bgcode::convert;

struct Parameter
{
    std::string_view name;
    std::vector<std::string_view> values;
    size_t default_id;
};

using namespace std::literals;
static const std::vector<Parameter> parameters = {
    { "checksum"sv, { "None"sv, "CRC32"sv }, (int)BinarizerConfig().checksum },
    { "file_metadata_compression"sv, { "None"sv, "Deflate"sv, "Heatshrink_11_4"sv, "Heatshrink_12_4"sv }, (int)BinarizerConfig().compression.file_metadata },
    { "print_metadata_compression"sv, { "None"sv, "Deflate"sv, "Heatshrink_11_4"sv, "Heatshrink_12_4"sv }, (int)BinarizerConfig().compression.print_metadata },
    { "printer_metadata_compression"sv, { "None"sv, "Deflate"sv, "Heatshrink_11_4"sv, "Heatshrink_12_4"sv }, (int)BinarizerConfig().compression.printer_metadata },
    { "slicer_metadata_compression"sv, { "None"sv, "Deflate"sv, "Heatshrink_11_4"sv, "Heatshrink_12_4"sv }, (int)BinarizerConfig().compression.slicer_metadata },
    { "gcode_compression"sv, { "None"sv, "Deflate"sv, "Heatshrink_11_4"sv, "Heatshrink_12_4"sv }, (int)BinarizerConfig().compression.gcode },
    { "gcode_encoding"sv, { "None"sv, "MeatPack"sv, "MeatPackComments"sv }, (int)BinarizerConfig().gcode_encoding },
    { "metadata_encoding"sv, { "INI"sv }, (int)BinarizerConfig().metadata_encoding }
};

class ScopedFile
{
public:
    explicit ScopedFile(FILE* file) : m_file(file) {}
    ~ScopedFile() { if (m_file != nullptr) fclose(m_file); }
private:
    FILE* m_file{ nullptr };
};

void show_help() {
    std::cout << "Usage: bgcode filename [ Binarization parameters ]\n";
    std::cout << "\nBinarization parameters (used only when converting to binary format):\n";
    for (const Parameter& p : parameters) {
        std::cout << "--" << p.name << "=X\n";
        std::cout << "  where X is one of:\n";
        size_t count = 0;
        for (const std::string_view& v : p.values) {
            std::cout << "  " << count << ") " << v;
            if (count == p.default_id)
                std::cout << " (default)";
            std::cout << "\n";
            ++count;
        }
    }
}

bool parse_args(int argc, const char* argv[], std::string& src_filename, bool& src_is_binary, BinarizerConfig& config)
{
    if (argc < 2) {
        show_help();
        return false;
    }

    std::vector<std::string_view> arguments;
    arguments.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        arguments.push_back(argv[i]);
    }

    size_t pos = arguments[1].find_last_of(".");
    if (pos == std::string::npos) {
        std::cout << "Invalid filename " << arguments[1] << " (required .gcode extension)\n";
        return false;
    }
    const std::string_view extension = arguments[1].substr(pos);
    if (extension != ".gcode" && extension != ".GCODE") {
        std::cout << "Found invalid extension '" << extension << "' (required .gcode extension)\n";
        return false;
    }
    src_filename = arguments[1];

    FILE* src_file = boost::nowide::fopen(src_filename.c_str(), "rb");
    if (src_file == nullptr) {
        std::cout << "Unable to open file '" << src_filename << "'\n";
        return false;
    }
    src_is_binary = is_valid_binary_gcode(*src_file) == EResult::Success;
    fclose(src_file);

    if (!src_is_binary) {
        for (size_t i = 2; i < arguments.size(); ++i) {
            const std::string_view& a = arguments[i];
            if (a.length() < 2 || a[0] != '-' || a[1] != '-') {
                std::cout << "Found invalid parameter '" << a << "'\n";
                std::cout << "Required syntax: --parameter=value\n";
                return false;
            }

            pos = a.find("=");
            if (pos == std::string_view::npos) {
                std::cout << "Found invalid parameter '" << a << "'\n";
                std::cout << "Required syntax: --parameter=value\n";
                return false;
            }

            const std::string_view key = a.substr(2, pos - 2);
            auto it = std::find_if(parameters.begin(), parameters.end(),
                [&key](const Parameter& item) { return item.name == key; });
            if (it == parameters.end()) {
                std::cout << "Found unknown parameter '" << key << "'\n";
                std::cout << "Accepted parameters:\n";
                for (const Parameter& p : parameters) {
                    std::cout << p.name << "\n";
                }
                return false;
            }

            const Parameter& parameter = parameters[std::distance(parameters.begin(), it)];

            const std::string_view value_str = a.substr(pos + 1);
            int value;
            try {
                value = std::stoi(std::string(value_str));
                if (value >= parameter.values.size())
                    throw std::runtime_error("invalid value");
            }
            catch (...) {
                std::cout << "Found invalid value for parameter '" << parameter.name << "'\n";
                std::cout << "Accepted values:\n";
                for (size_t p = 0; p < parameter.values.size(); ++p) {
                    std::cout << p << ") " << parameter.values[p] << "\n";
                }
                return false;
            }

            if (parameter.name == "checksum")
                config.checksum = (EChecksumType)value;
            else if (parameter.name == "file_metadata_compression")
                config.compression.file_metadata = (ECompressionType)value;
            else if (parameter.name == "print_metadata_compression")
                config.compression.print_metadata = (ECompressionType)value;
            else if (parameter.name == "printer_metadata_compression")
                config.compression.printer_metadata = (ECompressionType)value;
            else if (parameter.name == "slicer_metadata_compression")
                config.compression.slicer_metadata = (ECompressionType)value;
            else if (parameter.name == "gcode_compression")
                config.compression.gcode = (ECompressionType)value;
            else if (parameter.name == "gcode_encoding")
                config.gcode_encoding = (EGCodeEncodingType)value;
            else if (parameter.name == "metadata_encoding")
                config.metadata_encoding = (EMetadataEncodingType)value;
        }
    }
    return true;
}

int main(int argc, const char* argv[])
{
    std::string src_filename;
    bool src_is_binary;
    BinarizerConfig config;
    if (!parse_args(argc, argv, src_filename, src_is_binary, config))
        return EXIT_FAILURE;

    // Open source file
    FILE* src_file = boost::nowide::fopen(src_filename.c_str(), "rb");
    if (src_file == nullptr) {
        std::cout << "Unable to open file '" << src_filename << "'\n";
        return EXIT_FAILURE;
    }
    ScopedFile scoped_src_file(src_file);

    const std::string src_stem = src_filename.substr(0, src_filename.find_last_of("."));
    const std::string dst = src_is_binary ? "_ascii" : "_binary";
    const std::string dst_filename = src_stem + dst + ".gcode";

    // Open destination file
    FILE* dst_file = boost::nowide::fopen(dst_filename.c_str(), "wb");
    if (dst_file == nullptr) {
        std::cout << "Unable to open file '" << dst_filename << "'\n";
        return EXIT_FAILURE;
    }
    ScopedFile scoped_dst_file(dst_file);

    // Perform conversion
    const EResult res = src_is_binary ? from_binary_to_ascii(*src_file, *dst_file, true) : from_ascii_to_binary(*src_file, *dst_file, config);
    if (res == EResult::Success) {
        if (!src_is_binary) {
            std::cout << "Binarization parameters\n";
            for (const Parameter& p : parameters) {
                std::cout << p.name << ": ";
                if (p.name == "checksum")
                    std::cout << p.values[(size_t)config.checksum] << "\n";
                else if (p.name == "file_metadata_compression")
                    std::cout << p.values[(size_t)config.compression.file_metadata] << "\n";
                else if (p.name == "print_metadata_compression")
                    std::cout << p.values[(size_t)config.compression.print_metadata] << "\n";
                else if (p.name == "printer_metadata_compression")
                    std::cout << p.values[(size_t)config.compression.printer_metadata] << "\n";
                else if (p.name == "slicer_metadata_compression")
                    std::cout << p.values[(size_t)config.compression.slicer_metadata] << "\n";
                else if (p.name == "gcode_compression")
                    std::cout << p.values[(size_t)config.compression.gcode] << "\n";
                else if (p.name == "gcode_encoding")
                    std::cout << p.values[(size_t)config.gcode_encoding] << "\n";
                else if (p.name == "metadata_encoding")
                    std::cout << p.values[(size_t)config.metadata_encoding] << "\n";
            }
        }
        std::cout << "Succesfully generated file '" << dst_filename << "'\n";
    }
    else {
        std::cout << "Unable to convert the file '" << src_filename << "'\n";
        std::cout << "Error: " << translate_result(res) << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
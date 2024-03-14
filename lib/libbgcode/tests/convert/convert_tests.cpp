#include <catch_main.hpp>

#include "convert/convert.hpp"

#include <fstream>

#include <boost/nowide/cstdio.hpp>

using namespace bgcode::core;
using namespace bgcode::binarize;
using namespace bgcode::convert;

class ScopedFile
{
public:
    explicit ScopedFile(FILE* file) : m_file(file) {}
    ~ScopedFile() { if (m_file != nullptr) fclose(m_file); }
private:
    FILE* m_file{ nullptr };
};

void binary_to_ascii(const std::string& src_filename, const std::string& dst_filename)
{
    // Open source file
    FILE* src_file = boost::nowide::fopen(src_filename.c_str(), "rb");
    REQUIRE(src_file != nullptr);
    ScopedFile scoped_src_file(src_file);

    // Open destination file
    FILE* dst_file = boost::nowide::fopen(dst_filename.c_str(), "wb");
    REQUIRE(dst_file != nullptr);
    ScopedFile scoped_dst_file(dst_file);

    // Perform conversion
    EResult res = from_binary_to_ascii(*src_file, *dst_file, true);
    REQUIRE(res == EResult::Success);
}

void ascii_to_binary(const std::string& src_filename, const std::string& dst_filename, const BinarizerConfig& config)
{
    // Open source file
    FILE* src_file = boost::nowide::fopen(src_filename.c_str(), "rb");
    REQUIRE(src_file != nullptr);
    ScopedFile ab_scoped_src_file(src_file);

    // Open destination file
    FILE* dst_file = boost::nowide::fopen(dst_filename.c_str(), "wb");
    REQUIRE(dst_file != nullptr);
    ScopedFile ab_scoped_dst_file(dst_file);

    // Perform conversion
    EResult res = from_ascii_to_binary(*src_file, *dst_file, config);
    REQUIRE(res == EResult::Success);
}

void compare_binary_files(const std::string& filename1, const std::string& filename2)
{
    // Open file 1
    FILE* file1 = boost::nowide::fopen(filename1.c_str(), "rb");
    REQUIRE(file1 != nullptr);
    ScopedFile scoped_file1(file1);

    // Open file 2
    FILE* file2 = boost::nowide::fopen(filename2.c_str(), "rb");
    REQUIRE(file2 != nullptr);
    ScopedFile scoped_file2(file2);

    // Compare file sizes
    fseek(file1, 0, SEEK_END);
    const long file1_size = ftell(file1);
    rewind(file1);
    fseek(file2, 0, SEEK_END);
    const long file2_size = ftell(file2);
    rewind(file2);
    REQUIRE(file1_size == file2_size);

    // Compare file contents
    static const size_t buf_size = 4096;
    std::vector<uint8_t> buf1(buf_size);
    std::vector<uint8_t> buf2(buf_size);
    do {
        const size_t r1 = fread(buf1.data(), 1, buf_size, file1);
        const size_t r2 = fread(buf2.data(), 1, buf_size, file2);
        REQUIRE(r1 == r2);
        REQUIRE(buf1 == buf2);
    } while (!feof(file1) || !feof(file2));
}

void compare_text_files(const std::string& filename1, const std::string& filename2)
{
    // Open files
    std::ifstream file1(filename1, std::ios::binary);
    REQUIRE(file1.good());
    std::ifstream file2(filename2, std::ios::binary);
    REQUIRE(file1.good());
    // Compare file contents
    std::string line1;
    std::string line2;
    while (std::getline(file1, line1)) {
        std::getline(file2, line2);
        if (!line1.empty() && line1.back() == '\r') line1.pop_back();
        if (!line2.empty() && line2.back() == '\r') line2.pop_back();
        REQUIRE(line1 == line2);
    }
}

TEST_CASE("Convert from binary to ascii", "[Convert]")
{
    std::cout << "\nTEST: Convert from binary to ascii\n";

    const std::string src_filename = std::string(TEST_DATA_DIR) + "/mini_cube_binary.gcode";
    const std::string dst_filename = std::string(TEST_DATA_DIR) + "/mini_cube_binary_converted.gcode";
    const std::string check_filename = std::string(TEST_DATA_DIR) + "/mini_cube_binary_ascii.gcode";

    // convert from binary to ascii
    binary_to_ascii(src_filename, dst_filename);
    // compare results
    compare_text_files(dst_filename, check_filename);
}

TEST_CASE("Convert from ascii to binary", "[Convert]")
{
    std::cout << "\nTEST: Convert from ascii to binary\n";

    // convert from ascii to binary
    const std::string ab_src_filename = std::string(TEST_DATA_DIR) + "/mini_cube_ascii.gcode";
    const std::string ab_dst_filename = std::string(TEST_DATA_DIR) + "/mini_cube_ascii_converted.gcode";
    BinarizerConfig config;
    config.checksum = EChecksumType::CRC32;
    config.compression.file_metadata = ECompressionType::None;
    config.compression.print_metadata = ECompressionType::None;
    config.compression.printer_metadata = ECompressionType::None;
    config.compression.slicer_metadata = ECompressionType::Deflate;
    config.compression.gcode = ECompressionType::Heatshrink_12_4;
    config.gcode_encoding = EGCodeEncodingType::MeatPackComments;
    config.metadata_encoding = EMetadataEncodingType::INI;
    ascii_to_binary(ab_src_filename, ab_dst_filename, config);

    // convert back from binary to ascii 
    const std::string ba_src_filename = ab_dst_filename;
    const std::string ba_dst_filename = std::string(TEST_DATA_DIR) + "/mini_cube_ascii_converted_converted.gcode";
    binary_to_ascii(ba_src_filename, ba_dst_filename);

    // compare results
    compare_text_files(ba_dst_filename, ab_src_filename);
}

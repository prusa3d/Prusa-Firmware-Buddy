#include <catch_main.hpp>

#include "core/core.hpp"

#include <boost/nowide/cstdio.hpp>

using namespace bgcode::core;

class ScopedFile
{
public:
    explicit ScopedFile(FILE* file) : m_file(file) {}
    ~ScopedFile() { if (m_file != nullptr) fclose(m_file); }
private:
    FILE* m_file{ nullptr };
};

static std::string checksum_type_as_string(EChecksumType type)
{
    switch (type)
    {
    case EChecksumType::None:  { return "None"; }
    case EChecksumType::CRC32: { return "CRC32"; }
    }
    return "";
};

static std::string block_type_as_string(EBlockType type)
{
    switch (type)
    {
    case EBlockType::FileMetadata:    { return "FileMetadata"; }
    case EBlockType::GCode:           { return "GCode"; }
    case EBlockType::SlicerMetadata:  { return "SlicerMetadata"; }
    case EBlockType::PrinterMetadata: { return "PrinterMetadata"; }
    case EBlockType::PrintMetadata:   { return "PrintMetadata"; }
    case EBlockType::Thumbnail:       { return "Thumbnail"; }
    }
    return "";
};

static std::string compression_type_as_string(ECompressionType type)
{
    switch (type)
    {
    case ECompressionType::None:            { return "None"; }
    case ECompressionType::Deflate:         { return "Deflate"; }
    case ECompressionType::Heatshrink_11_4: { return "Heatshrink 11,4"; }
    case ECompressionType::Heatshrink_12_4: { return "Heatshrink 12,4"; }
    }
    return "";
};

static std::string metadata_encoding_as_string(EMetadataEncodingType type)
{
    switch (type)
    {
    case EMetadataEncodingType::INI: { return "INI"; }
    }
    return "";
};

static std::string gcode_encoding_as_string(EGCodeEncodingType type)
{
    switch (type)
    {
    case EGCodeEncodingType::None:             { return "None"; }
    case EGCodeEncodingType::MeatPack:         { return "MeatPack"; }
    case EGCodeEncodingType::MeatPackComments: { return "MeatPackComments"; }
    }
    return "";
};

static std::string thumbnail_format_as_string(EThumbnailFormat type)
{
    switch (type)
    {
    case EThumbnailFormat::JPG: { return "JPG"; }
    case EThumbnailFormat::PNG: { return "PNG"; }
    case EThumbnailFormat::QOI: { return "QOI"; }
    }
    return "";
};

 TEST_CASE("File transversal", "[Core]")
 {
     const std::string filename = std::string(TEST_DATA_DIR) + "/mini_cube_binary.gcode";
     std::cout << "\nTEST: File transversal\n";
     std::cout << "File:" << filename << "\n";

     const size_t MAX_CHECKSUM_CACHE_SIZE = 2048;
     uint8_t checksum_verify_buffer[MAX_CHECKSUM_CACHE_SIZE];

     FILE* file = boost::nowide::fopen(filename.c_str(), "rb");
     REQUIRE(file != nullptr);
     ScopedFile scoped_file(file);
     REQUIRE(is_valid_binary_gcode(*file, true, checksum_verify_buffer, sizeof(checksum_verify_buffer)) == EResult::Success);

     fseek(file, 0, SEEK_END);
     const long file_size = ftell(file);
     rewind(file);

     FileHeader file_header;
     REQUIRE(read_header(*file, file_header, nullptr) == EResult::Success);
     std::cout << "Checksum type: " << checksum_type_as_string((EChecksumType)file_header.checksum_type) << "\n";

     BlockHeader block_header;

     do
     {
         // read block header
         REQUIRE(read_next_block_header(*file, file_header, block_header, checksum_verify_buffer, sizeof(checksum_verify_buffer)) == EResult::Success);
         std::cout << "Block: " << block_type_as_string((EBlockType)block_header.type);
         std::cout << " - compression: " << compression_type_as_string((ECompressionType)block_header.compression);
         switch ((EBlockType)block_header.type)
         {
         case EBlockType::FileMetadata:
         case EBlockType::PrinterMetadata:
         case EBlockType::PrintMetadata:
         case EBlockType::SlicerMetadata:
         {
             const long curr_pos = ftell(file);
             uint16_t encoding;
             const size_t rsize = fread(&encoding, 1, sizeof(encoding), file);
             REQUIRE((ferror(file) == 0 && rsize == sizeof(encoding)));
             fseek(file, curr_pos, SEEK_SET);
             std::cout << " - encoding: " << metadata_encoding_as_string((EMetadataEncodingType)encoding);
             break;
         }
         case EBlockType::GCode:
         {
             const long curr_pos = ftell(file);
             uint16_t encoding;
             const size_t rsize = fread(&encoding, 1, sizeof(encoding), file);
             REQUIRE((ferror(file) == 0 && rsize == sizeof(encoding)));
             fseek(file, curr_pos, SEEK_SET);
             std::cout << " - encoding: " << gcode_encoding_as_string((EGCodeEncodingType)encoding);
             break;
         }
         case EBlockType::Thumbnail:
         {
             const long curr_pos = ftell(file);
             ThumbnailParams thumbnail_params;
             REQUIRE(thumbnail_params.read(*file) == EResult::Success);
             fseek(file, curr_pos, SEEK_SET);
             std::cout << " - format: " << thumbnail_format_as_string((EThumbnailFormat)thumbnail_params.format);
             std::cout << " (size: " << thumbnail_params.width << "x" << thumbnail_params.height << ")";
             break;
         }
         default: { break; }
         }
         std::cout << " - data size: " << ((block_header.compressed_size == 0) ? block_header.uncompressed_size : block_header.compressed_size);
         std::cout << "\n";

         // move to next block header
         REQUIRE(skip_block(*file, file_header, block_header) == EResult::Success);
         if (ftell(file) == file_size)
             break;
     } while (true);
 }

 TEST_CASE("Search for GCode blocks", "[Core]")
 {
     const std::string filename = std::string(TEST_DATA_DIR) + "/mini_cube_binary.gcode";
     std::cout << "\nTEST: Search for GCode blocks\n";
     std::cout << "File:" << filename << "\n";

     const size_t MAX_CHECKSUM_CACHE_SIZE = 2048;
     uint8_t checksum_verify_buffer[MAX_CHECKSUM_CACHE_SIZE];

     FILE* file = boost::nowide::fopen(filename.c_str(), "rb");
     REQUIRE(file != nullptr);
     ScopedFile scoped_file(file);
     REQUIRE(is_valid_binary_gcode(*file, true, checksum_verify_buffer, sizeof(checksum_verify_buffer)) == EResult::Success);

     fseek(file, 0, SEEK_END);
     const long file_size = ftell(file);
     rewind(file);

     FileHeader file_header;
     REQUIRE(read_header(*file, file_header, nullptr) == EResult::Success);

     BlockHeader block_header;

     do
     {
         // search and read block header by type
         REQUIRE(read_next_block_header(*file, file_header, block_header, EBlockType::GCode, checksum_verify_buffer, sizeof(checksum_verify_buffer)) == EResult::Success);
         std::cout << "Block type: " << block_type_as_string((EBlockType)block_header.type) << "\n";

         // move to next block header
         REQUIRE(skip_block(*file, file_header, block_header) == EResult::Success);
         if (ftell(file) == file_size)
             break;
     } while (true);
 }

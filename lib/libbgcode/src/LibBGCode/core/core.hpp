#ifndef BGCODE_CORE_HPP
#define BGCODE_CORE_HPP

#include "core/export.h"

#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <climits>
#include <array>
#include <vector>
#include <string>
#include <string_view>

namespace bgcode { namespace core {

enum class EResult : uint16_t
{
    Success,
    ReadError,
    WriteError,
    InvalidMagicNumber,
    InvalidVersionNumber,
    InvalidChecksumType,
    InvalidBlockType,
    InvalidCompressionType,
    InvalidMetadataEncodingType,
    InvalidGCodeEncodingType,
    DataCompressionError,
    DataUncompressionError,
    MetadataEncodingError,
    MetadataDecodingError,
    GCodeEncodingError,
    GCodeDecodingError,
    BlockNotFound,
    InvalidChecksum,
    InvalidThumbnailFormat,
    InvalidThumbnailWidth,
    InvalidThumbnailHeight,
    InvalidThumbnailDataSize,
    InvalidBinaryGCodeFile,
    InvalidAsciiGCodeFile,
    InvalidSequenceOfBlocks,
    InvalidBuffer,
    AlreadyBinarized,
    MissingPrinterMetadata,
    MissingPrintMetadata,
    MissingSlicerMetadata,
    BlockTooLarge,
};

enum class EChecksumType : uint16_t
{
    None,
    CRC32
};

enum class EBlockType : uint16_t
{
    FileMetadata,
    GCode,
    SlicerMetadata,
    PrinterMetadata,
    PrintMetadata,
    Thumbnail
};

enum class ECompressionType : uint16_t
{
    None,
    Deflate,
    Heatshrink_11_4,
    Heatshrink_12_4
};

enum class EMetadataEncodingType : uint16_t
{
    INI,
    JSON
};

enum class EGCodeEncodingType : uint16_t
{
    None,
    MeatPack,
    MeatPackComments
};

enum class EThumbnailFormat : uint16_t
{
    PNG,
    JPG,
    QOI
};

struct BGCODE_CORE_EXPORT FileHeader
{
    uint32_t magic;
    uint32_t version;
    uint16_t checksum_type;

    FileHeader();
    FileHeader(uint32_t mg, uint32_t ver, uint16_t chk_type);

    EResult write(FILE& file) const;
    EResult read(FILE& file, const uint32_t* const max_version);
};

struct BGCODE_CORE_EXPORT BlockHeader
{
    uint16_t type{ 0 };
    uint16_t compression{ 0 };
    uint32_t uncompressed_size{ 0 };
    uint32_t compressed_size{ 0 };

    BlockHeader() = default;
    BlockHeader(uint16_t type, uint16_t compression, uint32_t uncompressed_size, uint32_t compressed_size = 0);

    // Returns the position of this block in the file.
    // Position is set by calling write() and read() methods.
    long get_position() const;

    EResult write(FILE& file);
    EResult read(FILE& file);

    // Returs the size of this BlockHeader, in bytes
    size_t get_size() const;

private:
    long m_position{ 0 };
};

struct BGCODE_CORE_EXPORT ThumbnailParams
{
    uint16_t format;
    uint16_t width;
    uint16_t height;

    EResult write(FILE& file) const;
    EResult read(FILE& file);
};

// Returns a string description of the given result
extern BGCODE_CORE_EXPORT std::string_view translate_result(EResult result);

// Returns EResult::Success if the given file is a valid binary gcode
// If check_contents is set to true, the order of the blocks is checked
// Does not modify the file position
// Caller is responsible for providing buffer for checksum calculation, if needed.
extern BGCODE_CORE_EXPORT EResult is_valid_binary_gcode(FILE& file, bool check_contents = false, std::byte* cs_buffer = nullptr,
    size_t cs_buffer_size = 0);

// Reads the file header.
// If max_version is not null, version is checked against the passed value.
// If return == EResult::Success:
// - header will contain the file header.
// - file position will be set at the start of the 1st block header.
extern BGCODE_CORE_EXPORT EResult read_header(FILE& file, FileHeader& header, const uint32_t* const max_version);

// Reads next block header from the current file position.
// File position must be at the start of a block header.
// If return == EResult::Success:
// - block_header will contain the header of the block.
// - file position will be set at the start of the block parameters data.
// Caller is responsible for providing buffer for checksum calculation, if needed.
extern BGCODE_CORE_EXPORT EResult read_next_block_header(FILE& file, const FileHeader& file_header, BlockHeader& block_header,
    std::byte* cs_buffer = nullptr, size_t cs_buffer_size = 0);

// Searches and reads next block header with the given type from the current file position.
// File position must be at the start of a block header.
// If return == EResult::Success:
// - block_header will contain the header of the block with the required type.
// - file position will be set at the start of the block parameters data.
// otherwise:
// - file position will keep the current value.
// Caller is responsible for providing buffer for checksum calculation, if needed.
extern BGCODE_CORE_EXPORT EResult read_next_block_header(FILE& file, const FileHeader& file_header, BlockHeader& block_header, EBlockType type,
    std::byte* cs_buffer = nullptr, size_t cs_buffer_size = 0);

// Calculates block checksum and verify it against checksum stored in file.
// Caller is responsible for providing buffer for checksum calculation, bigger buffer means faster calculation and vice versa.
// If return == EResult::Success:
// - file position will be set at the start of the next block header.
extern BGCODE_CORE_EXPORT EResult verify_block_checksum(FILE& file, const FileHeader& file_header, const BlockHeader& block_header, std::byte* buffer,
    size_t buffer_size);

// Skips the content (parameters + data + checksum) of the block with the given block header.
// File position must be at the start of the block parameters.
// If return == EResult::Success:
// - file position will be set at the start of the next block header.
extern BGCODE_CORE_EXPORT EResult skip_block_content(FILE& file, const FileHeader& file_header, const BlockHeader& block_header);

// Skips the block with the given block header.
// File position must be set by a previous call to BlockHeader::write() or BlockHeader::read().
// If return == EResult::Success:
// - file position will be set at the start of the next block header.
extern BGCODE_CORE_EXPORT EResult skip_block(FILE& file, const FileHeader& file_header, const BlockHeader& block_header);

// Returns the size of the parameters of the given block type, in bytes.
extern BGCODE_CORE_EXPORT size_t block_parameters_size(EBlockType type);

// Returns the size of the payload (parameters + data) of the block with the given header, in bytes.
extern BGCODE_CORE_EXPORT size_t block_payload_size(const BlockHeader& block_header);

// Returns the size of the checksum of the given type, in bytes.
extern BGCODE_CORE_EXPORT size_t checksum_size(EChecksumType type);

// Returns the size of the content (parameters + data + checksum) of the block with the given header, in bytes.
extern BGCODE_CORE_EXPORT size_t block_content_size(const FileHeader& file_header, const BlockHeader& block_header);

// Highest version of the binary format supported by this library instance
extern BGCODE_CORE_EXPORT uint32_t bgcode_version() noexcept;

// Version of the library
extern BGCODE_CORE_EXPORT const char* version() noexcept;

}} // bgcode::core

#endif // BGCODE_CORE_HPP

#ifndef _BGCODE_CORE_HPP_
#define _BGCODE_CORE_HPP_

#include "core/export.h"

#include <cstdio>
#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <string_view>

namespace bgcode { namespace core {

static constexpr const std::array<uint8_t, 4> MAGIC{ 'G', 'C', 'D', 'E' };
// Library version
static constexpr const uint32_t VERSION = 1;
// Max size of checksum buffer data, in bytes
// Increase this value if you implement a checksum algorithm needing a bigger buffer
#define MAX_CHECKSUM_SIZE 4

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
    AlreadyBinarized
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
    INI
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

class Checksum
{
public:
    // Constructs a checksum of the given type.
    // The checksum data are sized accordingly.
    explicit Checksum(EChecksumType type);

    EChecksumType get_type() const;

    // Append vector of data to checksum
    void append(const std::vector<uint8_t>& data);
    // Append data to the checksum
    void append(const uint8_t* data, size_t size);
    // Append any aritmetic data to the checksum (shorthand for aritmetic types)
    template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
    void append(T& data) { append(reinterpret_cast<const uint8_t*>(&data), sizeof(data)); }

    // Returns true if the given checksum is equal to this one
    bool matches(Checksum& other);

    EResult write(FILE& file);
    EResult read(FILE& file);

private:
    EChecksumType m_type;
    // actual size of checksum buffer, type dependent
    size_t m_size;
    std::array<uint8_t, MAX_CHECKSUM_SIZE> m_checksum;
};

struct FileHeader
{
    uint32_t magic{ *(uint32_t*)(MAGIC.data()) };
    uint32_t version{ VERSION };
    uint16_t checksum_type{ (uint16_t)EChecksumType::None };

    EResult write(FILE& file) const;
    EResult read(FILE& file, const uint32_t* const max_version);
};

struct BlockHeader
{
    uint16_t type{ 0 };
    uint16_t compression{ 0 };
    uint32_t uncompressed_size{ 0 };
    uint32_t compressed_size{ 0 };

    BlockHeader() = default;
    BlockHeader(uint16_t type, uint16_t compression, uint32_t uncompressed_size, uint32_t compressed_size = 0);

    // Updates the given checksum with the data of this BlockHeader
    void update_checksum(Checksum& checksum) const;

    // Returns the position of this block in the file.
    // Position is set by calling write() and read() methods.
    long get_position() const;

    EResult write(FILE& file) const;
    EResult read(FILE& file);

    // Returs the size of this BlockHeader, in bytes
    size_t get_size() const;

private:
    mutable long m_position{ 0 };
};

struct ThumbnailParams
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
extern BGCODE_CORE_EXPORT EResult is_valid_binary_gcode(FILE& file, bool check_contents = false, uint8_t* cs_buffer = nullptr,
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
    uint8_t* cs_buffer = nullptr, size_t cs_buffer_size = 0);

// Searches and reads next block header with the given type from the current file position.
// File position must be at the start of a block header.
// If return == EResult::Success:
// - block_header will contain the header of the block with the required type.
// - file position will be set at the start of the block parameters data.
// otherwise:
// - file position will keep the current value.
// Caller is responsible for providing buffer for checksum calculation, if needed.
extern BGCODE_CORE_EXPORT EResult read_next_block_header(FILE& file, const FileHeader& file_header, BlockHeader& block_header, EBlockType type,
    uint8_t* cs_buffer = nullptr, size_t cs_buffer_size = 0);

// Calculates block checksum and verify it against checksum stored in file.
// Caller is responsible for providing buffer for checksum calculation, bigger buffer means faster calculation and vice versa.
// If return == EResult::Success:
// - file position will be set at the start of the next block header.
extern BGCODE_CORE_EXPORT EResult verify_block_checksum(FILE& file, const FileHeader& file_header, const BlockHeader& block_header, uint8_t* buffer,
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

}} // bgcode::core

#endif // _BGCODE_CORE_HPP_

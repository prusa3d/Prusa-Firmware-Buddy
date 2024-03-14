#ifndef _BGCODE_BINARIZE_HPP_
#define _BGCODE_BINARIZE_HPP_

#include "binarize/export.h"
#include "core/core.hpp"

namespace bgcode { namespace binarize {

struct BaseMetadataBlock
{
    // type of data encoding
    uint16_t encoding_type{ 0 };
    // data in key/value form
    std::vector<std::pair<std::string, std::string>> raw_data;

    // write block header and data in encoded format
    core::EResult write(FILE& file, core::EBlockType block_type, core::ECompressionType compression_type, core::Checksum& checksum) const;
    // read block data in encoded format
    core::EResult read_data(FILE& file, const core::BlockHeader& block_header);
};

struct FileMetadataBlock : public BaseMetadataBlock
{
    // write block header and data
    core::EResult write(FILE& file, core::ECompressionType compression_type, core::EChecksumType checksum_type) const;
    // read block data
    core::EResult read_data(FILE& file, const core::FileHeader& file_header, const core::BlockHeader& block_header);
};

struct PrintMetadataBlock : public BaseMetadataBlock
{
    // write block header and data
    core::EResult write(FILE& file, core::ECompressionType compression_type, core::EChecksumType checksum_type) const;
    // read block data
    core::EResult read_data(FILE& file, const core::FileHeader& file_header, const core::BlockHeader& block_header);
};

struct PrinterMetadataBlock : public BaseMetadataBlock
{
    // write block header and data
    core::EResult write(FILE& file, core::ECompressionType compression_type, core::EChecksumType checksum_type) const;
    // read block data
    core::EResult read_data(FILE& file, const core::FileHeader& file_header, const core::BlockHeader& block_header);
};

struct ThumbnailBlock
{
    core::ThumbnailParams params;
    std::vector<uint8_t> data;

    // write block header and data
    core::EResult write(FILE& file, core::EChecksumType checksum_type);
    // read block data
    core::EResult read_data(FILE& file, const core::FileHeader& file_header, const core::BlockHeader& block_header);

private:
    void update_checksum(core::Checksum& checksum) const;
};

struct GCodeBlock
{
    uint16_t encoding_type{ 0 };
    std::string raw_data;

    // write block header and data
    core::EResult write(FILE& file, core::ECompressionType compression_type, core::EChecksumType checksum_type) const;
    // read block data
    core::EResult read_data(FILE& file, const core::FileHeader& file_header, const core::BlockHeader& block_header);
};

struct SlicerMetadataBlock : public BaseMetadataBlock
{
    // write block header and data
    core::EResult write(FILE& file, core::ECompressionType compression_type, core::EChecksumType checksum_type) const;
    // read block data
    core::EResult read_data(FILE& file, const core::FileHeader& file_header, const core::BlockHeader& block_header);
};

struct BinarizerConfig
{
    struct Compression
    {
        core::ECompressionType file_metadata{ core::ECompressionType::None };
        core::ECompressionType printer_metadata{ core::ECompressionType::None };
        core::ECompressionType print_metadata{ core::ECompressionType::None };
        core::ECompressionType slicer_metadata{ core::ECompressionType::None };
        core::ECompressionType gcode{ core::ECompressionType::None };
    };
    Compression compression;
    core::EGCodeEncodingType gcode_encoding{ core::EGCodeEncodingType::None };
    core::EMetadataEncodingType metadata_encoding{ core::EMetadataEncodingType::INI };
    core::EChecksumType checksum{ core::EChecksumType::CRC32 };
};

struct BinaryData
{
    FileMetadataBlock file_metadata;
    PrinterMetadataBlock printer_metadata;
    std::vector<ThumbnailBlock> thumbnails;
    SlicerMetadataBlock slicer_metadata;
    PrintMetadataBlock print_metadata;
};

class Binarizer
{
public:
    bool is_enabled() const;
    void set_enabled(bool enable);

    BinaryData& get_binary_data();
    const BinaryData& get_binary_data() const;

    size_t get_max_gcode_cache_size() const;
    void set_max_gcode_cache_size(size_t size);

    core::EResult initialize(FILE& file, const BinarizerConfig& config);
    core::EResult append_gcode(const std::string& gcode);
    core::EResult finalize();

private:
    FILE* m_file{ nullptr };
    bool m_enabled{ false };
    BinarizerConfig m_config;
    BinaryData m_binary_data;
    std::string m_gcode_cache;
    size_t m_gcode_cache_size{ 65536 };
};

}} // bgcode::core

#endif // _BGCODE_BINARIZE_HPP_

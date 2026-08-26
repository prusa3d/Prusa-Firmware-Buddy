#include "core_impl.hpp"
#include <cstring>

namespace bgcode { namespace core {

template<class T>
static bool write_to_file(FILE& file, const T* data, size_t data_size)
{
    const size_t wsize = fwrite(static_cast<const void*>(data), 1, data_size, &file);
    return !ferror(&file) && wsize == data_size;
}

template<class T>
static bool read_from_file(FILE& file, T *data, size_t data_size)
{
    static_assert(!std::is_const_v<T>, "Type of output buffer cannot be const!");

    const size_t rsize = fread(static_cast<void *>(data), 1, data_size, &file);
    return !ferror(&file) && rsize == data_size;
}

EResult verify_block_checksum(FILE& file, const FileHeader& file_header,
                              const BlockHeader& block_header, std::byte* buffer, size_t buffer_size)
{
    if (buffer == nullptr || buffer_size == 0)
        return EResult::InvalidBuffer;

    // No checksum in file, no checking, just return success
    if (file_header.checksum_type == (uint16_t)EChecksumType::None)
        return EResult::Success;

    // seek after header, where payload starts
    if (fseek(&file, block_header.get_position() + (long)block_header.get_size(), SEEK_SET) != 0)
        return EResult::ReadError;

    Checksum curr_cs((EChecksumType)file_header.checksum_type);
    // update block checksum block header
    update_checksum(curr_cs, block_header);

    // read block payload
    size_t remaining_payload_size = block_payload_size(block_header);
    while (remaining_payload_size > 0) {
        const size_t size_to_read = std::min(remaining_payload_size, buffer_size);
        if (!read_from_file(file, buffer, size_to_read))
            return EResult::ReadError;
        curr_cs.append(buffer, size_to_read);
        remaining_payload_size -= size_to_read;
    }

    // read checksum
    Checksum read_cs((EChecksumType)file_header.checksum_type);
    EResult res = read_cs.read(file);
    if (res != EResult::Success)
        // propagate error
        return res;

    // Verify checksum
    if (!curr_cs.matches(read_cs))
        return EResult::InvalidChecksum;

    return EResult::Success;
}

Checksum::Checksum(EChecksumType type)
    : m_type(type), m_size(checksum_size(type))
{
    m_checksum.fill(std::byte{0});
}

void Checksum::append(const std::vector<std::byte>& data)
{
    append(data.data(), data.size());
}

bool Checksum::matches(Checksum& other)
{
    return m_checksum == other.m_checksum;
}

EResult Checksum::write(FILE& file)
{
    if (m_type != EChecksumType::None) {
        if (!write_to_file(file, m_checksum.data(), m_size))
            return EResult::WriteError;
    }
    return EResult::Success;
}

EResult Checksum::read(FILE& file)
{
    if (m_type != EChecksumType::None) {
        if (!read_from_file(file, m_checksum.data(), m_size))
            return EResult::ReadError;
    }
    return EResult::Success;
}

FileHeader::FileHeader()
    : magic{MAGICi32}
    , version{VERSION}
    , checksum_type{static_cast<uint16_t>(EChecksumType::None)}
{}

FileHeader::FileHeader(uint32_t mg, uint32_t ver, uint16_t chk_type)
    : magic{mg}, version{ver}, checksum_type{chk_type}
{}

EResult FileHeader::write(FILE& file) const
{
    if (magic != MAGICi32)
        return EResult::InvalidMagicNumber;
    if (checksum_type >= checksum_types_count())
        return EResult::InvalidChecksumType;

    if (!write_to_file(file, &magic, sizeof(magic)))
       return EResult::WriteError;
    if (!write_to_file(file, &version, sizeof(version)))
        return EResult::WriteError;
    if (!write_to_file(file, &checksum_type, sizeof(checksum_type)))
        return EResult::WriteError;

    return EResult::Success;
}

EResult FileHeader::read(FILE& file, const uint32_t* const max_version)
{
    if (!read_from_file(file, &magic, sizeof(magic)))
        return EResult::ReadError;
    if (magic != MAGICi32)
        return EResult::InvalidMagicNumber;

    if (!read_from_file(file, &version, sizeof(version)))
        return EResult::ReadError;
    if (max_version != nullptr && version > *max_version)
        return EResult::InvalidVersionNumber;

    if (!read_from_file(file, &checksum_type, sizeof(checksum_type)))
        return EResult::ReadError;
    if (checksum_type >= checksum_types_count())
        return EResult::InvalidChecksumType;

    return EResult::Success;
}

BlockHeader::BlockHeader(uint16_t type, uint16_t compression, uint32_t uncompressed_size, uint32_t compressed_size)
  : type(type)
  , compression(compression)
  , uncompressed_size(uncompressed_size)
  , compressed_size(compressed_size)
{}

long BlockHeader::get_position() const
{
    return m_position;
}

EResult BlockHeader::write(FILE& file)
{
    m_position = ftell(&file);
    if (!write_to_file(file, &type, sizeof(type)))
        return EResult::WriteError;
    if (!write_to_file(file, &compression, sizeof(compression)))
        return EResult::WriteError;
    if (!write_to_file(file, &uncompressed_size, sizeof(uncompressed_size)))
        return EResult::WriteError;
    if (compression != (uint16_t)ECompressionType::None) {
        if (!write_to_file(file, &compressed_size, sizeof(compressed_size)))
            return EResult::WriteError;
    }
    return EResult::Success;
}

EResult BlockHeader::read(FILE& file)
{
    m_position = ftell(&file);
    if (!read_from_file(file, &type, sizeof(type)))
        return EResult::ReadError;
    if (type >= block_types_count())
        return EResult::InvalidBlockType;

    if (!read_from_file(file, &compression, sizeof(compression)))
        return EResult::ReadError;
    if (compression >= compression_types_count())
        return EResult::InvalidCompressionType;

    if (!read_from_file(file, &uncompressed_size, sizeof(uncompressed_size)))
        return EResult::ReadError;
    if (compression != (uint16_t)ECompressionType::None) {
        if (!read_from_file(file, &compressed_size, sizeof(compressed_size)))
            return EResult::ReadError;
    }

    return EResult::Success;
}

size_t BlockHeader::get_size() const {
    return sizeof(type) + sizeof(compression) + sizeof(uncompressed_size) +
        ((compression == (uint16_t)ECompressionType::None)? 0 : sizeof(compressed_size));
}

EResult ThumbnailParams::write(FILE& file) const {
    if (!write_to_file(file, &format, sizeof(format)))
        return EResult::WriteError;
    if (!write_to_file(file, &width, sizeof(width)))
        return EResult::WriteError;
    if (!write_to_file(file, &height, sizeof(height)))
        return EResult::WriteError;
    return EResult::Success;
}

EResult ThumbnailParams::read(FILE& file){
    if (!read_from_file(file, &format, sizeof(format)))
        return EResult::ReadError;
    if (!read_from_file(file, &width, sizeof(width)))
        return EResult::ReadError;
    if (!read_from_file(file, &height, sizeof(height)))
        return EResult::ReadError;
    return EResult::Success;
}

BGCODE_CORE_EXPORT std::string_view translate_result(EResult result)
{
    using namespace std::literals;
    switch (result)
    {
    case EResult::Success:                     { return "Success"sv; }
    case EResult::ReadError:                   { return "Read error"sv; }
    case EResult::WriteError:                  { return "Write error"sv; }
    case EResult::InvalidMagicNumber:          { return "Invalid magic number"sv; }
    case EResult::InvalidVersionNumber:        { return "Invalid version number"sv; }
    case EResult::InvalidChecksumType:         { return "Invalid checksum type"sv; }
    case EResult::InvalidBlockType:            { return "Invalid block type"sv; }
    case EResult::InvalidCompressionType:      { return "Invalid compression type"sv; }
    case EResult::InvalidMetadataEncodingType: { return "Invalid metadata encoding type"sv; }
    case EResult::InvalidGCodeEncodingType:    { return "Invalid gcode encoding type"sv; }
    case EResult::DataCompressionError:        { return "Data compression error"sv; }
    case EResult::DataUncompressionError:      { return "Data uncompression error"sv; }
    case EResult::MetadataEncodingError:       { return "Metadata encoding error"sv; }
    case EResult::MetadataDecodingError:       { return "Metadata decoding error"sv; }
    case EResult::GCodeEncodingError:          { return "GCode encoding error"sv; }
    case EResult::GCodeDecodingError:          { return "GCode decoding error"sv; }
    case EResult::BlockNotFound:               { return "Block not found"sv; }
    case EResult::InvalidChecksum:             { return "Invalid checksum"sv; }
    case EResult::InvalidThumbnailFormat:      { return "Invalid thumbnail format"sv; }
    case EResult::InvalidThumbnailWidth:       { return "Invalid thumbnail width"sv; }
    case EResult::InvalidThumbnailHeight:      { return "Invalid thumbnail height"sv; }
    case EResult::InvalidThumbnailDataSize:    { return "Invalid thumbnail data size"sv; }
    case EResult::InvalidBinaryGCodeFile:      { return "Invalid binary GCode file"sv; }
    case EResult::InvalidAsciiGCodeFile:       { return "Invalid ascii GCode file"sv; }
    case EResult::InvalidSequenceOfBlocks:     { return "Invalid sequence of blocks"sv; }
    case EResult::InvalidBuffer:               { return "Invalid buffer"sv; }
    case EResult::AlreadyBinarized:            { return "Already binarized"sv; }
    case EResult::MissingPrinterMetadata:      { return "Missing printer metadata"sv; }
    case EResult::MissingPrintMetadata:        { return "Missing print metadata"sv; }
    case EResult::MissingSlicerMetadata:       { return "Missing slicer metadata"sv; }
    case EResult::BlockTooLarge:               { return "Block data size exceeds the configured maximum"sv; }
    }
    return std::string_view();
}

BGCODE_CORE_EXPORT EResult is_valid_binary_gcode(FILE& file, bool check_contents, std::byte* cs_buffer, size_t cs_buffer_size)
{
    // cache file position
    const long curr_pos = ftell(&file);
    rewind(&file);

    // check magic number
    std::array<char, 4> magic;
    const size_t rsize = fread((void*)magic.data(), 1, magic.size(), &file);
    if (ferror(&file) && rsize != magic.size())
        return EResult::ReadError;
    else if (magic != MAGIC) {
        // restore file position
        fseek(&file, curr_pos, SEEK_SET);
        return EResult::InvalidMagicNumber;
    }

    // check contents
    if (check_contents) {
        fseek(&file, 0, SEEK_END);
        const long file_size = ftell(&file);
        rewind(&file);

        // read header
        FileHeader file_header;
        EResult res = read_header(file, file_header, nullptr);
        if (res != EResult::Success) {
            // restore file position
            fseek(&file, curr_pos, SEEK_SET);
            // propagate error
            return res;
        }
        BlockHeader block_header;
        // read file metadata block header, if present
        res = read_next_block_header(file, file_header, block_header, cs_buffer, cs_buffer_size);
        if (res != EResult::Success) {
            // restore file position
            fseek(&file, curr_pos, SEEK_SET);
            // propagate error
            return res;
        }
        if ((EBlockType)block_header.type != EBlockType::FileMetadata &&
            (EBlockType)block_header.type != EBlockType::PrinterMetadata) {
            // restore file position
            fseek(&file, curr_pos, SEEK_SET);
            return EResult::InvalidBlockType;
        }

        // read printer metadata block header, if file metadata block is present
        if ((EBlockType)block_header.type == EBlockType::FileMetadata) {
            res = skip_block(file, file_header, block_header);
            if (res != EResult::Success) {
                // restore file position
                fseek(&file, curr_pos, SEEK_SET);
                // propagate error
                return res;
            }
            res = read_next_block_header(file, file_header, block_header, cs_buffer, cs_buffer_size);
            if (res != EResult::Success) {
                // restore file position
                fseek(&file, curr_pos, SEEK_SET);
                // propagate error
                return res;
            }
        }
        if ((EBlockType)block_header.type != EBlockType::PrinterMetadata) {
            // restore file position
            fseek(&file, curr_pos, SEEK_SET);
            return EResult::InvalidBlockType;
        }

        // read thumbnails block headers, if present
        res = skip_block(file, file_header, block_header);
        if (res != EResult::Success) {
            // restore file position
            fseek(&file, curr_pos, SEEK_SET);
            // propagate error
            return res;
        }
        res = read_next_block_header(file, file_header, block_header, cs_buffer, cs_buffer_size);
        if (res != EResult::Success) {
            // restore file position
            fseek(&file, curr_pos, SEEK_SET);
            // propagate error
            return res;
        }
        while ((EBlockType)block_header.type == EBlockType::Thumbnail) {
            res = skip_block(file, file_header, block_header);
            if (res != EResult::Success) {
                // restore file position
                fseek(&file, curr_pos, SEEK_SET);
                // propagate error
                return res;
            }
            res = read_next_block_header(file, file_header, block_header, cs_buffer, cs_buffer_size);
            if (res != EResult::Success) {
                // restore file position
                fseek(&file, curr_pos, SEEK_SET);
                // propagate error
                return res;
            }
        }

        // read print metadata block header
        if ((EBlockType)block_header.type != EBlockType::PrintMetadata) {
            // restore file position
            fseek(&file, curr_pos, SEEK_SET);
            return EResult::InvalidBlockType;
        }

        for (size_t i = 0; i < 2; i++) {
            // read slicer metadata block header
            res = skip_block(file, file_header, block_header);
            if (res != EResult::Success) {
                // restore file position
                fseek(&file, curr_pos, SEEK_SET);
                // propagate error
                return res;
            }
            res = read_next_block_header(file, file_header, block_header, cs_buffer, cs_buffer_size);
            if (res != EResult::Success) {
                // restore file position
                fseek(&file, curr_pos, SEEK_SET);
                // propagate error
                return res;
            }
            if ((EBlockType)block_header.type != EBlockType::SlicerMetadata && i == 0) {
                // restore file position
                fseek(&file, curr_pos, SEEK_SET);
                return EResult::InvalidBlockType;
            }
        }

        // read gcode block headers
        do {
            res = skip_block(file, file_header, block_header);
            if (res != EResult::Success) {
                // restore file position
                fseek(&file, curr_pos, SEEK_SET);
                // propagate error
                return res;
            }
            if (ftell(&file) == file_size)
                break;
            res = read_next_block_header(file, file_header, block_header, cs_buffer, cs_buffer_size);
            if (res != EResult::Success) {
                // restore file position
                fseek(&file, curr_pos, SEEK_SET);
                // propagate error
                return res;
            }
            if ((EBlockType)block_header.type != EBlockType::GCode) {
                // restore file position
                fseek(&file, curr_pos, SEEK_SET);
                return EResult::InvalidBlockType;
            }
        } while (!feof(&file));
    }

    fseek(&file, curr_pos, SEEK_SET);
    return EResult::Success;
}

BGCODE_CORE_EXPORT EResult read_header(FILE& file, FileHeader& header, const uint32_t* const max_version)
{
    rewind(&file);
    return header.read(file, max_version);
}

BGCODE_CORE_EXPORT EResult read_next_block_header(FILE& file, const FileHeader& file_header, BlockHeader& block_header,
    std::byte* cs_buffer, size_t cs_buffer_size)
{
    EResult res = block_header.read(file);
    if (res == EResult::Success && cs_buffer != nullptr && cs_buffer_size > 0) {
        res = verify_block_checksum(file, file_header, block_header, cs_buffer, cs_buffer_size);
        // return to payload position after checksum verification
        if (fseek(&file, block_header.get_position() + static_cast<long>(block_header.get_size()), SEEK_SET) != 0)
            res = EResult::ReadError;
    }

    return res;
}

BGCODE_CORE_EXPORT EResult read_next_block_header(FILE& file, const FileHeader& file_header, BlockHeader& block_header, EBlockType type,
    std::byte* cs_buffer, size_t cs_buffer_size)
{
    // cache file position
    const long curr_pos = ftell(&file);

    do {
        EResult res = read_next_block_header(file, file_header, block_header, nullptr, 0); // intentionally skip checksum verification
        if (res != EResult::Success)
            // propagate error
            return res;
        else if (feof(&file)) {
            // block not found
            // restore file position
            fseek(&file, curr_pos, SEEK_SET);
            return EResult::BlockNotFound;
        }
        else if ((EBlockType)block_header.type == type) {
            // block found
            if (cs_buffer != nullptr && cs_buffer_size > 0) {
                // checksum verification requested
                res = verify_block_checksum(file, file_header, block_header, cs_buffer, cs_buffer_size);
                // return to payload position after checksum verification
                if (fseek(&file, block_header.get_position() + (long)block_header.get_size(), SEEK_SET) != 0)
                    res = EResult::ReadError;
                return res; // propagate error or success
            }
            return EResult::Success;
        }

        if (!feof(&file)) {
            res = skip_block(file, file_header, block_header);
            if (res != EResult::Success)
                // propagate error
                return res;
        }
    } while (true);
}

BGCODE_CORE_EXPORT size_t block_parameters_size(EBlockType type)
{
    switch (type)
    {
    case EBlockType::FileMetadata:    { return sizeof(uint16_t); } /* encoding_type */
    case EBlockType::GCode:           { return sizeof(uint16_t); } /* encoding_type */
    case EBlockType::SlicerMetadata:  { return sizeof(uint16_t); } /* encoding_type */
    case EBlockType::PrinterMetadata: { return sizeof(uint16_t); } /* encoding_type */
    case EBlockType::PrintMetadata:   { return sizeof(uint16_t); } /* encoding_type */
    case EBlockType::Thumbnail:       { return sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t); } /* format, width, height */
    }
    return 0;
}

BGCODE_CORE_EXPORT EResult skip_block_content(FILE& file, const FileHeader& file_header, const BlockHeader& block_header)
{
    fseek(&file, (long)block_content_size(file_header, block_header), SEEK_CUR);
    return ferror(&file) ? EResult::ReadError : EResult::Success;
}

BGCODE_CORE_EXPORT EResult skip_block(FILE& file, const FileHeader& file_header, const BlockHeader& block_header)
{
    fseek(&file, block_header.get_position() + (long)block_header.get_size() + (long)block_content_size(file_header, block_header), SEEK_SET);
    return ferror(&file) ? EResult::ReadError : EResult::Success;
}

BGCODE_CORE_EXPORT size_t block_payload_size(const BlockHeader& block_header)
{
    size_t ret = block_parameters_size((EBlockType)block_header.type);
    ret += ((ECompressionType)block_header.compression == ECompressionType::None) ?
        block_header.uncompressed_size : block_header.compressed_size;
    return ret;
}

BGCODE_CORE_EXPORT size_t checksum_size(EChecksumType type)
{
  switch (type)
  {
  case EChecksumType::None:  { return 0; }
  case EChecksumType::CRC32: { return 4; }
  }
  return 0;
}

BGCODE_CORE_EXPORT size_t block_content_size(const FileHeader& file_header, const BlockHeader& block_header)
{
  return block_payload_size(block_header) + checksum_size((EChecksumType)file_header.checksum_type);
}

uint32_t bgcode_version() noexcept
{
    return VERSION;
}

const char *version() noexcept
{
    return LibBGCode_VERSION;
}

} // namespace core
} // namespace bgcode

#include "binarize.hpp"
#include "meatpack.hpp"

extern "C" {
#include <heatshrink/heatshrink_encoder.h>
#include <heatshrink/heatshrink_decoder.h>
}
#include <zlib.h>

#include <cstring>
#include <cassert>

namespace bgcode {
using namespace core;
namespace binarize {

static bool write_to_file(FILE& file, const void* data, size_t data_size)
{
    const size_t wsize = fwrite(data, 1, data_size, &file);
    return !ferror(&file) && wsize == data_size;
}

static bool read_from_file(FILE& file, void* data, size_t data_size)
{
    const size_t rsize = fread(data, 1, data_size, &file);
    return !ferror(&file) && rsize == data_size;
}

static std::vector<uint8_t> encode(const void* data, size_t data_size)
{
    std::vector<uint8_t> ret(data_size);
    memcpy(ret.data(), data, data_size);
    return ret;
}

static uint16_t metadata_encoding_types_count() { return 1 + (uint16_t)EMetadataEncodingType::INI; }
static uint16_t thumbnail_formats_count()       { return 1 + (uint16_t)EThumbnailFormat::QOI; }
static uint16_t gcode_encoding_types_count()    { return 1 + (uint16_t)EGCodeEncodingType::MeatPackComments; }

static bool encode_metadata(const std::vector<std::pair<std::string, std::string>>& src, std::vector<uint8_t>& dst,
    EMetadataEncodingType encoding_type)
{
    for (const auto& [key, value] : src) {
        switch (encoding_type)
        {
        case EMetadataEncodingType::INI:
        {
            dst.insert(dst.end(), key.begin(), key.end());
            dst.emplace_back('=');
            dst.insert(dst.end(), value.begin(), value.end());
            dst.emplace_back('\n');
            break;
        }
        }
    }
    return true;
}

static bool decode_metadata(const std::vector<uint8_t>& src, std::vector<std::pair<std::string, std::string>>& dst,
    EMetadataEncodingType encoding_type)
{
    switch (encoding_type)
    {
    case EMetadataEncodingType::INI:
    {
        auto begin_it = src.begin();
        auto end_it = src.begin();
        while (end_it != src.end()) {
            while (end_it != src.end() && *end_it != '\n') {
                ++end_it;
            }
            const std::string item(begin_it, end_it);
            const size_t pos = item.find_first_of('=');
            if (pos != std::string::npos) {
                dst.emplace_back(std::make_pair(item.substr(0, pos), item.substr(pos + 1)));
                begin_it = ++end_it;
            }
        }
        break;
    }
    }

    return true;
}

static bool encode_gcode(const std::string& src, std::vector<uint8_t>& dst, EGCodeEncodingType encoding_type)
{
    switch (encoding_type)
    {
    case EGCodeEncodingType::None:
    {
        dst.insert(dst.end(), src.begin(), src.end());
        break;
    }
    case EGCodeEncodingType::MeatPack:
    case EGCodeEncodingType::MeatPackComments:
    {
        uint8_t binarizer_flags = (encoding_type == EGCodeEncodingType::MeatPack) ? MeatPack::Flag_RemoveComments : 0;
        binarizer_flags |= MeatPack::Flag_OmitWhitespaces;
        MeatPack::MPBinarizer binarizer(binarizer_flags);
        binarizer.initialize(dst);
        auto begin_it = src.begin();
        auto end_it = src.begin();
        while (end_it != src.end()) {
            while (end_it != src.end() && *end_it != '\n') {
                ++end_it;
            }
            const std::string line(begin_it, ++end_it);
            binarizer.binarize_line(line, dst);
            begin_it = end_it;
        }
        binarizer.finalize(dst);
        break;
    }
    }
    return true;
}

static bool decode_gcode(const std::vector<uint8_t>& src, std::string& dst, EGCodeEncodingType encoding_type)
{
    switch (encoding_type)
    {
    case EGCodeEncodingType::None:
    {
        dst.insert(dst.end(), src.begin(), src.end());
        break;
    }
    case EGCodeEncodingType::MeatPack:
    case EGCodeEncodingType::MeatPackComments:
    {
        MeatPack::unbinarize(src, dst);
        break;
    }
    }
    return true;
}

static bool compress(const std::vector<uint8_t>& src, std::vector<uint8_t>& dst, ECompressionType compression_type)
{
    switch (compression_type)
    {
    case ECompressionType::Deflate:
    {
        dst.clear();

        const size_t BUFSIZE = 2048;
        std::vector<uint8_t> temp_buffer(BUFSIZE);

        z_stream strm{};
        strm.next_in = const_cast<uint8_t*>(src.data());
        strm.avail_in = (uInt)src.size();
        strm.next_out = temp_buffer.data();
        strm.avail_out = BUFSIZE;

        const int level = Z_DEFAULT_COMPRESSION;
        int res = deflateInit(&strm, level);
        if (res != Z_OK)
            return false;

        while (strm.avail_in > 0) {
            res = deflate(&strm, Z_NO_FLUSH);
            if (res != Z_OK) {
                deflateEnd(&strm);
                return false;
            }
            if (strm.avail_out == 0) {
                dst.insert(dst.end(), temp_buffer.data(), temp_buffer.data() + BUFSIZE);
                strm.next_out = temp_buffer.data();
                strm.avail_out = BUFSIZE;
            }
        }

        int deflate_res = Z_OK;
        while (deflate_res == Z_OK) {
            if (strm.avail_out == 0) {
                dst.insert(dst.end(), temp_buffer.data(), temp_buffer.data() + BUFSIZE);
                strm.next_out = temp_buffer.data();
                strm.avail_out = BUFSIZE;
            }
            deflate_res = deflate(&strm, Z_FINISH);
        }

        if (deflate_res != Z_STREAM_END) {
            deflateEnd(&strm);
            return false;
        }

        dst.insert(dst.end(), temp_buffer.data(), temp_buffer.data() + BUFSIZE - strm.avail_out);
        deflateEnd(&strm);
        break;
    }
    case ECompressionType::Heatshrink_11_4:
    case ECompressionType::Heatshrink_12_4:
    {
        const uint8_t window_sz = (compression_type == ECompressionType::Heatshrink_11_4) ? 11 : 12;
        const uint8_t lookahead_sz = 4;
        heatshrink_encoder* encoder = heatshrink_encoder_alloc(window_sz, lookahead_sz);
        if (encoder == nullptr)
            return false;

        // calculate the maximum compressed size (assuming a conservative estimate)
        const size_t src_size = src.size();
        const size_t max_compressed_size = src_size + (src_size >> 2);
        dst.resize(max_compressed_size);

        uint8_t* buf = const_cast<uint8_t*>(src.data());
        uint8_t* outbuf = dst.data();

        // compress data
        size_t tosink = src_size;
        size_t output_size = 0;
        while (tosink > 0) {
            size_t sunk = 0;
            const HSE_sink_res sink_res = heatshrink_encoder_sink(encoder, buf, tosink, &sunk);
            if (sink_res != HSER_SINK_OK) {
                heatshrink_encoder_free(encoder);
                return false;
            }
            if (sunk == 0)
                // all input data processed
                break;

            tosink -= sunk;
            buf += sunk;

            size_t polled = 0;
            const HSE_poll_res poll_res = heatshrink_encoder_poll(encoder, outbuf + output_size, max_compressed_size - output_size, &polled);
            if (poll_res < 0) {
                heatshrink_encoder_free(encoder);
                return false;
            }
            output_size += polled;
        }

        // input data finished
        const HSE_finish_res finish_res = heatshrink_encoder_finish(encoder);
        if (finish_res < 0) {
            heatshrink_encoder_free(encoder);
            return false;
        }

        // poll for final output
        size_t polled = 0;
        const HSE_poll_res poll_res = heatshrink_encoder_poll(encoder, outbuf + output_size, max_compressed_size - output_size, &polled);
        if (poll_res < 0) {
            heatshrink_encoder_free(encoder);
            return false;
        }
        dst.resize(output_size + polled);
        heatshrink_encoder_free(encoder);
        break;
    }
    case ECompressionType::None:
    default:
    {
        break;
    }
    }

    return true;
}

static bool uncompress(const std::vector<uint8_t>& src, std::vector<uint8_t>& dst, ECompressionType compression_type, size_t uncompressed_size)
{
    switch (compression_type)
    {
    case ECompressionType::Deflate:
    {
        dst.clear();
        dst.reserve(uncompressed_size);

        const size_t BUFSIZE = 2048;
        std::vector<uint8_t> temp_buffer(BUFSIZE);

        z_stream strm{};
        strm.next_in = const_cast<uint8_t*>(src.data());
        strm.avail_in = (uInt)src.size();
        strm.next_out = temp_buffer.data();
        strm.avail_out = BUFSIZE;
        int res = inflateInit(&strm);
        if (res != Z_OK)
            return false;

        while (strm.avail_in > 0) {
            res = inflate(&strm, Z_NO_FLUSH);
            if (res != Z_OK && res != Z_STREAM_END) {
                inflateEnd(&strm);
                return false;
            }
            if (strm.avail_out == 0) {
                dst.insert(dst.end(), temp_buffer.data(), temp_buffer.data() + BUFSIZE);
                strm.next_out = temp_buffer.data();
                strm.avail_out = BUFSIZE;
            }
        }

        int inflate_res = Z_OK;
        while (inflate_res == Z_OK) {
            if (strm.avail_out == 0) {
                dst.insert(dst.end(), temp_buffer.data(), temp_buffer.data() + BUFSIZE);
                strm.next_out = temp_buffer.data();
                strm.avail_out = BUFSIZE;
            }
            inflate_res = inflate(&strm, Z_FINISH);
        }

        if (inflate_res != Z_STREAM_END) {
            inflateEnd(&strm);
            return false;
        }

        dst.insert(dst.end(), temp_buffer.data(), temp_buffer.data() + BUFSIZE - strm.avail_out);
        inflateEnd(&strm);
        break;
    }
    case ECompressionType::Heatshrink_11_4:
    case ECompressionType::Heatshrink_12_4:
    {
        const uint8_t window_sz = (compression_type == ECompressionType::Heatshrink_11_4) ? 11 : 12;
        const uint8_t lookahead_sz = 4;
        const uint16_t input_buffer_size = 2048;
        heatshrink_decoder* decoder = heatshrink_decoder_alloc(input_buffer_size, window_sz, lookahead_sz);
        if (decoder == nullptr)
            return false;

        dst.resize(uncompressed_size);

        uint8_t* buf = const_cast<uint8_t*>(src.data());
        uint8_t* outbuf = dst.data();

        uint32_t sunk = 0;
        uint32_t polled = 0;

        const size_t compressed_size = src.size();
        while (sunk < compressed_size) {
            size_t count = 0;
            const HSD_sink_res sink_res = heatshrink_decoder_sink(decoder, &buf[sunk], compressed_size - sunk, &count);
            if (sink_res < 0) {
                heatshrink_decoder_free(decoder);
                return false;
            }

            sunk += (uint32_t)count;

            HSD_poll_res poll_res;
            do {
                poll_res = heatshrink_decoder_poll(decoder, &outbuf[polled], uncompressed_size - polled, &count);
                if (poll_res < 0) {
                    heatshrink_decoder_free(decoder);
                    return false;
                }
                polled += (uint32_t)count;
            } while (polled < uncompressed_size && poll_res == HSDR_POLL_MORE);
        }

        const HSD_finish_res finish_res = heatshrink_decoder_finish(decoder);
        if (finish_res < 0) {
            heatshrink_decoder_free(decoder);
            return false;
        }

        heatshrink_decoder_free(decoder);
        break;
    }
    case ECompressionType::None:
    default:
    {
        break;
    }
    }

    return true;
}

EResult BaseMetadataBlock::write(FILE& file, EBlockType block_type, ECompressionType compression_type,
    Checksum& checksum) const
{
    if (encoding_type > metadata_encoding_types_count())
        return EResult::InvalidMetadataEncodingType;

    BlockHeader block_header((uint16_t)block_type, (uint16_t)compression_type, (uint32_t)0);
    std::vector<uint8_t> out_data;
    if (!raw_data.empty()) {
        // process payload encoding
        std::vector<uint8_t> uncompressed_data;
        if (!encode_metadata(raw_data, uncompressed_data, (EMetadataEncodingType)encoding_type))
            return EResult::MetadataEncodingError;
        // process payload compression
        block_header.uncompressed_size = (uint32_t)uncompressed_data.size();
        std::vector<uint8_t> compressed_data;
        if (compression_type != ECompressionType::None) {
            if (!compress(uncompressed_data, compressed_data, compression_type))
                return EResult::DataCompressionError;
            block_header.compressed_size = (uint32_t)compressed_data.size();
        }
        out_data.swap((compression_type == ECompressionType::None) ? uncompressed_data : compressed_data);
    }

    // write block header
    EResult res = block_header.write(file);
    if (res != EResult::Success)
        // propagate error
        return res;

    // write block payload
    if (!write_to_file(file, (const void*)&encoding_type, sizeof(encoding_type)))
        return EResult::WriteError;
    if (!out_data.empty()) {
        if (!write_to_file(file, (const void*)out_data.data(), out_data.size()))
            return EResult::WriteError;
    }

    if (checksum.get_type() != EChecksumType::None) {
        // update checksum with block header
        block_header.update_checksum(checksum);
        // update checksum with block payload
        checksum.append(encoding_type);
        if (!out_data.empty())
            checksum.append(out_data);
    }
    return EResult::Success;
}

EResult BaseMetadataBlock::read_data(FILE& file, const BlockHeader& block_header)
{
    const ECompressionType compression_type = (ECompressionType)block_header.compression;

    if (!read_from_file(file, (void*)&encoding_type, sizeof(encoding_type)))
        return EResult::ReadError;
    if (encoding_type > metadata_encoding_types_count())
        return EResult::InvalidMetadataEncodingType;

    std::vector<uint8_t> data;
    const size_t data_size = (compression_type == ECompressionType::None) ? block_header.uncompressed_size : block_header.compressed_size;
    if (data_size > 0) {
        data.resize(data_size);
        if (!read_from_file(file, (void*)data.data(), data_size))
            return EResult::ReadError;
    }

    std::vector<uint8_t> uncompressed_data;
    if (compression_type != ECompressionType::None) {
        if (!uncompress(data, uncompressed_data, compression_type, block_header.uncompressed_size))
            return EResult::DataUncompressionError;
    }

    if (!decode_metadata((compression_type == ECompressionType::None) ? data : uncompressed_data, raw_data, (EMetadataEncodingType)encoding_type))
        return EResult::MetadataDecodingError;

    return EResult::Success;
}

EResult FileMetadataBlock::write(FILE& file, ECompressionType compression_type, EChecksumType checksum_type) const
{
    Checksum cs(checksum_type);

    // write block header, payload
    EResult res = BaseMetadataBlock::write(file, EBlockType::FileMetadata, compression_type, cs);
    if (res != EResult::Success)
        // propagate error
        return res;

    // write block checksum
    if (checksum_type != EChecksumType::None)
        return cs.write(file);

    return EResult::Success;
}

EResult FileMetadataBlock::read_data(FILE& file, const FileHeader& file_header, const BlockHeader& block_header)
{
    // read block payload
    EResult res = BaseMetadataBlock::read_data(file, block_header);
    if (res != EResult::Success)
        // propagate error
        return res;

    const EChecksumType checksum_type = (EChecksumType)file_header.checksum_type;
    if (checksum_type != EChecksumType::None) {
        // read block checksum
        Checksum cs(checksum_type);
        res = cs.read(file);
        if (res != EResult::Success)
            // propagate error
            return res;
    }
    return EResult::Success;
}

EResult PrintMetadataBlock::write(FILE& file, ECompressionType compression_type, EChecksumType checksum_type) const
{
    Checksum cs(checksum_type);

    // write block header, payload
    EResult res = BaseMetadataBlock::write(file, EBlockType::PrintMetadata, compression_type, cs);
    if (res != EResult::Success)
        // propagate error
        return res;

    // write block checksum
    if (checksum_type != EChecksumType::None)
        return cs.write(file);

    return EResult::Success;
}

EResult PrintMetadataBlock::read_data(FILE& file, const FileHeader& file_header, const BlockHeader& block_header)
{
    // read block payload
    EResult res = BaseMetadataBlock::read_data(file, block_header);
    if (res != EResult::Success)
        // propagate error
        return res;

    const EChecksumType checksum_type = (EChecksumType)file_header.checksum_type;
    if (checksum_type != EChecksumType::None) {
        // read block checksum
        Checksum cs(checksum_type);
        res = cs.read(file);
        if (res != EResult::Success)
            // propagate error
            return res;
    }
    return EResult::Success;
}

EResult PrinterMetadataBlock::write(FILE& file, ECompressionType compression_type, EChecksumType checksum_type) const
{
    Checksum cs(checksum_type);

    // write block header, payload
    EResult res = BaseMetadataBlock::write(file, EBlockType::PrinterMetadata, compression_type, cs);
    if (res != EResult::Success)
        // propagate error
        return res;

    // write block checksum
    if (checksum_type != EChecksumType::None)
        return cs.write(file);

    return EResult::Success;
}

EResult PrinterMetadataBlock::read_data(FILE& file, const FileHeader& file_header, const BlockHeader& block_header)
{
    // read block payload
    EResult res = BaseMetadataBlock::read_data(file, block_header);
    if (res != EResult::Success)
        // propagate error
        return res;

    const EChecksumType checksum_type = (EChecksumType)file_header.checksum_type;
    if (checksum_type != EChecksumType::None) {
        // read block checksum
        Checksum cs(checksum_type);
        res = cs.read(file);
        if (res != EResult::Success)
            // propagate error
            return res;
    }
    return EResult::Success;
}

EResult ThumbnailBlock::write(FILE& file, EChecksumType checksum_type)
{
    if (params.format >= thumbnail_formats_count())
        return EResult::InvalidThumbnailFormat;
    if (params.width == 0)
        return EResult::InvalidThumbnailWidth;
    if (params.height == 0)
        return EResult::InvalidThumbnailHeight;
    if (data.size() == 0)
        return EResult::InvalidThumbnailDataSize;

    // write block header
    const BlockHeader block_header((uint16_t)EBlockType::Thumbnail, (uint16_t)ECompressionType::None, (uint32_t)data.size());
    EResult res = block_header.write(file);
    if (res != EResult::Success)
        // propagate error
        return res;

    res = params.write(file);
    if (res != EResult::Success){
        // propagate error
        return res;
    }

    if (!write_to_file(file, (const void*)data.data(), data.size()))
        return EResult::WriteError;

    if (checksum_type != EChecksumType::None) {
        Checksum cs(checksum_type);
        // update checksum with block header
        block_header.update_checksum(cs);
        // update checksum with block payload
        update_checksum(cs);
        // write block checksum
        res = cs.write(file);
        if (res != EResult::Success)
            // propagate error
            return res;
    }
    return EResult::Success;
}

EResult ThumbnailBlock::read_data(FILE& file, const FileHeader& file_header, const BlockHeader& block_header)
{
    // read block payload
    EResult res = params.read(file);
    if (res != EResult::Success)
        // propagate error
        return res;
    if (params.format >= thumbnail_formats_count())
        return EResult::InvalidThumbnailFormat;
    if (params.width == 0)
        return EResult::InvalidThumbnailWidth;
    if (params.height == 0)
        return EResult::InvalidThumbnailHeight;
    if (block_header.uncompressed_size == 0)
        return EResult::InvalidThumbnailDataSize;

    data.resize(block_header.uncompressed_size);
    if (!read_from_file(file, (void*)data.data(), block_header.uncompressed_size))
        return EResult::ReadError;

    const EChecksumType checksum_type = (EChecksumType)file_header.checksum_type;
    if (checksum_type != EChecksumType::None) {
        // read block checksum
        Checksum cs(checksum_type);
        const EResult res = cs.read(file);
        if (res != EResult::Success)
            // propagate error
            return res;
    }
    return EResult::Success;
}

void ThumbnailBlock::update_checksum(Checksum& checksum) const
{
    checksum.append(params.format);
    checksum.append(params.width);
    checksum.append(params.height);
    checksum.append(data);
}

EResult GCodeBlock::write(FILE& file, ECompressionType compression_type, EChecksumType checksum_type) const
{
    if (encoding_type > gcode_encoding_types_count())
        return EResult::InvalidGCodeEncodingType;

    BlockHeader block_header((uint16_t)EBlockType::GCode, (uint16_t)compression_type, (uint32_t)0);
    std::vector<uint8_t> out_data;
    if (!raw_data.empty()) {
        // process payload encoding
        std::vector<uint8_t> uncompressed_data;
        if (!encode_gcode(raw_data, uncompressed_data, (EGCodeEncodingType)encoding_type))
            return EResult::GCodeEncodingError;
        // process payload compression
        block_header.uncompressed_size = (uint32_t)uncompressed_data.size();
        std::vector<uint8_t> compressed_data;
        if (compression_type != ECompressionType::None) {
            if (!compress(uncompressed_data, compressed_data, compression_type))
                return EResult::DataCompressionError;
            block_header.compressed_size = (uint32_t)compressed_data.size();
        }
        out_data.swap((compression_type == ECompressionType::None) ? uncompressed_data : compressed_data);
    }

    // write block header
    EResult res = block_header.write(file);
    if (res != EResult::Success)
        // propagate error
        return res;

    // write block payload
    if (!write_to_file(file, (const void*)&encoding_type, sizeof(encoding_type)))
        return EResult::WriteError;
    if (!out_data.empty()) {
        if (!write_to_file(file, (const void*)out_data.data(), out_data.size()))
            return EResult::WriteError;
    }

    // write checksum
    if (checksum_type != EChecksumType::None) {
        Checksum cs(checksum_type);
        // update checksum with block header
        block_header.update_checksum(cs);
        // update checksum with block payload
        cs.append(encode((const void*)&encoding_type, sizeof(encoding_type)));
        if (!out_data.empty())
            cs.append(out_data);
        res = cs.write(file);
        if (res != EResult::Success)
            // propagate error
            return res;
    }
    return EResult::Success;
}

EResult GCodeBlock::read_data(FILE& file, const FileHeader& file_header, const BlockHeader& block_header)
{
    const ECompressionType compression_type = (ECompressionType)block_header.compression;

    if (!read_from_file(file, (void*)&encoding_type, sizeof(encoding_type)))
        return EResult::ReadError;
    if (encoding_type > gcode_encoding_types_count())
        return EResult::InvalidGCodeEncodingType;

    std::vector<uint8_t> data;
    const size_t data_size = (compression_type == ECompressionType::None) ? block_header.uncompressed_size : block_header.compressed_size;
    if (data_size > 0) {
        data.resize(data_size);
        if (!read_from_file(file, (void*)data.data(), data_size))
            return EResult::ReadError;
    }

    std::vector<uint8_t> uncompressed_data;
    if (compression_type != ECompressionType::None) {
        if (!uncompress(data, uncompressed_data, compression_type, block_header.uncompressed_size))
            return EResult::DataUncompressionError;
    }

    if (!decode_gcode((compression_type == ECompressionType::None) ? data : uncompressed_data, raw_data, (EGCodeEncodingType)encoding_type))
        return EResult::GCodeDecodingError;

    const EChecksumType checksum_type = (EChecksumType)file_header.checksum_type;
    if (checksum_type != EChecksumType::None) {
        // read block checksum
        Checksum cs(checksum_type);
        const EResult res = cs.read(file);
        if (res != EResult::Success)
            // propagate error
            return res;
    }
    return EResult::Success;
}

EResult SlicerMetadataBlock::write(FILE& file, ECompressionType compression_type, EChecksumType checksum_type) const
{
    Checksum cs(checksum_type);

    // write block header, payload
    EResult res = BaseMetadataBlock::write(file, EBlockType::SlicerMetadata, compression_type, cs);
    if (res != EResult::Success)
        // propagate error
        return res;

    // write block checksum
    if (checksum_type != EChecksumType::None)
        return cs.write(file);

    return EResult::Success;
}

EResult SlicerMetadataBlock::read_data(FILE& file, const FileHeader& file_header, const BlockHeader& block_header)
{
    // read block payload
    EResult res = BaseMetadataBlock::read_data(file, block_header);
    if (res != EResult::Success)
        // propagate error
        return res;

    const EChecksumType checksum_type = (EChecksumType)file_header.checksum_type;
    if (checksum_type != EChecksumType::None) {
        // read block checksum
        Checksum cs(checksum_type);
        res = cs.read(file);
        if (res != EResult::Success)
            // propagate error
            return res;
    }
    return EResult::Success;
}

bool Binarizer::is_enabled() const { return m_enabled; }
void Binarizer::set_enabled(bool enable) { m_enabled = enable; }
BinaryData& Binarizer::get_binary_data() { return m_binary_data; }
const BinaryData& Binarizer::get_binary_data() const { return m_binary_data; }
size_t Binarizer::get_max_gcode_cache_size() const { return m_gcode_cache_size; }
void Binarizer::set_max_gcode_cache_size(size_t size) { m_gcode_cache_size = size; }

EResult Binarizer::initialize(FILE& file, const BinarizerConfig& config)
{
    if (!m_enabled)
        return EResult::Success;

    m_file = &file;
    m_config = config;

    // save header
    FileHeader file_header;
    file_header.checksum_type = (uint16_t)m_config.checksum;
    EResult res = file_header.write(*m_file);
    if (res != EResult::Success)
        // propagate error
        return res;

    // save file metadata block
    m_binary_data.file_metadata.encoding_type = (uint16_t)config.metadata_encoding;
    res = m_binary_data.file_metadata.write(*m_file, m_config.compression.file_metadata, m_config.checksum);
    if (res != EResult::Success)
        // propagate error
        return res;

    // save printer metadata block
    m_binary_data.printer_metadata.encoding_type = (uint16_t)config.metadata_encoding;
    res = m_binary_data.printer_metadata.write(*m_file, m_config.compression.printer_metadata, m_config.checksum);
    if (res != EResult::Success)
        // propagate error
        return res;

    // save thumbnail blocks
    for (ThumbnailBlock& block : m_binary_data.thumbnails) {
        res = block.write(*m_file, m_config.checksum);
        if (res != EResult::Success)
            // propagate error
            return res;
    }

    // save print metadata block
    m_binary_data.print_metadata.encoding_type = (uint16_t)config.metadata_encoding;
    res = m_binary_data.print_metadata.write(*m_file, m_config.compression.print_metadata, m_config.checksum);
    if (res != EResult::Success)
        // propagate error
        return res;

    // save slicer metadata block
    m_binary_data.slicer_metadata.encoding_type = (uint16_t)config.metadata_encoding;
    res = m_binary_data.slicer_metadata.write(*m_file, m_config.compression.slicer_metadata, m_config.checksum);
    if (res != EResult::Success)
        // propagate error
        return res;

    return EResult::Success;
}

static EResult write_gcode_block(FILE& file, const std::string& raw_data, const BinarizerConfig& config)
{
    GCodeBlock block;
    block.encoding_type = (uint16_t)config.gcode_encoding;
    block.raw_data = raw_data;
    return block.write(file, config.compression.gcode, config.checksum);
}

EResult Binarizer::append_gcode(const std::string& gcode)
{
    if (gcode.empty())
        return EResult::Success;

    assert(m_file != nullptr);
    if (m_file == nullptr)
        return EResult::WriteError;

    auto it_begin = gcode.begin();
    do {
        const size_t begin_pos = std::distance(gcode.begin(), it_begin);
        const size_t end_line_pos = gcode.find_first_of('\n', begin_pos);
        if (end_line_pos == std::string::npos)
            return EResult::WriteError;

        const size_t line_size = 1 + end_line_pos - begin_pos;
        if (line_size + m_gcode_cache.length() > m_gcode_cache_size) {
            if (!m_gcode_cache.empty()) {
                const EResult res = write_gcode_block(*m_file, m_gcode_cache, m_config);
                if (res != EResult::Success)
                    // propagate error
                    return res;
                m_gcode_cache.clear();
            }
        }

        if (line_size > m_gcode_cache_size)
            return EResult::WriteError;

        m_gcode_cache.insert(m_gcode_cache.end(), it_begin, it_begin + line_size);
        it_begin += line_size;
    } while (it_begin != gcode.end());

    return EResult::Success;
}

EResult Binarizer::finalize()
{
    if (!m_enabled)
        return EResult::Success;

    // save gcode cache, if not empty
    if (!m_gcode_cache.empty()) {
        const EResult res = write_gcode_block(*m_file, m_gcode_cache, m_config);
        if (res != EResult::Success)
            // propagate error
            return res;
    }

    return EResult::Success;
}

}} // namespace bgcode

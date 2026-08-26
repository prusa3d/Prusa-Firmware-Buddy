#include "binarize.hpp"
#include "meatpack.hpp"

#include "core/core_impl.hpp"

extern "C" {
#include <heatshrink/heatshrink_encoder.h>
#include <heatshrink/heatshrink_decoder.h>
}
#include <zlib.h>

#include <algorithm>
#include <cstring>
#include <cassert>

namespace bgcode {

using namespace core;

namespace binarize {

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

void update_checksum(Checksum& checksum, const ThumbnailBlock &th)
{
    checksum.append(th.params.format);
    checksum.append(th.params.width);
    checksum.append(th.params.height);
    checksum.append(th.data);
}

static std::vector<uint8_t> encode(const std::byte* data, size_t data_size)
{
    std::vector<uint8_t> ret(data_size);
    memcpy(ret.data(), data, data_size);
    return ret;
}

// 16 MiB. Two things set this. It is ~150x the largest block a slicer emits in
// practice (the biggest observed in a sample of real files was a ~108 KB
// thumbnail), so it cannot reject plausible content. And peak memory is roughly
// three times the limit rather than equal to it -- measured 200 MB peak RSS for a
// single block at a 64 MiB limit -- because the compressed buffer, the
// decompressed buffer and the decoded result are live at the same time. The
// limit therefore has to be chosen against 3x, not 1x.
static size_t g_max_block_data_size = 16 * 1024 * 1024;

size_t get_max_block_data_size() { return g_max_block_data_size; }
void set_max_block_data_size(size_t size) { g_max_block_data_size = size; }

// Both header size fields must be checked before either is used to allocate:
// compressed_size drives a resize() before any payload is read, and
// uncompressed_size drives an eager resize() inside the heatshrink decoder.
static bool block_sizes_within_limit(const BlockHeader& block_header)
{
    const size_t limit = g_max_block_data_size;
    return block_header.uncompressed_size <= limit && block_header.compressed_size <= limit;
}

static uint16_t metadata_encoding_types_count() { return 1 + (uint16_t)EMetadataEncodingType::JSON; }
static uint16_t thumbnail_formats_count()       { return 1 + (uint16_t)EThumbnailFormat::QOI; }
static uint16_t gcode_encoding_types_count()    { return 1 + (uint16_t)EGCodeEncodingType::MeatPackComments; }

static bool encode_metadata(const std::vector<std::pair<std::string, std::string>>& src, std::vector<uint8_t>& dst,
    EMetadataEncodingType encoding_type)
{

    switch (encoding_type)
    {
    case EMetadataEncodingType::INI:
    {
        for (const auto& [key, value] : src) {
            dst.insert(dst.end(), key.begin(), key.end());
            dst.emplace_back('=');
            dst.insert(dst.end(), value.begin(), value.end());
            dst.emplace_back('\n');
        }
        break;
    }
    case EMetadataEncodingType::JSON:
    {
        if (src.size() != 1)
            return false;
        const auto& [k, v] = src.at(0);
        dst.insert(dst.end(), v.begin(), v.end());
        break;
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
        // The advance must not depend on finding '=': a line without it would
        // otherwise never move the iterator and the loop could not terminate.
        // Nor may the iterator be incremented past end(), which would leave the
        // next dereference scanning beyond the buffer.
        auto begin_it = src.begin();
        while (begin_it != src.end()) {
            const auto end_it = std::find(begin_it, src.end(), '\n');
            const std::string item(begin_it, end_it);
            const size_t pos = item.find_first_of('=');
            if (pos != std::string::npos) {
                dst.emplace_back(item.substr(0, pos), item.substr(pos + 1));
            }
            begin_it = (end_it == src.end()) ? end_it : end_it + 1;
        }
        break;
    }
    case EMetadataEncodingType::JSON:
    {
        std::string v;
        v.insert(v.end(), src.begin(), src.end());
        dst.emplace_back("", v);
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
            // The newline belongs to the line, but a final line need not have one:
            // incrementing unconditionally would step past end(), and the outer
            // condition would then still hold, leaving the next scan reading beyond
            // the buffer.
            if (end_it != src.end())
                ++end_it;
            const std::string line(begin_it, end_it);
            if (!binarizer.binarize_line(line, dst))
                return false;
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

static bool compress(std::vector<uint8_t>& src, std::vector<uint8_t>& dst, ECompressionType compression_type)
{
    switch (compression_type)
    {
    case ECompressionType::Deflate:
    {
        dst.clear();

        const size_t BUFSIZE = 2048;
        std::vector<uint8_t> temp_buffer(BUFSIZE);

        z_stream strm{};
        strm.next_in = static_cast<Bytef*>(src.data());
        strm.avail_in = static_cast<uInt>(src.size());
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

        // Heatshrink can expand: every literal costs a tag bit, so the worst case is
        // about 9/8 of the input, plus whatever the final flush emits and the padding
        // to a whole byte. A pure ratio is not enough because it rounds down to
        // nothing for tiny inputs -- a 1-byte payload got a 1-byte buffer, which
        // cannot hold even one tagged literal.
        const size_t src_size = src.size();
        const size_t max_compressed_size = src_size + (src_size >> 2) + 64;
        dst.resize(max_compressed_size);

        uint8_t* buf = src.data();
        uint8_t* outbuf = dst.data();

        // compress data
        size_t tosink = src_size;
        size_t output_size = 0;

        // Drain until the encoder says it is empty, and treat a full output
        // buffer as an error rather than silently truncating.
        auto drain = [&]() {
            HSE_poll_res poll_res;
            do {
                size_t polled = 0;
                poll_res = heatshrink_encoder_poll(encoder, outbuf + output_size, max_compressed_size - output_size, &polled);
                if (poll_res < 0)
                    return false;
                output_size += polled;
                if (poll_res == HSER_POLL_MORE && output_size == max_compressed_size)
                    return false;
            } while (poll_res == HSER_POLL_MORE);
            return true;
        };

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

            if (!drain()) {
                heatshrink_encoder_free(encoder);
                return false;
            }
        }

        // input data finished
        const HSE_finish_res finish_res = heatshrink_encoder_finish(encoder);
        if (finish_res < 0) {
            heatshrink_encoder_free(encoder);
            return false;
        }

        // Poll for the final output; the flush is where more than one poll is
        // most likely to be needed.
        if (!drain()) {
            heatshrink_encoder_free(encoder);
            return false;
        }
        dst.resize(output_size);
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
    // Defensive: the callers above check the header, but the heatshrink branch
    // below resizes to uncompressed_size in one go before decoding anything, so
    // this must not be reachable with an unbounded value from any caller.
    if (uncompressed_size > g_max_block_data_size)
        return false;

    switch (compression_type)
    {
    case ECompressionType::Deflate:
    {
        dst.clear();
        // inflate cannot make progress from zero input; an empty stream
        // declaring zero output decompresses to nothing.
        if (src.empty() && uncompressed_size == 0)
            break;
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
            // total_in/total_out, not avail_in/avail_out: the flush block below
            // always resets avail_out to BUFSIZE whenever it runs, so avail_out alone
            // cannot show whether the preceding inflate() call made progress. total_*
            // is cumulative and monotonic and nothing here touches it.
            const uLong total_in_before = strm.total_in;
            const uLong total_out_before = strm.total_out;

            res = inflate(&strm, Z_NO_FLUSH);
            if (res != Z_OK && res != Z_STREAM_END) {
                inflateEnd(&strm);
                return false;
            }
            if (strm.avail_out == 0) {
                // The declared uncompressed_size is only a reserve() hint above; a
                // header may understate it while the stream expands without bound,
                // so the real output has to be bounded here.
                if (dst.size() + BUFSIZE > g_max_block_data_size) {
                    inflateEnd(&strm);
                    return false;
                }
                dst.insert(dst.end(), temp_buffer.data(), temp_buffer.data() + BUFSIZE);
                strm.next_out = temp_buffer.data();
                strm.avail_out = BUFSIZE;
            }

            // Trailing bytes after a complete stream leave avail_in > 0 with nothing
            // left to decode. Breaking here relies on the Z_FINISH loop below being a
            // harmless no-op on an already-finished stream: zlib returns Z_STREAM_END
            // again rather than erroring, which its source does but its documentation
            // does not promise.
            if (res == Z_STREAM_END)
                break;
            // Without this guard a stream that neither consumes input nor produces
            // output would spin here forever.
            if (strm.total_in == total_in_before && strm.total_out == total_out_before) {
                inflateEnd(&strm);
                return false;
            }
        }

        int inflate_res = Z_OK;
        while (inflate_res == Z_OK) {
            if (strm.avail_out == 0) {
                if (dst.size() + BUFSIZE > g_max_block_data_size) {
                    inflateEnd(&strm);
                    return false;
                }
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

        // The final flush shares the same unbounded-header risk as the flush block
        // above it, so it needs the same cap check rather than being the one insert
        // in this function that trusts uncompressed_size.
        if (dst.size() + (BUFSIZE - strm.avail_out) > g_max_block_data_size) {
            inflateEnd(&strm);
            return false;
        }
        dst.insert(dst.end(), temp_buffer.data(), temp_buffer.data() + BUFSIZE - strm.avail_out);

        // uncompressed_size is only a reserve() hint to zlib, not something it
        // enforces, so a header can under- or over-state it and inflate() will
        // happily produce a different amount of output. Require an exact match
        // rather than trusting the declared size.
        if (dst.size() != uncompressed_size) {
            inflateEnd(&strm);
            return false;
        }

        inflateEnd(&strm);
        break;
    }
    case ECompressionType::Heatshrink_11_4:
    case ECompressionType::Heatshrink_12_4:
    {
        // A payload that decodes to nothing is compressed from nothing, so the two
        // declared sizes must both be zero; anything else is a header that contradicts
        // itself and is rejected rather than silently dropping the payload.
        if (uncompressed_size == 0) {
            if (!src.empty())
                return false;
            dst.clear();
            break;
        }

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
            const uint32_t sunk_before = sunk;
            const uint32_t polled_before = polled;

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

            // Neither call advanced: dst is full while the decoder's input buffer is
            // also full, so the declared uncompressed_size cannot hold the stream and
            // the file is malformed. Rejecting rather than breaking matters here --
            // breaking would leave dst sized to uncompressed_size but only partly
            // filled and still report success.
            if (sunk == sunk_before && polled == polled_before) {
                heatshrink_decoder_free(decoder);
                return false;
            }
        }

        // Unlike Deflate, heatshrink's output buffer is sized to uncompressed_size
        // up front, so a stream that ends early leaves the tail zero-padded rather
        // than failing: the loop above simply runs out of input with polled short
        // of the declared size. That has to be caught here rather than relying on
        // the finish check below, which is about pending output, not missing output.
        if (polled != uncompressed_size) {
            heatshrink_decoder_free(decoder);
            return false;
        }

        const HSD_finish_res finish_res = heatshrink_decoder_finish(decoder);
        // HSDR_FINISH_DONE is 0 and HSDR_FINISH_MORE is 1, so `finish_res < 0` lets
        // "there is still output pending" through as success. Pending output here
        // means polled already reached uncompressed_size while the decoder still
        // has more to give, so the real stream is longer than declared.
        if (finish_res != HSDR_FINISH_DONE) {
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


// write block header and data in encoded format
core::EResult write(const BaseMetadataBlock &block, FILE& file, core::EBlockType block_type, core::ECompressionType compression_type, core::Checksum &checksum)
{
    if (block.encoding_type > metadata_encoding_types_count())
        return EResult::InvalidMetadataEncodingType;

    BlockHeader block_header((uint16_t)block_type, (uint16_t)compression_type, (uint32_t)0);
    std::vector<uint8_t> out_data;
    if (!block.raw_data.empty()) {
        // process payload encoding
        std::vector<uint8_t> uncompressed_data;
        if (!encode_metadata(block.raw_data, uncompressed_data, (EMetadataEncodingType)block.encoding_type))
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
    if (!write_to_file(file, &block.encoding_type, sizeof(block.encoding_type)))
        return EResult::WriteError;
    if (!out_data.empty()) {
        if (!write_to_file(file, out_data.data(), out_data.size()))
            return EResult::WriteError;
    }

    if (checksum.get_type() != EChecksumType::None) {
        // update checksum with block header
        update_checksum(checksum, block_header);
        // update checksum with block payload
        checksum.append(block.encoding_type);
        if (!out_data.empty())
            checksum.append(static_cast<unsigned char*>(out_data.data()), out_data.size());
    }

    return EResult::Success;
}

EResult BaseMetadataBlock::read_data(FILE& file, const BlockHeader& block_header)
{
    const ECompressionType compression_type = (ECompressionType)block_header.compression;

    if (!block_sizes_within_limit(block_header))
        return EResult::BlockTooLarge;

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
    EResult res = binarize::write(*this, file, EBlockType::FileMetadata, compression_type, cs);
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
    EResult res = binarize::write(*this, file, EBlockType::PrintMetadata, compression_type, cs);
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
    EResult res = binarize::write(*this, file, EBlockType::PrinterMetadata, compression_type, cs);
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
    BlockHeader block_header((uint16_t)EBlockType::Thumbnail, (uint16_t)ECompressionType::None, (uint32_t)data.size());
    EResult res = block_header.write(file);
    if (res != EResult::Success)
        // propagate error
        return res;

    res = params.write(file);
    if (res != EResult::Success){
        // propagate error
        return res;
    }

    if (!write_to_file(file, data.data(), data.size()))
        return EResult::WriteError;

    if (checksum_type != EChecksumType::None) {
        Checksum cs(checksum_type);
        // update checksum with block header
        update_checksum(cs, block_header);
        // update checksum with block payload
        update_checksum(cs, *this);
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
    if (!block_sizes_within_limit(block_header))
        return EResult::BlockTooLarge;

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
    if (!write_to_file(file, &encoding_type, sizeof(encoding_type)))
        return EResult::WriteError;
    if (!out_data.empty()) {
        if (!write_to_file(file, out_data.data(), out_data.size()))
            return EResult::WriteError;
    }

    // write checksum
    if (checksum_type != EChecksumType::None) {
        Checksum cs(checksum_type);
        // update checksum with block header
        update_checksum(cs, block_header);
        // update checksum with block payload
        std::vector<uint8_t> data_to_encode =
            encode(reinterpret_cast<const std::byte*>(&encoding_type), sizeof(encoding_type));
        cs.append(data_to_encode.data(), data_to_encode.size());
        if (!out_data.empty())
            cs.append(static_cast<unsigned char *>(out_data.data()), out_data.size());
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

    if (!block_sizes_within_limit(block_header))
        return EResult::BlockTooLarge;

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
    EResult res = binarize::write(*this, file, EBlockType::SlicerMetadata, compression_type, cs);
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

void Slicer3MetadataBlock::set_json(std::string_view json)
{
    if (raw_data.empty())
        raw_data.emplace_back("", json);
    else
        raw_data.front().second = json;
}

const std::string& Slicer3MetadataBlock::json() const
{
    if (raw_data.empty()) {
        static std::string EMPTY;
        return EMPTY;
    }
    return raw_data.front().second;
}


EResult Slicer3MetadataBlock::write(FILE& file, ECompressionType compression_type, EChecksumType checksum_type) const
{
    Checksum cs(checksum_type);

    // write block header, payload
    EResult res = binarize::write(*this, file, EBlockType::SlicerMetadata, compression_type, cs);
    if (res != EResult::Success)
        // propagate error
        return res;

    // write block checksum
    if (checksum_type != EChecksumType::None)
        return cs.write(file);

    return EResult::Success;
}

EResult Slicer3MetadataBlock::read_data(FILE& file, const FileHeader& file_header, const BlockHeader& block_header)
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

EPeekSlicerMetadataResult peek_slicer_metadata_block(FILE& file, const core::BlockHeader& block_header)
{
    if (EBlockType{block_header.type} != EBlockType::SlicerMetadata)
        return EPeekSlicerMetadataResult::OtherBlockFound;
    decltype(Slicer3MetadataBlock::encoding_type) encoding_type;
    auto pos = ftell(&file);
    if (pos < 0)
        return EPeekSlicerMetadataResult::ReadError;
    if (!read_from_file(file, (void*)&encoding_type, sizeof(encoding_type)))
        return EPeekSlicerMetadataResult::ReadError;
    // rewind back the file like we didn't read it
    fseek(&file, pos, SEEK_SET);
    return EMetadataEncodingType{encoding_type} == EMetadataEncodingType::JSON ? EPeekSlicerMetadataResult::Slicer3MetadataFound : EPeekSlicerMetadataResult::SlicerMetadataFound;
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

    // save file metadata block, if present
    if (!m_binary_data.file_metadata.raw_data.empty()) {
        m_binary_data.file_metadata.encoding_type = (uint16_t)config.metadata_encoding;
        res = m_binary_data.file_metadata.write(*m_file, m_config.compression.file_metadata, m_config.checksum);
        if (res != EResult::Success)
            // propagate error
            return res;
    }

    // save printer metadata block
    if (m_binary_data.printer_metadata.raw_data.empty())
        return EResult::MissingPrinterMetadata;
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
    if (m_binary_data.print_metadata.raw_data.empty())
        return EResult::MissingPrintMetadata;
    m_binary_data.print_metadata.encoding_type = (uint16_t)config.metadata_encoding;
    res = m_binary_data.print_metadata.write(*m_file, m_config.compression.print_metadata, m_config.checksum);
    if (res != EResult::Success)
        // propagate error
        return res;

    // save slicer metadata blocks
    if (m_binary_data.slicer_metadata.raw_data.empty() && m_binary_data.slicer3_metadata.raw_data.empty())
        return EResult::MissingSlicerMetadata;

    if (!m_binary_data.slicer_metadata.raw_data.empty()) {
        m_binary_data.slicer_metadata.encoding_type = (uint16_t)config.metadata_encoding;
        res = m_binary_data.slicer_metadata.write(*m_file, m_config.compression.slicer_metadata, m_config.checksum);
        if (res != EResult::Success) {
            // propagate error
            return res;
        }
    }

    if (!m_binary_data.slicer3_metadata.raw_data.empty()) {
        res = m_binary_data.slicer3_metadata.write(*m_file, m_config.compression.slicer3_metadata, m_config.checksum);
        if (res != EResult::Success) {
            // propagate error
            return res;
        }
    }

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

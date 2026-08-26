#pragma once

#include <catch2/catch_test_macros.hpp>

#include "binarize/binarize.hpp"
#include "core/core.hpp"

#include <cstdio>
#include <string>

// Shared helpers for the binarize tests: crafting single-block files (including
// malformed ones), writing blocks, reading them back and round-tripping.
namespace bgcode_test {

using namespace bgcode::core;
using namespace bgcode::binarize;

// RAII wrapper over std::tmpfile(); closes on destruction.
class ScopedFile
{
public:
    ScopedFile() : m_file(std::tmpfile()) { REQUIRE(m_file != nullptr); }
    ~ScopedFile() { if (m_file != nullptr) std::fclose(m_file); }

    ScopedFile(ScopedFile&& other) noexcept : m_file(other.m_file) { other.m_file = nullptr; }
    ScopedFile& operator=(ScopedFile&& other) noexcept {
        if (this != &other) {
            if (m_file != nullptr) std::fclose(m_file);
            m_file = other.m_file;
            other.m_file = nullptr;
        }
        return *this;
    }
    ScopedFile(const ScopedFile&) = delete;
    ScopedFile& operator=(const ScopedFile&) = delete;

    FILE* get() const { return m_file; }
    FILE& operator*() const { return *m_file; }

private:
    FILE* m_file{ nullptr };
};

// Craft a FileHeader plus one block written verbatim from raw bytes, with the
// given (possibly dishonest) header sizes; rewound, ready to read. Bytes is any
// container exposing data()/size()/empty() (std::vector<uint8_t> or std::string).
template <typename Bytes>
ScopedFile make_block_file(EBlockType block_type, ECompressionType compression, uint16_t encoding,
                           uint32_t uncompressed_size, uint32_t compressed_size, const Bytes& payload)
{
    ScopedFile file;

    const FileHeader file_header;
    REQUIRE(file_header.write(*file) == EResult::Success);

    BlockHeader block_header(static_cast<uint16_t>(block_type), static_cast<uint16_t>(compression),
                             uncompressed_size, compressed_size);
    REQUIRE(block_header.write(*file) == EResult::Success);

    REQUIRE(std::fwrite(&encoding, sizeof(encoding), 1, file.get()) == 1);
    if (!payload.empty())
        REQUIRE(std::fwrite(payload.data(), 1, payload.size(), file.get()) == payload.size());

    std::rewind(file.get());
    return file;
}

// Write a FileHeader then the block; returns the block.write() result without
// rewinding, so a caller can also check for an expected write failure.
template <typename Block>
EResult write_block_to(FILE& file, const Block& block, ECompressionType compression,
                       EChecksumType checksum = EChecksumType::None)
{
    const FileHeader file_header;
    REQUIRE(file_header.write(file) == EResult::Success);
    return block.write(file, compression, checksum);
}

// Read a FileHeader and first block header, then that block's data into
// out_block; returns the read_data() result.
template <typename Block>
EResult read_first_block(FILE& file, Block& out_block)
{
    FileHeader file_header;
    REQUIRE(read_header(file, file_header, nullptr) == EResult::Success);
    BlockHeader block_header;
    REQUIRE(read_next_block_header(file, file_header, block_header) == EResult::Success);
    return out_block.read_data(file, file_header, block_header);
}

// Write a block object and read it back into out_block; the write must succeed.
template <typename Block>
EResult roundtrip_block(const Block& block, ECompressionType compression, Block& out_block,
                        EChecksumType checksum = EChecksumType::None)
{
    ScopedFile file;
    REQUIRE(write_block_to(*file, block, compression, checksum) == EResult::Success);
    std::rewind(file.get());
    return read_first_block(*file, out_block);
}

// A GCodeBlock carrying the given encoding and raw data.
inline GCodeBlock make_gcode_block(EGCodeEncodingType encoding, const std::string& raw_data)
{
    GCodeBlock block;
    block.encoding_type = static_cast<uint16_t>(encoding);
    block.raw_data = raw_data;
    return block;
}

} // namespace bgcode_test

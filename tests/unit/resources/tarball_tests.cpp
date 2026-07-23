/// @file
#include <resources/tarball_internal.hpp>

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <string>
#include <string_view>

using namespace buddy::resources::tarball;
using namespace std::string_view_literals;

namespace {

using Block = std::array<uint8_t, block_size>;

constexpr char root[] = "/system/res";

// Fluent builder for a 512-byte ustar header block. Only the fields that are set get
// written; everything else stays zero. with_checksum() sums over the current contents, so
// chain it after the other fields.
class BlockBuilder {
public:
    BlockBuilder &with_name(const char *name) {
        write_field(field_offset_name, name);
        return *this;
    }

    BlockBuilder &with_prefix(const char *prefix) {
        write_field(field_offset_prefix, prefix);
        return *this;
    }

    // Compute and store the POSIX ustar checksum: the sum of all 512 bytes with the checksum
    // field counted as 8 spaces, written back as zero-padded octal followed by a NUL and a
    // space (the conventional ustar encoding).
    BlockBuilder &with_checksum() {
        uint32_t sum = 0;
        for (size_t i = 0; i < block_size; i++) {
            sum += (i >= field_offset_chksum && i < field_offset_chksum + field_length_chksum) ? ' ' : block[i];
        }
        char field[field_length_chksum];
        std::snprintf(field, sizeof(field), "%06o", sum); // 6 octal digits + NUL
        std::memcpy(block.data() + field_offset_chksum, field, field_length_chksum - 1); // includes the trailing NUL
        block[field_offset_chksum + field_length_chksum - 1] = ' ';
        return *this;
    }

    Block build() const {
        return block;
    }

private:
    // Write a NUL-padded string at `offset` (no terminator past the written bytes; the field
    // width is the caller's concern).
    void write_field(size_t offset, const char *str) {
        std::memcpy(block.data() + offset, str, std::strlen(str));
    }

    Block block {};
};

} // namespace

TEST_CASE("parse_octal", "[tarball]") {
    auto parse = [](const char *field, size_t len) -> std::optional<uint32_t> {
        uint32_t out = 0xdead;
        if (!parse_octal(reinterpret_cast<const uint8_t *>(field), len, out)) {
            return std::nullopt;
        }
        return out;
    };

    SECTION("zero-padded value") {
        REQUIRE(parse("0000755", 7) == 0755u);
    }

    SECTION("leading and trailing padding") {
        REQUIRE(parse("   0755 ", 8) == 0755u);
        REQUIRE(parse("0755\0\0\0", 7) == 0755u);
    }

    SECTION("all padding parses as zero") {
        REQUIRE(parse("        ", 8) == 0u);
        REQUIRE(parse("\0\0\0\0", 4) == 0u);
    }

    SECTION("full-width with no terminator") {
        REQUIRE(parse("755", 3) == 0755u);
    }

    SECTION("stray non-octal digit is rejected") {
        REQUIRE(parse("8", 1) == std::nullopt);
        REQUIRE(parse("129", 3) == std::nullopt);
        REQUIRE(parse("12x", 3) == std::nullopt);
    }
}

TEST_CASE("checksum_ok", "[tarball]") {
    SECTION("valid header") {
        Block block = BlockBuilder {}.with_name("/foo").with_checksum().build();
        REQUIRE(checksum_ok(block.data()));
    }

    SECTION("flipped content byte invalidates the checksum") {
        Block block = BlockBuilder {}.with_name("/foo").with_checksum().build();
        block[field_offset_name] = '/' + 1; // changes the sum without touching the stored value
        REQUIRE_FALSE(checksum_ok(block.data()));
    }

    SECTION("non-octal stored checksum is rejected") {
        Block block = BlockBuilder {}.with_name("/foo").with_checksum().build();
        block[field_offset_chksum] = 'Z';
        REQUIRE_FALSE(checksum_ok(block.data()));
    }

    SECTION("all-zero end-of-archive block is not a valid header") {
        Block block {};
        REQUIRE_FALSE(checksum_ok(block.data()));
    }
}

TEST_CASE("padded_to_block", "[tarball]") {
    REQUIRE(padded_to_block(0) == 0);
    REQUIRE(padded_to_block(1) == 512);
    REQUIRE(padded_to_block(100) == 512);
    REQUIRE(padded_to_block(511) == 512);
    REQUIRE(padded_to_block(512) == 512);
    REQUIRE(padded_to_block(513) == 1024);
}

TEST_CASE("build_path - valid names", "[tarball][security]") {
    SECTION("regular file") {
        Block block = BlockBuilder {}.with_name("/a/b.txt").build();
        REQUIRE(build_path(block, root, /*is_dir=*/false) == "/system/res/a/b.txt"sv);
    }

    SECTION("directory drops the trailing slash") {
        Block block = BlockBuilder {}.with_name("/a/").build();
        REQUIRE(build_path(block, root, /*is_dir=*/true) == "/system/res/a"sv);
    }

    SECTION("name filling the full 100-byte field") {
        std::string name = "/" + std::string(99, 'a'); // exactly 100 bytes, no in-field NUL
        REQUIRE(name.size() == field_length_name);
        Block block = BlockBuilder {}.with_name(name.c_str()).build();
        REQUIRE(build_path(block, root, /*is_dir=*/false) == "/system/res" + name);
    }
}

TEST_CASE("build_path - rejects unsafe names", "[tarball][security]") {
    SECTION("`..` in the middle of a path is rejected") {
        Block block = BlockBuilder {}.with_name("/a/../b").build();
        REQUIRE(build_path(block, root, /*is_dir=*/false) == nullptr);
    }

    SECTION("`..` as the whole rooted name is rejected") {
        Block block = BlockBuilder {}.with_name("/..").build();
        REQUIRE(build_path(block, root, /*is_dir=*/false) == nullptr);
    }

    SECTION("trailing `..` directory is rejected") {
        Block block = BlockBuilder {}.with_name("/a/..").build();
        REQUIRE(build_path(block, root, /*is_dir=*/true) == nullptr);
    }

    SECTION("`..` as a substring of a component is allowed") {
        Block block = BlockBuilder {}.with_name("/a..b/c").build();
        REQUIRE(build_path(block, root, /*is_dir=*/false) == "/system/res/a..b/c"sv);
    }

    SECTION("a component merely starting with `..` is allowed") {
        Block block = BlockBuilder {}.with_name("/..a/b").build();
        REQUIRE(build_path(block, root, /*is_dir=*/false) == "/system/res/..a/b"sv);
    }

    SECTION("non-rooted name is rejected") {
        Block block = BlockBuilder {}.with_name("a/b").build();
        REQUIRE(build_path(block, root, /*is_dir=*/false) == nullptr);
    }

    SECTION("non-empty prefix field is rejected") {
        Block block = BlockBuilder {}.with_name("/a/b").with_prefix("x").build();
        REQUIRE(build_path(block, root, /*is_dir=*/false) == nullptr);
    }
}

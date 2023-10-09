#include <transfers/decrypt.hpp>

#include <catch2/catch.hpp>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

using namespace transfers;
using std::min;
using std::string;
using std::string_view;
using std::vector;

namespace {

vector<uint8_t> read_file(string_view name) {
    string full_name = DATA_PATH;
    full_name += "/";
    full_name += name;
    vector<uint8_t> result;

    const size_t bufsize = 1024;

    char buffer[bufsize];

    FILE *f = fopen(full_name.c_str(), "rb");
    REQUIRE(f != nullptr);
    size_t amt;
    while ((amt = fread(buffer, 1, bufsize, f)) > 0) {
        // Any easier way to extend by all of the values?
        for (size_t i = 0; i < amt; i++) {
            result.push_back(buffer[i]);
        }
    }
    fclose(f);

    return result;
}

const Decryptor::Block key = { 0x1c, 0x44, 0x5f, 0xac, 0x4b, 0xb4, 0x01, 0x7c, 0x1e, 0xff, 0x53, 0x73, 0xea, 0x7f, 0x95, 0x1d };
const Decryptor::Block iv = { 0xa7, 0xf6, 0x5b, 0xce, 0xe0, 0xa6, 0xf4, 0x5f, 0xdf, 0x16, 0x9e, 0x31, 0x29, 0xf0, 0xa4, 0x37 };

} // namespace

TEST_CASE("Decrypt file") {
    const auto encrypted = read_file("box.crypt");
    const auto plain = read_file("box.gcode");

    auto ctr = Decryptor::CTR(iv);
    Decryptor decryptor(key, ctr, plain.size());

    size_t block_size = 1;

    SECTION("Cipher block") {
        block_size = 16;
    }

    SECTION("Multiple of blocks") {
        block_size = 32;
    }

    SECTION("Smaller") {
        block_size = 13;
    }

    SECTION("Larger") {
        block_size = 37;
    }

    SECTION("Everything in one go") {
        block_size = encrypted.size();
    }

    INFO("Block size " << block_size);
    size_t position_in = 0;
    size_t position_out = 0;

    while (position_in < encrypted.size()) {
        size_t rest = encrypted.size() - position_in;
        size_t block_in = min(rest, block_size);
        // Need in-out buffer.
        uint8_t block[block_in + 16];
        memcpy(block, &encrypted[0] + position_in, block_in);
        size_t out = decryptor.decrypt(block, block_in);
        REQUIRE(out <= block_in + 16);
        for (size_t i = 0; i < out; i++) {
            size_t pos_exp = position_out + i;
            INFO("Position " << pos_exp);
            REQUIRE(block[i] == plain[pos_exp]);
        }

        position_in += block_in;
        position_out += out;
    }

    REQUIRE(position_out == plain.size());
}

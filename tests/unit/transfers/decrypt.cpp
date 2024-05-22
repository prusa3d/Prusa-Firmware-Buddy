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
const Decryptor::Block nonce = { 0xa7, 0xf6, 0x5b, 0xce, 0xe0, 0xa6, 0xf4, 0x5f, 0xdf, 0x16, 0x9e, 0x31, 0x29, 0xf0, 0xa4, 0x37 };

} // namespace

TEST_CASE("Decrypt file") {
    const auto encrypted = read_file("box.crypt");
    const auto plain = read_file("box.gcode");

    Decryptor decryptor(key, nonce, 0, plain.size());

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
        size_t chunk = min(rest, block_size);
        uint8_t out_buff[chunk];
        auto [in_used, out_used] = decryptor.decrypt(&encrypted[0] + position_in, chunk, out_buff, chunk);
        REQUIRE(in_used <= chunk);
        REQUIRE(out_used <= chunk);
        for (size_t i = 0; i < out_used; i++) {
            size_t pos_exp = position_out + i;
            INFO("Position " << pos_exp);
            REQUIRE(out_buff[i] == plain[pos_exp]);
        }

        position_in += in_used;
        position_out += out_used;
    }

    REQUIRE(position_out == plain.size());
}

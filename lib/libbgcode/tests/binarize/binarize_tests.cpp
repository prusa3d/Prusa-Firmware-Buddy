#include <catch2/catch_test_macros.hpp>

#include "binarize/binarize.hpp"
#include "binarize/meatpack.hpp"

#include <string>
#include <vector>

using namespace MeatPack;

// Binarize lines via MeatPack and unbinarize back to verify round-trip.
static std::string roundtrip(const std::vector<std::string> &lines, uint8_t flags) {
    MPBinarizer mp_binarizer(flags);
    std::vector<uint8_t> binary;
    mp_binarizer.initialize(binary);
    for (const std::string &line: lines) {
        REQUIRE(mp_binarizer.binarize_line(line, binary));
    }

    mp_binarizer.finalize(binary);

    std::string unbinarize_result;
    unbinarize(binary, unbinarize_result);

    return unbinarize_result;
}

TEST_CASE("MeatPackComments preserves indented comments", "[Binarize][MeatPack]") {
    const std::vector<std::string> lines = {
        ";FLUSH_START\n",
        " G1 E1 F6000\n",
        "  ;FLUSH_END\n",
        "\t;TAB_COMMENT\n",
    };

    const std::string roundtrip_result = roundtrip(lines, Flag_OmitWhitespaces);
    REQUIRE(roundtrip_result.find(";FLUSH_START") != std::string::npos);
    REQUIRE(roundtrip_result.find(";FLUSH_END") != std::string::npos);
    REQUIRE(roundtrip_result.find(";TAB_COMMENT") != std::string::npos);
    REQUIRE(roundtrip_result.find("G1") != std::string::npos);
}

TEST_CASE("MeatPack removes indented comments when Flag_RemoveComments set", "[Binarize][MeatPack]") {
    const std::vector<std::string> lines = {
        ";FLUSH_START\n",
        " G1 E1 F6000\n",
        "  ;FLUSH_END\n",
        "\t;TAB_COMMENT\n",
    };

    const std::string roundtrip_result = roundtrip(lines, Flag_OmitWhitespaces | Flag_RemoveComments);
    REQUIRE(roundtrip_result.find(";FLUSH_START") == std::string::npos);
    REQUIRE(roundtrip_result.find(";FLUSH_END") == std::string::npos);
    REQUIRE(roundtrip_result.find(";TAB_COMMENT") == std::string::npos);
    REQUIRE(roundtrip_result.find("G1") != std::string::npos);
}

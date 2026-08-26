#include <catch2/catch_test_macros.hpp>

#include "binarize_test_utils.hpp"
#include "binarize/meatpack.hpp"

#include <cstdint>
#include <string>
#include <vector>

using namespace bgcode::core;
using namespace bgcode::binarize;
using namespace bgcode_test;
using MeatPack::unbinarize;

namespace {

// A MeatPack stream that makes one decode iteration emit four characters:
//   FF FF FB  enable packing (signal, signal, command 251)
//   0F        first-half packed, second not: queues one full char and buffers it
//   FF        a lone signal byte, held as a pending literal
//   00        resolves the pending signal, so this iteration decodes the buffered
//             literal, the signal byte and then a fresh packed pair
// The last iteration therefore calls the receive handler twice and writes four
// characters before the buffer is drained. The output buffer used to hold two,
// so the third write ran off the end of a stack array - an out-of-bounds write.
const std::vector<uint8_t> kOverflowingMeatPackStream = {
    0xFF, 0xFF, 0xFB, 0x0F, 0xFF, 0x00,
};

} // namespace

// A single decode iteration can call the receive handler twice and each call can
// emit two characters, so up to four accumulate before the drain. The output
// buffer used to be two bytes, so the third write left it - an out-of-bounds
// stack write, not merely a read. Decoding the crafted stream must stay in
// bounds; only that is asserted (caught as a stack-buffer-overflow under ASan, or
// as stack smashing without it), the decoded text itself is unspecified.
TEST_CASE("unbinarize does not overflow its output buffer", "[Binarize][MeatPack]") {
    std::string out;
    REQUIRE_NOTHROW(unbinarize(kOverflowingMeatPackStream, out));
}

// The same stream through the public reader path (GCodeBlock::read_data) an
// opened .bgcode file takes. It must return an ordinary result rather than write
// out of bounds.
TEST_CASE("reading a crafted MeatPack gcode block stays in bounds", "[Binarize][MeatPack]") {
    const ScopedFile file = make_block_file(EBlockType::GCode, ECompressionType::None,
                                            static_cast<uint16_t>(EGCodeEncodingType::MeatPack),
                                            static_cast<uint32_t>(kOverflowingMeatPackStream.size()), 0,
                                            kOverflowingMeatPackStream);

    GCodeBlock block;
    const EResult result = read_first_block(*file, block);

    REQUIRE((result == EResult::Success || result == EResult::GCodeDecodingError));
}

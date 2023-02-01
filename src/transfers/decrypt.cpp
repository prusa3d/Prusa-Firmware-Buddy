#include "decrypt.hpp"

#include <cassert>
#include <cstring>

using std::min;

namespace transfers {

Decryptor::Decryptor(const Block &key, const Block &iv, uint32_t orig_size)
    : size_left(orig_size)
    , iv(iv) {
    mbedtls_aes_init(&context);
    mbedtls_aes_setkey_dec(&context, key.begin(), key.size() * 8);
}

Decryptor::~Decryptor() {
    mbedtls_aes_free(&context);
}

uint32_t Decryptor::decrypt(uint8_t *buffer, uint32_t input_size) {
    /*
     * The cipher works in blocks. So we need to feed it in blocks and keep a
     * leftover around. There's a bunch of cornercases around that, so we make
     * it as simple as possible ‒ loop over single block segments and end each
     * one in a consistent state.
     *
     * We could probably do so in fewer calls to mbedtls (gather a longer
     * chunk) or do so with fewer local variables, but correctness/simplicity
     * takes precedence here. Besides, the compiler is likely to optimize most
     * of the local variables out anyway.
     */

    uint32_t output_size = 0;
    while (input_size + leftover_size >= BlockSize) {
        uint8_t *const output_buffer = buffer;
        // First, gather one block in leftover ‒ just complete the rest (the
        // rest _may_ be 0, it won't make a difference).
        const uint32_t rest = BlockSize - leftover_size;
        assert(rest <= input_size);
        memcpy(leftover.begin() + leftover_size, buffer, rest);
        input_size -= rest;
        buffer += rest;

        // Decrypt that one block
        Block output;
        mbedtls_aes_crypt_cbc(&context, MBEDTLS_AES_DECRYPT, BlockSize, iv.data() /* updated in place */, leftover.begin(), output.begin());

        // Shuffle the things around - we need to make room for the whole block
        // in the buffer, so we move whatever stands in the way into leftover
        // for the next run.
        leftover_size = min(BlockSize - rest, input_size);
        memcpy(leftover.begin(), buffer, leftover_size);
        input_size -= leftover_size;
        buffer += leftover_size;

        // Handle chopping off the padding at the end.
        const uint32_t out_chunk = min(size_left, BlockSize);

        memcpy(output_buffer, output.begin(), out_chunk);
        size_left -= out_chunk;
        output_size += out_chunk;
    }

    // Whatever is left inside the buffer can't form a full block now, but keep
    // it around for next time.
    memcpy(leftover.begin() + leftover_size, buffer, input_size);
    leftover_size += input_size;
    assert(leftover_size < BlockSize);

    return output_size;
}

}

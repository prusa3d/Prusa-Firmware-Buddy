#include "decrypt.hpp"

#include <common/bsod.h>
#include <logging/log.h>

#include <type_traits>
#include <cassert>
#include <cstring>

using std::min;

LOG_COMPONENT_REF(connect);

namespace transfers {

Decryptor::Decryptor(const Block &key, const Mode mode, uint32_t orig_size)
    : size_left(orig_size)
    , mode(mode) {
    mbedtls_aes_init(&context);
    this->mode.setup_context(context, key);
}

Decryptor::~Decryptor() {
    mbedtls_aes_free(&context);
}

uint32_t Decryptor::decrypt(uint8_t *buffer, uint32_t buffer_size) {
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
    while (buffer_size + leftover_size >= BlockSize) {
        uint8_t *const output_buffer = buffer;
        // First, gather one block in leftover ‒ just complete the rest (the
        // rest _may_ be 0, it won't make a difference).
        const uint32_t rest = BlockSize - leftover_size;
        assert(rest <= buffer_size);
        memcpy(leftover.begin() + leftover_size, buffer, rest);
        buffer_size -= rest;
        buffer += rest;

        // Decrypt that one block
        Block output;
        mode.decrypt(context, leftover, output);

        // Shuffle the things around - we need to make room for the whole block
        // in the buffer, so we move whatever stands in the way into leftover
        // for the next run.
        leftover_size = min(BlockSize - rest, buffer_size);
        memcpy(leftover.begin(), buffer, leftover_size);
        buffer_size -= leftover_size;
        buffer += leftover_size;

        // Handle chopping off the padding at the end.
        const uint32_t out_chunk = min(size_left, BlockSize);

        memcpy(output_buffer, output.begin(), out_chunk);
        size_left -= out_chunk;
        output_size += out_chunk;
    }

    // Whatever is left inside the buffer can't form a full block now, but keep
    // it around for next time.
    memcpy(leftover.begin() + leftover_size, buffer, buffer_size);
    leftover_size += buffer_size;
    assert(leftover_size < BlockSize);

    return output_size;
}

void Decryptor::CTR::setup_context(mbedtls_aes_context &context, const Block &key) {
    mbedtls_aes_init(&context);
    mbedtls_aes_setkey_enc(&context, key.data(), key.size() * 8);
}

void Decryptor::CTR::decrypt(mbedtls_aes_context &context, const Block &input, Block &output) {
    size_t nonce_offset = 0;
    Block stream_block;
    mbedtls_aes_crypt_ctr(&context, BlockSize, &nonce_offset, nonce.data(), stream_block.data(), input.begin(), output.begin());
    assert(nonce_offset == 0); // we start at 0 and end at 0 (one whole block)
}

void Decryptor::CTR::reset(const Block &nonce, uint32_t offset) {
    assert(offset % BlockSize == 0);

    this->nonce = nonce;

    /// Set counter to the block index we are going to decrypt
    uint32_t block_idx = offset / BlockSize;
    for (uint32_t i = BlockSize - 1; i >= BlockSize - CtrCounterSize; i--) {
        this->nonce[i] = block_idx & 0xff;
        block_idx >>= 8;
    }
}

} // namespace transfers

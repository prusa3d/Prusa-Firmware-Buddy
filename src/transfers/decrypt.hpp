#pragma once

#include <array>
#include <tuple>
#include <cstdint>
#include <cstdlib>
#include <assert.h>

#include <mbedtls/aes.h>

namespace transfers {

class Decryptor {
public:
    static constexpr uint32_t BlockSize = 16;
    static constexpr uint32_t CtrCounterSize = 4;
    using Block = std::array<uint8_t, BlockSize>;

private:
    size_t stoff = 0;

    // We now use the CTR mode of aes. That one is a stream cipher (doesn't
    // need to be block-padded). Nevertheless, for historical reasons, we _do_
    // the block padding on the sender side and cut it off here by the expected
    // size.
    size_t size_left;
    // Note:
    // There doesn't seem to be a legal way of moving this thing after it is
    // initialized. At least, memcpy does *not* work. Discovered the hard way.
    //
    // Therefore, the whole Decryptor is not movable _on purpose_.
    mbedtls_aes_context context;
    Block stream = {};
    Block nonce = {};

public:
    Decryptor(const Block &key, const Block &nonce, size_t offset, size_t size_left);
    Decryptor(const Decryptor &other) = delete;
    Decryptor(Decryptor &&other) = delete;
    Decryptor &operator=(const Decryptor &other) = delete;
    Decryptor &operator=(Decryptor &&other) = delete;
    ~Decryptor();

    // Decrypts some data from in-buffer to the out-buffer.
    //
    // Returns how much was consumed of each buffer. Will make at least some progress.
    std::tuple<size_t, size_t> decrypt(const uint8_t *in, size_t in_size, uint8_t *out, size_t out_size);
};

} // namespace transfers

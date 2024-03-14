#include "decrypt.hpp"

#include <common/bsod.h>
#include <logging/log.h>

#include <type_traits>
#include <cassert>
#include <cstring>

using std::make_tuple;
using std::min;
using std::tuple;

LOG_COMPONENT_REF(connect);

namespace transfers {

Decryptor::Decryptor(const Block &key, const Block &nonce, size_t offset, size_t size_left)
    : size_left(size_left)
    , nonce(nonce) {
    assert(offset % BlockSize == 0);

    /// Set counter to the block index we are going to decrypt
    uint32_t block_idx = offset / BlockSize;
    for (uint32_t i = BlockSize - 1; i >= BlockSize - CtrCounterSize; i--) {
        this->nonce[i] = block_idx & 0xff;
        block_idx >>= 8;
    }

    mbedtls_aes_init(&context);
    mbedtls_aes_setkey_enc(&context, key.data(), key.size() * 8);
}

Decryptor::~Decryptor() {
    mbedtls_aes_free(&context);
}

tuple<size_t, size_t> Decryptor::decrypt(const uint8_t *in, size_t in_size, uint8_t *out, size_t out_size) {
    size_t amnt = min(size_left, min(in_size, out_size));
    size_left -= amnt;
    auto ret = mbedtls_aes_crypt_ctr(&context, amnt, &stoff, nonce.data(), stream.data(), in, out);
    if (ret != 0) {
        // There should be no failure mode in real life for decryption with
        // correct parameters, but we are super-careful and catch any
        // unexpected return values because handling encrypted data is
        // sensitive thing at best.
        bsod("Decryption failed");
    }
    return make_tuple(size_left ? amnt : in_size /* Discard the rest of input when everything consumed */, amnt);
}

} // namespace transfers

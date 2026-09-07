#include "decryptor.hpp"

#include <cstring>
#include <bsod/bsod.h>

namespace e2ee {
void Decryptor::set_cipher_info(e2ee::SymmetricCipherInfo cipher_info) {
    mbedtls_aes_setkey_dec(aes_ctx.context.get(), cipher_info.keys->encryption_key, e2ee::KEY_SIZE * 8);
    num_of_hmacs = cipher_info.num_of_hmacs;
}

Decryptor::Decryptor() {
    cache_curr_pos = cache.end();
    cache_end = cache.end();
}

Decryptor::~Decryptor() {
}

void Decryptor::setup_block(uint64_t offset, uint32_t block_size) {
    memset(cache.data(), 0, cache.size());
    cache_curr_pos = cache.end();
    cache_end = cache.end();
    memset(iv.data(), 0, iv.size());
    memcpy(iv.data(), reinterpret_cast<const uint8_t *>(&offset), sizeof offset);
    remaining_encrypted_data_size = block_size - num_of_hmacs * e2ee::HMAC_SIZE;
    debug_assert(remaining_encrypted_data_size % BlockSize == 0);
}

namespace {
    std::optional<size_t> pkcs7_padding_data_len(Decryptor::Block &block) {
        uint8_t padding_len = block.back();
        for (size_t i = block.size() - 1; i > block.size() - 1 - padding_len; i--) {
            if (block[i] != padding_len) {
                // Invalid padding
                return std::nullopt;
            }
        }
        return block.size() - padding_len;
    }
} // namespace

// Note: The error checks here can fail only from programmer mistakes or malformed bgcode file
bool Decryptor::decrypt(FILE *file, uint8_t *buffer, size_t size) {
    if (size > remaining_encrypted_data_size) {
        return false;
    }

    size_t cache_size = cache_end - cache_curr_pos;
    // Take all we can from cache
    size_t from_cache = std::min(size, cache_size);
    memcpy(buffer, cache_curr_pos, from_cache);
    cache_curr_pos += from_cache;
    size -= from_cache;
    remaining_encrypted_data_size -= from_cache;

    // Decrypt the rest, until we are done
    while (size > 0) {
        Block in;
        if (fread(in.data(), 1, in.size(), file) != in.size()) {
            return false;
        }
        size_t to_return = std::min(size, in.size());
        auto ret = mbedtls_aes_crypt_cbc(aes_ctx.context.get(), MBEDTLS_AES_DECRYPT, cache.size(), iv.data(), in.data(), cache.data());
        if (ret != 0) {
            return false;
        }
        // last block, handle padding
        if (remaining_encrypted_data_size == BlockSize) {
            auto data_size_opt = pkcs7_padding_data_len(cache);
            if (!data_size_opt.has_value()) {
                return false;
            }
            size_t data_size = data_size_opt.value();
            if (to_return > data_size) {
                return false;
            }
            cache_curr_pos = cache.begin();
            // point one after the last valid byte
            cache_end = cache.begin() + data_size + 1;
            remaining_encrypted_data_size -= BlockSize - data_size;
        } else {
            cache_curr_pos = cache.begin();
        }
        memcpy(buffer, cache_curr_pos, to_return);
        remaining_encrypted_data_size -= to_return;
        cache_curr_pos += to_return;
        buffer += to_return;
        size -= to_return;
    }
    return true;
}
} // namespace e2ee

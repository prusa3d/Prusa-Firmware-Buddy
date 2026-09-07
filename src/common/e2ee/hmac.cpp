#include "hmac.hpp"

#include <logging/log.hpp>
#include <bsod.h>
#include <crc32.h>

#include <cstring>

using bgcode::core::BlockHeader;
using bgcode::core::EBlockType;
using bgcode::core::ECompressionType;

LOG_COMPONENT_REF(PRUSA_PACK_READER);
namespace e2ee {

HMAC::HMAC(const uint8_t *sign_key, size_t key_len) {
    mbedtls_md_init(&md_ctx);
    // These errors should not happen in practise
    if (auto res = mbedtls_md_setup(&md_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1); res != 0) {
        bsod("Unable to setup HMAC context: %d", res);
    }
    mbedtls_md_hmac_starts(&md_ctx, sign_key, key_len);
}

HMAC::~HMAC() {
    mbedtls_md_free(&md_ctx);
}

void HMAC::update(uint8_t *data, size_t size) {
    mbedtls_md_hmac_update(&md_ctx, data, size);
}

void HMAC::finish(uint8_t *output, [[maybe_unused]] size_t size) {
    debug_assert(size == e2ee::HMAC_SIZE);
    mbedtls_md_hmac_finish(&md_ctx, output);
}

CheckResult check_hmac_and_crc(FILE *file, BlockHeader header, e2ee::SymmetricCipherInfo &info, bool check_crc) {
    const bool check_hmac = (EBlockType)header.type == EBlockType::EncryptedBlock;
    long pos = ftell(file);
    HMAC hmac(info.keys->sign_key, std::size(info.keys->sign_key));
    uint32_t crc = 0;
    if (check_hmac) {
        block_header_bytes_cb(header, [&hmac](uint8_t *data, size_t size) {
            hmac.update(data, size);
        });
        uint8_t iv[16];
        auto header_pos = header.get_position();
        memset(iv, 0, sizeof(iv));
        memcpy(iv, reinterpret_cast<const uint8_t *>(&header_pos), sizeof header_pos);
        hmac.update(iv, sizeof(iv));
    }
    if (check_crc) {
        block_header_bytes_cb(header, [&crc](uint8_t *data, size_t size) {
            crc = crc32_calc_ex(crc, data, size);
        });
    }
    constexpr size_t BLOCK_SIZE = 64;
    uint8_t block[BLOCK_SIZE];
    const size_t hmacs_size = info.num_of_hmacs * e2ee::HMAC_SIZE;
    size_t hmac_data_size = bgcode::core::block_payload_size(header) - hmacs_size;
    while (hmac_data_size > 0) {
        size_t to_read = std::min(hmac_data_size, BLOCK_SIZE);
        if (fread(block, 1, to_read, file) != to_read) {
            return CheckResult::ERROR;
        }

        if (check_hmac) {
            hmac.update(block, to_read);
        }
        if (check_crc) {
            crc = crc32_calc_ex(crc, block, to_read);
        }
        hmac_data_size -= to_read;
    }
    // Read HMACs for crc
    if (check_crc) {
        size_t size = hmacs_size;
        while (size > 0) {
            size_t to_read = std::min(size, BLOCK_SIZE);
            if (fread(block, 1, to_read, file) != to_read) {
                return CheckResult::ERROR;
            }
            crc = crc32_calc_ex(crc, block, to_read);
            size -= to_read;
        }
        uint32_t read_crc;
        if (fread(&read_crc, sizeof(read_crc), 1, file) != 1) {
            return CheckResult::ERROR;
        }
        if (read_crc != crc) {
            return CheckResult::CORRUPTED;
        }
    }

    if (check_hmac) {
        uint8_t computed_hmac[e2ee::HMAC_SIZE];
        hmac.finish(computed_hmac, sizeof(computed_hmac));
        size_t hmac_pos = header.get_position() + header.get_size() + bgcode::core::block_payload_size(header) - info.num_of_hmacs * e2ee::HMAC_SIZE + info.hmac_index * e2ee::HMAC_SIZE;
        if (fseek(file, hmac_pos, SEEK_SET) != 0) {
            return CheckResult::ERROR;
        }
        uint8_t read_hmac[e2ee::HMAC_SIZE];
        if (fread(read_hmac, 1, sizeof(read_hmac), file) != sizeof(read_hmac)) {
            return CheckResult::ERROR;
        }
        if (memcmp(read_hmac, computed_hmac, e2ee::HMAC_SIZE) != 0) {
            log_info(PRUSA_PACK_READER, "HMAC mismatch in block starting at: %ld", header.get_position());
            return CheckResult::CORRUPTED;
        }
    }

    if (fseek(file, pos, SEEK_SET) != 0) {
        return CheckResult::ERROR;
    }
    return CheckResult::OK;
}
} // namespace e2ee

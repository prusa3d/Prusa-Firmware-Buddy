#include <gcode_reader_binary.hpp>
#include <e2ee/decryptor.hpp>
#include <unique_file_ptr.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <mbedtls/aes.h>

constexpr uint8_t encryption_key[] = {
    0xb1, 0x1c, 0x6c, 0xb1, 0xf4, 0x10, 0x22, 0x11,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
};
constexpr uint64_t iv = 1234567890;

uint32_t get_file_size(FILE *file) {
    const long saved_pos = ftell(file);
    REQUIRE(saved_pos >= 0);
    REQUIRE(fseek(file, 0, SEEK_END) == 0);
    const long size = ftell(file);
    REQUIRE(size >= 0);
    REQUIRE(fseek(file, saved_pos, SEEK_SET) == 0);
    return size;
}

// Encrypts a single 16B block the way the fixture files were produced, so a
// bad PKCS#7 padding byte can be planted at a chosen decrypted position.
unique_file_ptr encrypted_block_file(const e2ee::Decryptor::Block &plaintext) {
    e2ee::Decryptor::Block iv_bytes = {};
    memcpy(iv_bytes.data(), &iv, sizeof(iv));

    mbedtls_aes_context enc_ctx;
    mbedtls_aes_init(&enc_ctx);
    REQUIRE(mbedtls_aes_setkey_enc(&enc_ctx, encryption_key, e2ee::KEY_SIZE * 8) == 0);
    e2ee::Decryptor::Block ciphertext;
    REQUIRE(mbedtls_aes_crypt_cbc(&enc_ctx, MBEDTLS_AES_ENCRYPT, plaintext.size(), iv_bytes.data(), plaintext.data(), ciphertext.data()) == 0);
    mbedtls_aes_free(&enc_ctx);

    unique_file_ptr file(tmpfile());
    REQUIRE(file.get() != nullptr);
    REQUIRE(fwrite(ciphertext.data(), 1, ciphertext.size(), file.get()) == ciphertext.size());
    REQUIRE(fseek(file.get(), 0, SEEK_SET) == 0);
    return file;
}

TEST_CASE("16 bytes encrypted") {
    e2ee::Decryptor decryptor;
    e2ee::SymmetricCipherInfo keys;
    keys.valid = true;
    memcpy(keys.keys->encryption_key, encryption_key, 16);
    decryptor.set_cipher_info(keys);
    uint8_t original[] = {
        0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20,
        0x73, 0x6f, 0x6d, 0x65, 0x20, 0x74, 0x65, 0x78
    };

    uint8_t buffer[16];
    unique_file_ptr file(fopen("./encrypted_text", "rb"));
    REQUIRE(file.get() != nullptr);
    auto data_size = get_file_size(file.get());
    REQUIRE(decryptor.setup_block(iv, data_size));

    SECTION("Read one block - one go") {
        REQUIRE(decryptor.decrypt(file.get(), buffer, sizeof(buffer)));
    }

    SECTION("Read by char") {
        memset(buffer, 0, sizeof(buffer));
        for (size_t i = 0; i < sizeof(buffer); i++) {
            REQUIRE(decryptor.decrypt(file.get(), buffer + i, 1));
            REQUIRE(memcmp(buffer, original, i) == 0);
        }
    }

    SECTION("Read by more sizes") {
        memset(buffer, 0, sizeof(buffer));
        REQUIRE(decryptor.decrypt(file.get(), buffer, 8));
        REQUIRE(memcmp(buffer, original, 8) == 0);
        REQUIRE(decryptor.decrypt(file.get(), buffer + 8, 2));
        REQUIRE(memcmp(buffer, original, 10) == 0);
        REQUIRE(decryptor.decrypt(file.get(), buffer + 10, 4));
        REQUIRE(memcmp(buffer, original, 14) == 0);
        REQUIRE(decryptor.decrypt(file.get(), buffer + 14, 2));
    }

    REQUIRE(memcmp(buffer, original, sizeof(original)) == 0);
}

TEST_CASE("20 bytes encrypted") {
    e2ee::Decryptor decryptor;
    e2ee::SymmetricCipherInfo keys;
    keys.valid = true;
    memcpy(keys.keys->encryption_key, encryption_key, 16);
    decryptor.set_cipher_info(keys);
    uint8_t original[] = {
        0x54,
        0x68,
        0x69,
        0x73,
        0x20,
        0x69,
        0x73,
        0x20,
        0x73,
        0x6f,
        0x6d,
        0x65,
        0x20,
        0x74,
        0x65,
        0x78,
        0x74,
        0x20,
        0x66,
        0x6f,
    };

    uint8_t buffer[20];
    unique_file_ptr file(fopen("./20_bytes_encrypted_text", "rb"));
    REQUIRE(file.get() != nullptr);
    auto data_size = get_file_size(file.get());
    REQUIRE(decryptor.setup_block(iv, data_size));

    SECTION("Read one block - one go") {
        REQUIRE(decryptor.decrypt(file.get(), buffer, sizeof(buffer)));
    }

    SECTION("Read by char") {
        memset(buffer, 0, sizeof(buffer));
        for (size_t i = 0; i < sizeof(buffer); i++) {
            REQUIRE(decryptor.decrypt(file.get(), buffer + i, 1));
            REQUIRE(memcmp(buffer, original, i + 1) == 0);
        }
    }

    SECTION("Read by more sizes 2 goes") {
        memset(buffer, 0, sizeof(buffer));
        REQUIRE(decryptor.decrypt(file.get(), buffer, 18));
        REQUIRE(memcmp(buffer, original, 18) == 0);
        REQUIRE(decryptor.decrypt(file.get(), buffer + 18, 2));
    }

    // Note: The move has to be either on the edge of the block or read enough after
    // so that we don't just take it from cache and really call the decryption
    SECTION("Test move constructor of decryptor") {
        memset(buffer, 0, sizeof(buffer));
        REQUIRE(decryptor.decrypt(file.get(), buffer, 16));
        REQUIRE(memcmp(buffer, original, 16) == 0);
        e2ee::Decryptor another_decryptor(std::move(decryptor));
        REQUIRE(another_decryptor.decrypt(file.get(), buffer + 16, 4));
    }

    SECTION("Test move operator of decryptor") {
        memset(buffer, 0, sizeof(buffer));
        REQUIRE(decryptor.decrypt(file.get(), buffer, 16));
        REQUIRE(memcmp(buffer, original, 16) == 0);
        e2ee::Decryptor another_decryptor;
        another_decryptor = std::move(decryptor);
        REQUIRE(another_decryptor.decrypt(file.get(), buffer + 16, 4));
    }

    SECTION("Read by more sizes") {
        memset(buffer, 0, sizeof(buffer));
        REQUIRE(decryptor.decrypt(file.get(), buffer, 8));
        REQUIRE(memcmp(buffer, original, 8) == 0);
        REQUIRE(decryptor.decrypt(file.get(), buffer + 8, 2));
        REQUIRE(memcmp(buffer, original, 10) == 0);
        REQUIRE(decryptor.decrypt(file.get(), buffer + 10, 4));
        REQUIRE(memcmp(buffer, original, 14) == 0);
        REQUIRE(decryptor.decrypt(file.get(), buffer + 14, 2));
        REQUIRE(memcmp(buffer, original, 16) == 0);
        REQUIRE(decryptor.decrypt(file.get(), buffer + 16, 4));
    }

    REQUIRE(memcmp(buffer, original, sizeof(original)) == 0);
}

TEST_CASE("setup_block rejects a block_size smaller than the HMACs it declares") {
    e2ee::Decryptor decryptor;
    e2ee::SymmetricCipherInfo keys;
    keys.valid = true;
    keys.num_of_hmacs = 2;
    memcpy(keys.keys->encryption_key, encryption_key, 16);
    decryptor.set_cipher_info(keys);

    REQUIRE_FALSE(decryptor.setup_block(iv, 2 * e2ee::HMAC_SIZE - 1));

    uint8_t buffer[16];
    unique_file_ptr file(fopen("./encrypted_text", "rb"));
    REQUIRE(file.get() != nullptr);
    // The invalid setup_block() call must not leave the decryptor able to
    // hand out data.
    REQUIRE_FALSE(decryptor.decrypt(file.get(), buffer, sizeof(buffer)));
}

TEST_CASE("setup_block rejects a payload that is not a whole number of cipher blocks") {
    e2ee::Decryptor decryptor;
    e2ee::SymmetricCipherInfo keys;
    keys.valid = true;
    keys.num_of_hmacs = 2;
    memcpy(keys.keys->encryption_key, encryption_key, 16);
    decryptor.set_cipher_info(keys);

    // Such a payload never reaches the last-block branch, so its padding would
    // be handed out as if it were data.
    REQUIRE_FALSE(decryptor.setup_block(iv, 2 * e2ee::HMAC_SIZE + 5));

    uint8_t buffer[16];
    unique_file_ptr file(fopen("./encrypted_text", "rb"));
    REQUIRE(file.get() != nullptr);
    REQUIRE_FALSE(decryptor.decrypt(file.get(), buffer, sizeof(buffer)));
}

TEST_CASE("decrypt rejects a malformed PKCS#7 padding byte") {
    uint8_t padding_len = GENERATE(0, 17, 255);
    CAPTURE(padding_len);

    e2ee::Decryptor::Block plaintext;
    plaintext.fill(0x41);
    plaintext.back() = padding_len;
    unique_file_ptr file = encrypted_block_file(plaintext);

    e2ee::Decryptor decryptor;
    e2ee::SymmetricCipherInfo keys;
    keys.valid = true;
    memcpy(keys.keys->encryption_key, encryption_key, 16);
    decryptor.set_cipher_info(keys);

    REQUIRE(decryptor.setup_block(iv, get_file_size(file.get())));

    uint8_t buffer[16];
    REQUIRE_FALSE(decryptor.decrypt(file.get(), buffer, sizeof(buffer)));
}

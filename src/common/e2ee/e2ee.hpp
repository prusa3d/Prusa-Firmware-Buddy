#pragma once

#include <crash_dump/secret.hpp>
#include <async_job/async_job_execution_control.hpp>
#include <optional>
#include <memory>
#include <array>
#include <cstring>

namespace bgcode {
namespace core {
    struct BlockHeader;
}
} // namespace bgcode
struct mbedtls_pk_context;

namespace e2ee {

struct Pk;
class SHA256MultiuseHash;

constexpr size_t HASH_SIZE = 32;
constexpr size_t KEY_HASH_STR_BUFFER_LEN = 2 * HASH_SIZE + 1;
constexpr size_t HMAC_SIZE = 32;
constexpr size_t KEY_SIZE = 16;
constexpr size_t SIGN_SIZE = 256;
constexpr size_t IDENTITY_NAME_LEN = 32;
// Size discovered by experimental means.
// FIXME: the key is probably smaller, investigate the size more and maybe make this smaller
constexpr size_t PRIVATE_KEY_BUFFER_SIZE = 2048;
constexpr size_t PUBLIC_KEY_BUFFER_SIZE = 400;
#ifdef UNITTESTS
constexpr const char *const private_key_path = "printer_private_key.der";
#else
constexpr const char *const private_key_path = "/system/e2ee/printer/pk.der";
#endif
constexpr const char *const identities_folder = "/system/e2ee/identities/";
constexpr const char *const identities_tmp_folder = "/system/e2ee/tmp_identities/";
constexpr size_t IDENTITY_PATH_LEN = strlen(identities_folder) + e2ee::HASH_SIZE * 2 + 1;
constexpr size_t IDENTITY_TMP_PATH_LEN = strlen(identities_tmp_folder) + e2ee::HASH_SIZE * 2 + 1;
constexpr size_t IDENTITY_PATH_MAX_LEN = std::max(IDENTITY_PATH_LEN, IDENTITY_TMP_PATH_LEN);
constexpr const char *const public_key_path = "/usb/pubkey.der";

// Error texts
constexpr const char *encrypted_for_different_printer = "Bgcode not encrypted for this printer!";
constexpr const char *key_block_hash_mismatch = "Key block hash mismatch";
constexpr const char *metadata_not_beggining = "Corrupted bgcode, metadata not at the beggining.";
constexpr const char *additional_data = "Additional non authorized data found.";
constexpr const char *key_before_identity = "Corrupted bgcode, key block before identity block.";
constexpr const char *encrypted_before_identity = "Corrupted bgcode, encrypted block before identity block.";
constexpr const char *encrypted_before_key = "Corrupted bgcode, encrypted block before key block.";
constexpr const char *unencrypted_in_encrypted = "Unencrypted gcode block found in encrypted bgcode.";
constexpr const char *file_error = "Error while reading file.";
constexpr const char *unknown_identity_cypher = "Unknown Identity block cypher";
constexpr const char *compressed_identity_block = "Compressed identity block not supported";
constexpr const char *identity_parsing_error = "Identity block parsing error";
constexpr const char *identity_verification_fail = "Identity verification failed!";
constexpr const char *identity_name_too_long = "Identity name too long";
constexpr const char *corrupted_metadata = "File has corrupted metadata";

struct IdentityBlockInfo {

    std::unique_ptr<Pk> identity_pk;
    // TODO: how long sould we allow this to be??
    std::array<char, IDENTITY_NAME_LEN> identity_name;
    std::array<uint8_t, HASH_SIZE> key_block_hash;
    bool one_time_identity;

    IdentityBlockInfo();
    ~IdentityBlockInfo();
    IdentityBlockInfo(const IdentityBlockInfo &) = delete;
    IdentityBlockInfo &operator=(const IdentityBlockInfo &) = delete;
    IdentityBlockInfo(IdentityBlockInfo &&);
    IdentityBlockInfo &operator=(IdentityBlockInfo &&);
};

struct IdentityInfo {
    std::array<char, e2ee::IDENTITY_NAME_LEN> identity_name;
    std::array<char, e2ee::KEY_HASH_STR_BUFFER_LEN> key_hash_str;
    bool one_time = false;
};

class PrinterPrivateKey {
public:
    PrinterPrivateKey();
    ~PrinterPrivateKey();
    PrinterPrivateKey(const PrinterPrivateKey &) = delete;
    PrinterPrivateKey &operator=(const PrinterPrivateKey &) = delete;
    PrinterPrivateKey(PrinterPrivateKey &&) = delete;
    PrinterPrivateKey &operator=(PrinterPrivateKey &&) = delete;
    mbedtls_pk_context *get_printer_private_key();

private:
    bool key_valid = false;
    std::unique_ptr<Pk> key;
};

// Looks for private key at private_key_path
bool is_private_key_present();

// if computed_intro_hash is nullptr the hash is not checked
const char *read_and_verify_identity_block(FILE *file, const bgcode::core::BlockHeader &block_header, uint8_t *computed_intro_hash, IdentityBlockInfo &info, bool verify_signature);

struct SymmetricCipherInfo {
    bool valid = false;
    struct Keys {
        uint8_t encryption_key[KEY_SIZE];
        uint8_t sign_key[KEY_SIZE];
    };
    crash_dump::Secret<Keys> keys;
    uint32_t num_of_hmacs = 0;
    uint32_t hmac_index = 0; // where our HMAC is

    bool extract_keys(uint8_t *key_block, size_t size);
};

std::optional<SymmetricCipherInfo> decrypt_key_block(FILE *file, const bgcode::core::BlockHeader &block_header, Pk &identity_pk, mbedtls_pk_context *printer_private_key, SHA256MultiuseHash *hash);

bool rsa_sha256_sign_verify(mbedtls_pk_context &pk, const uint8_t *message, size_t message_size, const uint8_t *signature, size_t sig_size);
bool rsa_oaep_decrypt(mbedtls_pk_context &pk, const uint8_t *encrypted_msg, size_t msg_size, uint8_t *output, size_t out_size);

} // namespace e2ee

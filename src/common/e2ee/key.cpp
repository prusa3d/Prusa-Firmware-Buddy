#include "key.hpp"
#include "e2ee.hpp"

#include <stat_retry.hpp>
#include <path_utils.h>
#include <common/directory.hpp>

#include <sys/stat.h>
#include <unique_file_ptr.hpp>
#include <raii/deleter.hpp>
#include <heap.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <memory>
#include <cstring>
#include <bsod/bsod.h>

using std::unique_ptr;

extern "C" {
size_t strlcpy(char *, const char *, size_t);
size_t strlcat(char *, const char *, size_t);
}

namespace {

// So we get RAII for all the init-free contexts of mbedtls
struct KeyGenContexts {
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_pk_context pk;

    KeyGenContexts() {
        mbedtls_entropy_init(&entropy);
        mbedtls_ctr_drbg_init(&ctr_drbg);
        mbedtls_pk_init(&pk);
        mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
        crash_dump::privacy_protection.reg(pk.pk_ctx, sizeof(mbedtls_rsa_context));
    }
    KeyGenContexts(const KeyGenContexts &) = delete;
    KeyGenContexts(KeyGenContexts &&) = delete;
    KeyGenContexts &operator=(const KeyGenContexts &) = delete;
    KeyGenContexts &operator=(KeyGenContexts &&) = delete;
    ~KeyGenContexts() {
        crash_dump::privacy_protection.unreg(pk.pk_ctx);
        mbedtls_pk_free(&pk);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        mbedtls_entropy_free(&entropy);
    }
};

bool save_identity_key_impl(const e2ee::IdentityInfo &info, const char *folder) {
    e2ee::IdentityKeyInfo key_info;
    strlcpy(key_info.identity_name, info.identity_name.data(), e2ee::IDENTITY_NAME_LEN);
    constexpr size_t buff_size = std::max(e2ee::IDENTITY_TMP_PATH_LEN, e2ee::IDENTITY_PATH_LEN);
    [[maybe_unused]] char file_path[buff_size];
    strlcpy(file_path, folder, buff_size);
    make_dirs(file_path);
    strlcat(file_path, info.key_hash_str.data(), buff_size);

    // check if it already exists
    if (file_exists(file_path)) {
        debug_assert(false);
        return false;
    }

    unique_file_ptr file(fopen(file_path, "w"));
    if (!file) {
        return false;
    }

    if (fwrite(&key_info, sizeof(key_info), 1, file.get()) != 1) {
        return false;
    }
    return true;
}

void remove_identities_in(const char *in_path) {
    struct dirent *entry;
    char path[e2ee::IDENTITY_PATH_MAX_LEN];
    Directory dir(in_path);
    if (!dir) {
        //????
        return;
    }

    while ((entry = dir.read()) != nullptr) {
        // Ignore "." and ".." (current and parent directories)
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        strlcpy(path, in_path, e2ee::IDENTITY_PATH_MAX_LEN);
        strlcat(path, entry->d_name, e2ee::IDENTITY_PATH_MAX_LEN);

        if (entry->d_type == DT_REG) {
            remove(path);
        }
    }
}

} // namespace

namespace e2ee {

Pk::Pk() {
    mbedtls_pk_init(&pk);
}

Pk::~Pk() {
    mbedtls_pk_free(&pk);
}

void generate_key(AsyncJobExecutionControl &control, bool &result) {
    result = false;
    unique_ptr<uint8_t, FreeDeleter> buffer(reinterpret_cast<uint8_t *>(malloc_fallible(PRIVATE_KEY_BUFFER_SIZE)));
    if (!buffer) {
        return;
    }
    crash_dump::ManualSecret secret(buffer.get(), PRIVATE_KEY_BUFFER_SIZE);

    int export_res = 0;
    {
        struct KeyGenContexts contexts;

        const char *pers = "ecp_keypair";

        if (mbedtls_ctr_drbg_seed(&contexts.ctr_drbg, mbedtls_entropy_func, &contexts.entropy, (const unsigned char *)pers, strlen(pers)) != 0) {
            return;
        }

        mbedtls_rsa_context *rsa = mbedtls_pk_rsa(contexts.pk);
        if (mbedtls_rsa_gen_key(rsa, mbedtls_ctr_drbg_random, &contexts.ctr_drbg, 2048, 65537) != 0) {
            return;
        }

        export_res = mbedtls_pk_write_key_der(&contexts.pk, buffer.get(), PRIVATE_KEY_BUFFER_SIZE);
        if (export_res <= 0) {
            return;
        }
    } // Free the mbedtls contexts, better chance to have RAM for the file

    if (control.is_discarded()) {
        // We aborted the generation, so do not save the key
        return;
    }

    make_dirs(private_key_path);
    unique_file_ptr fout(fopen(private_key_path, "wb"));
    if (!fout) {
        return;
    }

    // Note: The mbedtls_pk_write_key_der writes to the _end_ of the buffer.
    if (fwrite(buffer.get() + PRIVATE_KEY_BUFFER_SIZE - export_res, export_res, 1, fout.get()) != 1) {
        return;
    }

    result = true;
    return;
}

bool export_key() {
    unique_ptr<uint8_t, FreeDeleter> buffer(reinterpret_cast<uint8_t *>(malloc_fallible(PRIVATE_KEY_BUFFER_SIZE)));
    if (!buffer) {
        return false;
    }
    crash_dump::ManualSecret secret(buffer.get(), PRIVATE_KEY_BUFFER_SIZE);

    unique_file_ptr inf(fopen(private_key_path, "rb"));
    if (!inf) {
        return false;
    }

    size_t ins = fread(buffer.get(), 1, PRIVATE_KEY_BUFFER_SIZE, inf.get());
    if (ins == 0 || ferror(inf.get()) || !feof(inf.get())) {
        return false;
    }

    inf.reset();

    int ret = 0;
    {
        Pk pk;
        if (mbedtls_pk_parse_key(&pk.pk, buffer.get(), ins, NULL /* No password */, 0) != 0) {
            return false;
        }

        ret = mbedtls_pk_write_pubkey_der(&pk.pk, buffer.get(), PRIVATE_KEY_BUFFER_SIZE);

        if (ret <= 0) {
            return false;
        }
    } // Destroy the pk

    unique_file_ptr outf(fopen(public_key_path, "wb"));
    if (!outf) {
        return false;
    }

    // Note: mbedtls writes to the _end_ of the buffer.
    if (fwrite(buffer.get() + PRIVATE_KEY_BUFFER_SIZE - ret, ret, 1, outf.get()) != 1) {
        outf.reset();
        // Result not checked - no way we can fail twice anyway.
        remove(public_key_path);
        return false;
    }

    return true;
}

void get_key_hash_string(char *out, [[maybe_unused]] size_t size, e2ee::Pk *pk) {
    debug_assert(size >= e2ee::KEY_HASH_STR_BUFFER_LEN);
    uint8_t key_hash[e2ee::HASH_SIZE];
    std::array<uint8_t, e2ee::PUBLIC_KEY_BUFFER_SIZE> buffer;
    int ret = mbedtls_pk_write_pubkey_der(&pk->pk, buffer.data(), e2ee::PUBLIC_KEY_BUFFER_SIZE);
    mbedtls_sha256_ret(buffer.data() + buffer.size() - ret, ret, key_hash, false);

    for (size_t i = 0; i < e2ee::HASH_SIZE; i++) {
        sprintf(&out[i * 2], "%02x", key_hash[i]);
    }
}

bool save_identity_key(const IdentityInfo &info) {
    return save_identity_key_impl(info, identities_folder);
}

bool save_identity_key_temporary(const IdentityInfo &info) {
    return save_identity_key_impl(info, identities_tmp_folder);
}

void remove_trusted_identity(const IdentityInfo &info) {
    char file_path[IDENTITY_PATH_LEN];
    strlcpy(file_path, identities_folder, IDENTITY_PATH_LEN);
    strlcat(file_path, info.key_hash_str.data(), IDENTITY_PATH_LEN);
    debug_assert(file_exists(file_path));
    remove(file_path);
}

void remove_temporary_identites() {
    remove_identities_in(identities_tmp_folder);
}

void remove_key() {
    // We ignore non-existence.
    remove(private_key_path);
}

void remove_all_identities() {
    remove_temporary_identites();
    remove_identities_in(identities_folder);
}

bool is_trusted_identity(const IdentityInfo &info) {
    char file_path[e2ee::IDENTITY_PATH_MAX_LEN];
    // do we trust it permanently?
    strlcpy(file_path, identities_folder, e2ee::IDENTITY_PATH_MAX_LEN);
    strlcat(file_path, info.key_hash_str.data(), e2ee::IDENTITY_PATH_MAX_LEN);
    bool trusted = file_exists(file_path);
    if (!trusted) {
        // or at least temporarily
        strlcpy(file_path, identities_tmp_folder, e2ee::IDENTITY_PATH_MAX_LEN);
        strlcat(file_path, info.key_hash_str.data(), e2ee::IDENTITY_PATH_MAX_LEN);
        trusted = file_exists(file_path);
    }
    return trusted;
}

} // namespace e2ee

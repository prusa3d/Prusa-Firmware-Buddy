#include "resources/hash.hpp"
#include "fileutils.hpp"
#include "sha256.h"
#include <memory>
#include <optional>

using namespace buddy::resources;
LOG_COMPONENT_REF(Resources);

class HashContext {
private:
    mbedtls_sha256_context sha;
    uint32_t counter;

public:
    HashContext()
        : sha()
        , counter(0) {
        mbedtls_sha256_init(&sha);
        mbedtls_sha256_starts_ret(&sha, 0);
    }

    void append_mark() {
        mbedtls_sha256_update_ret(&sha, (unsigned char *)&counter, sizeof(counter));
        counter += 1;
    }

    void append_data(const uint8_t *data, size_t length) {
        mbedtls_sha256_update_ret(&sha, data, length);
    }

    void finish(Hash &output) {
        mbedtls_sha256_finish_ret(&sha, &output[0]);
    }

    ~HashContext() {
        mbedtls_sha256_free(&sha);
    }
};

static bool update_hash_with_file(const char *root, Path &path, HashContext &hash_ctx) {
    // mark
    hash_ctx.append_mark();

    // filepath
    size_t root_length = strlen(root);
    const char *full_path = path.get();
    const char *filepath = full_path + root_length;
    hash_ctx.append_data((uint8_t *)filepath, strlen(filepath));

    // mark
    hash_ctx.append_mark();

    // filedata
    std::unique_ptr<FILE, FILEDeleter> source(fopen(path.get(), "rb"));
    if (source.get() == nullptr) {
        log_error(Resources, "Failed to open file %s", source.get());
        return false;
    }
    uint8_t buffer[128];
    bool success;
    while (true) {
        int read = fread(buffer, 1, sizeof(buffer), source.get());
        if (read > 0) {
            hash_ctx.append_data(buffer, read);
        }
        if (ferror(source.get())) {
            success = false;
            break;
        }
        if (feof(source.get())) {
            success = true;
            break;
        }
    }

    // mark
    hash_ctx.append_mark();

    return success;
}

static bool update_hash_with_directory(const char *root, Path &path, HashContext &hash_ctx) {
    std::optional<long> last_dir_location;

    // mark
    hash_ctx.append_mark();

    // filepath
    size_t root_length = strlen(root);
    const char *full_path = path.get();
    const char *filepath = full_path + root_length;
    if (strlen(filepath) == 0) {
        filepath = "/";
    }
    hash_ctx.append_data((uint8_t *)filepath, strlen(filepath));

    // mark
    hash_ctx.append_mark();

    while (true) {
        // get info about the next item in the directory
        std::unique_ptr<DIR, DIRDeleter> dir(opendir(path.get()));
        if (last_dir_location.has_value()) {
            seekdir(dir.get(), last_dir_location.value());
        }
        struct dirent *entry = readdir(dir.get());
        if (!entry) {
            break;
        }

        // save current position
        last_dir_location = telldir(dir.get());

        // skip the entry immediately if "." or ".."
        if (entry->d_type == DT_DIR && (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)) {
            continue;
        }

        // save info and close the dir to save resources
        path.push(entry->d_name);
        auto d_type = entry->d_type;
        dir.reset();

        // hash the item
        bool success;
        if (d_type == DT_REG) {
            success = update_hash_with_file(root, path, hash_ctx);
        } else if (d_type == DT_DIR) {
            success = update_hash_with_directory(root, path, hash_ctx);
        } else {
            success = true;
        }

        // restore previous state
        path.pop();

        if (!success) {
            break;
        }
    }

    // mark
    hash_ctx.append_mark();

    return true;
}

bool buddy::resources::calculate_directory_hash(Hash &hash, const char *root) {
    HashContext sha_ctx;
    const Path root_dir(root);
    Path current_path(root);

    log_info(Resources, "Starting to calculate hash of %s", root);

    bool success = update_hash_with_directory(root_dir.get(), current_path, sha_ctx);

    if (success) {
        sha_ctx.finish(hash);
        log_info(Resources, "Hash calculation finished (result %02X%02X%02X%02X%02X%02X...)", hash[0], hash[1], hash[2], hash[3], hash[4], hash[5]);
    }

    return success;
}

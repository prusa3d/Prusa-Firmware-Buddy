#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <memory>
#include <optional>
#include <cerrno>
#include <sys/stat.h>
#include <sys/iosupport.h>
#include <unistd.h>

#include "bsod.h"
#include "bbf.hpp"
#include <common/sys.hpp>
#include <logging/log.hpp>
#include <freertos/critical_section.hpp>
#include <buddy/bootstrap_state.hpp>
#include "timing.h"
#include "cmsis_os.h"

#include <directory.hpp>
#include <unique_file_ptr.hpp>

#include "semihosting/semihosting.hpp"
#include "resources/bootstrap.hpp"
#include "resources/hash.hpp"
#include "resources/tarball.hpp"
#include <raii/scope_guard.hpp>
#include <sha256.h>

#include <common/mutable_path.hpp>

using Path = MutablePath;

LOG_COMPONENT_DEF(Resources, logging::Severity::debug);
using buddy::BootstrapStage;

static bool has_bbf_suffix(const char *fname) {
    char *dot = strrchr(fname, '.');

    if (dot == nullptr) {
        return 0;
    }

    return strcasecmp(dot, ".bbf") == 0;
}

static bool is_relevant_bbf_for_bootstrap(FILE *bbf, const char *path, const buddy::resources::Revision &revision, buddy::bbf::TLVType tlv_entry) {
    uint32_t hash_len;
    if (!buddy::bbf::seek_to_tlv_entry(bbf, tlv_entry, hash_len)) {
        log_error(Resources, "Failed to seek to resources revision %s", path);
        return false;
    }

    buddy::resources::Revision bbf_revision;
    if (fread(&bbf_revision.hash[0], 1, revision.hash.size(), bbf) != hash_len) {
        log_error(Resources, "Failed to read resources revision %s", path);
        return false;
    }

    return bbf_revision == revision;
}

[[nodiscard]] static bool remove_directory_recursive(Path &path) {
    while (true) {
        errno = 0;
        // get info about the next item in the directory
        Directory dir { path.get() };

        if (!dir) {
            return false;
        }

        struct dirent *entry = dir.read();
        if (!entry && errno != 0) {
            return false;
        }

        // skip the entry immediately if "." or ".."
        while (entry && ((strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0))) {
            entry = dir.read();
        }

        // is the dir already empty?
        if (!entry) {
            break;
        }

        // save info and close the dir to save resources
        path.push(entry->d_name);
        auto d_type = entry->d_type;
        dir.close();

        // remove the item
        bool success;
        if (d_type == DT_REG) {
            log_info(Resources, "Removing file %s", path.get());
            success = remove(path.get()) == 0;
        } else if (d_type == DT_DIR) {
            log_info(Resources, "Removing directory %s", path.get());
            success = remove_directory_recursive(path);
        } else {
            log_warning(Resources, "Unexpected item %hhu: %s; skipping", d_type, path.get());
            success = true;
        }

        // restore previous state
        path.pop();

        if (!success) {
            log_error(Resources, "Error when removing directory %s (errno %i)", path.get(), errno);
            return false;
        }
    }

    // and finally, remove the directory itself
    return remove(path.get()) == 0;
}

static bool remove_recursive_if_exists(Path &path) {
    struct stat sb;
    int stat_retval = stat(path.get(), &sb);

    // does it exists?
    if (stat_retval == -1 && errno == ENOENT) {
        return true;
    }

    // something went wrong
    if (stat_retval == -1) {
        return false;
    }

    if (S_ISREG(sb.st_mode)) {
        return remove(path.get()) == 0;
    } else if (S_ISDIR(sb.st_mode)) {
        return remove_directory_recursive(path);
    } else {
        return false;
    }
}

static bool bootstrap_over_debugger_possible() {
    // debugger flag is located at the beginning of the CCMRAM
    // to enable bootstrap over debugger, the debugger has to set the flag to 0xABCDABCD
    static uint32_t debugger_flag __attribute__((__section__(".ccmram_beginning"), used));

    bool flag_set = debugger_flag == 0xABCDABCD;
    bool debugger_connected = sys_debugger_attached();

    return debugger_connected && flag_set;
}

static FILE *open_bbf_over_debugger(Path &path_buffer, const buddy::resources::Revision &revision, buddy::bbf::TLVType &bbf_entry) {
    // find the path first
    auto retval = semihosting::sys_getcmdline(path_buffer.get_buffer(), path_buffer.maximum_length());
    if (retval != 0) {
        return nullptr;
    }
    const char *first_space = strchr(path_buffer.get(), ' ');
    if (first_space == nullptr) {
        return nullptr;
    }
    const char *filepath = first_space + 1;
    log_warning(Resources, "BBF over debugger filename: %s", filepath);

    FILE *bbf = nullptr;
    {
        MutablePath path;
        path.push("semihosting");
        path.push(filepath);
        bbf = fopen(path.get(), "rb");
    }
    if (bbf == nullptr) {
        return nullptr;
    }

    if (is_relevant_bbf_for_bootstrap(bbf, filepath, revision, buddy::bbf::TLVType::RESOURCES_TARBALL_DIGEST)) {
        log_info(Resources, "Found suitable bbf provided by debugger: %s", filepath);
        bbf_entry = buddy::bbf::TLVType::RESOURCES_TARBALL;
        return bbf;
    } else if (is_relevant_bbf_for_bootstrap(bbf, filepath, revision, buddy::bbf::TLVType::BOOTLOADER_TARBALL_DIGEST)) {
        log_info(Resources, "Found suitable bbf provided by debugger: %s", filepath);
        bbf_entry = buddy::bbf::TLVType::BOOTLOADER_TARBALL;
        return bbf;
    } else {
        fclose(bbf);
        return nullptr;
    }
}

static bool find_suitable_bbf_file(const buddy::resources::Revision &revision, Path &bbf, buddy::bbf::TLVType &bbf_entry) {
    log_debug(Resources, "Searching for a bbf...");

    // open the directory
    Directory dir { "/usb" };
    if (!dir) {
        log_warning(Resources, "Failed to open /usb directory");
        return false;
    }

    // locate bbf file
    bool bbf_found = false;
    struct dirent *entry;
    while ((entry = dir.read())) {
        // check is bbf
        if (!has_bbf_suffix(entry->d_name)) {
            log_debug(Resources, "Skipping file: %s (bad suffix)", entry->d_name);
            continue;
        }
        // create full path
        bbf.set("/usb");
        bbf.push(entry->d_name);
        // open the bbf
        unique_file_ptr bbf_file(fopen(bbf.get(), "rb"));
        if (bbf_file.get() == nullptr) {
            log_error(Resources, "Failed to open %s", bbf.get());
            continue;
        }

        // check if the file contains required resources
        if (is_relevant_bbf_for_bootstrap(bbf_file.get(), bbf.get(), revision, buddy::bbf::TLVType::RESOURCES_TARBALL_DIGEST)) {
            log_info(Resources, "Found suitable bbf for bootstraping: %s", bbf.get());
            bbf_found = true;
            bbf_entry = buddy::bbf::TLVType::RESOURCES_TARBALL;
            break;
        } else if (is_relevant_bbf_for_bootstrap(bbf_file.get(), bbf.get(), revision, buddy::bbf::TLVType::BOOTLOADER_TARBALL_DIGEST)) {
            log_info(Resources, "Found suitable bbf for bootstraping: %s", bbf.get());
            bbf_found = true;
            bbf_entry = buddy::bbf::TLVType::BOOTLOADER_TARBALL;
            break;
        } else {
            log_info(Resources, "Skipping file: %s (not compatible)", bbf.get());
            continue;
        }
    }

    if (!bbf_found) {
        log_warning(Resources, "Failed to find a .bbf file");
        return false;
    }

    return true;
}

[[nodiscard]] static bool extract_and_hash(int payload_fd, uint32_t length, std::span<uint8_t> tar_buffer, buddy::resources::Hash &out_hash) {
    buddy::bootstrap_state_set(0, buddy::BootstrapStage::copying_files);

    struct {
        mbedtls_sha256_context sha;
        uint32_t length;
        uint32_t extracted = 0;
        unsigned last_percent = 0;
    } state;
    state.length = length;

    mbedtls_sha256_init(&state.sha);
    ScopeGuard free_sha = [&] { mbedtls_sha256_free(&state.sha); };
    mbedtls_sha256_starts_ret(&state.sha, 0);

    const auto on_data = [&state](std::span<const uint8_t> data) {
        mbedtls_sha256_update_ret(&state.sha, data.data(), data.size());

        state.extracted += data.size();
        const unsigned percent = state.extracted * 100 / state.length;
        if (percent != state.last_percent) {
            buddy::bootstrap_state_set(percent, buddy::BootstrapStage::copying_files);
            state.last_percent = percent;
        }
    };
    if (!buddy::resources::tarball::extract(payload_fd, length, tar_buffer, on_data)) {
        return false;
    }

    mbedtls_sha256_finish_ret(&state.sha, out_hash.data());
    return true;
}

static bool do_bootstrap(const buddy::resources::Revision &revision) {
    Path source_path("/");
    unique_file_ptr bbf;
    buddy::bbf::TLVType bbf_entry = buddy::bbf::TLVType::RESOURCES_TARBALL;

    bootstrap_state_set(0, BootstrapStage::looking_for_bbf); // initial report

    // try to find required BBF on attached USB drive
    if (find_suitable_bbf_file(revision, source_path, bbf_entry)) {
        bbf.reset(fopen(source_path.get(), "rb"));
    }

    // try to open BBF supplied over semihosting (connected debugger)
    if (bbf.get() == nullptr && bootstrap_over_debugger_possible()) {
        bbf.reset(open_bbf_over_debugger(source_path, revision, bbf_entry));
    }

    if (bbf.get() == nullptr) {
        return false;
    }

    bootstrap_state_set(0, BootstrapStage::preparing_bootstrap);

    // use a small buffer for the BBF
    setvbuf(bbf.get(), NULL, _IOFBF, 32);

    uint32_t length;
    if (!buddy::bbf::seek_to_tlv_entry(bbf.get(), bbf_entry, length)) {
        log_error(Resources, "Failed to find payload in BBF");
        return false;
    }

    // Realign the raw fd to the stream's logical position, we are not using stdio
    // to circumvent the buffering.
    const int payload_fd = fileno(bbf.get());
    const long payload_start = ftell(bbf.get());
    if (payload_start < 0 || lseek(payload_fd, payload_start, SEEK_SET) != payload_start) {
        log_error(Resources, "Failed to align BBF fd for payload read (errno %i)", errno);
        return false;
    }

    // clear installed resources
    Path target_path("/internal/res");
    buddy::resources::InstalledRevision::clear();
    if (remove_recursive_if_exists(target_path) == false) {
        log_error(Resources, "Failed to remove the /internal/res directory");
        return false;
    }

    // Larger the buffer, faster the copy, with diminishing returns
    constexpr size_t tar_buffer_size = 32 * buddy::resources::tarball::block_size;
    auto tar_buffer = std::make_unique<uint8_t[]>(tar_buffer_size);

    // copy the resources and compute hash
    buddy::resources::Hash current_hash;
    if (!extract_and_hash(payload_fd, length, { tar_buffer.get(), tar_buffer_size }, current_hash)) {
        log_error(Resources, "Failed to copy resources");
        return false;
    }

    if (revision.hash != current_hash) {
        log_error(Resources, "Installed resources but the hash does not match!");
        return false;
    }

    // save the installed revision
    if (!buddy::resources::InstalledRevision::set(revision)) {
        log_error(Resources, "Failed to save installed resources revision");
        return false;
    }

    return true;
}

bool buddy::resources::bootstrap(const buddy::resources::Revision &revision) {
    while (true) {
        bool success = do_bootstrap(revision);
        if (success) {
            log_info(Resources, "Bootstrap successful");
            return true;
        } else {
            log_info(Resources, "Bootstrap failed. Retrying in a moment...");
            osDelay(1000);
        }
    }
}

bool buddy::resources::has_resources(const Revision &revision) {
    Revision installed;
    if (buddy::resources::InstalledRevision::fetch(installed) == false) {
        return false;
    }

    return installed == revision;
}

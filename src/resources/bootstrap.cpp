#include <dirent.h>
#include <memory>
#include <string.h>
#include <optional>
#include <cerrno>
#include <sys/stat.h>

#include "bsod.h"
#include "bbf.hpp"
#include "log.h"
#include "timing.h"
#include "cmsis_os.h"

#include "resources/bootstrap.hpp"
#include "resources/hash.hpp"

#include "fileutils.hpp"

LOG_COMPONENT_DEF(Resources, LOG_SEVERITY_DEBUG);
using BootstrapStage = buddy::resources::BootstrapStage;

struct ResourcesScanResult {
    unsigned files_count;
    unsigned directories_count;
    unsigned total_size_of_files;

    ResourcesScanResult()
        : files_count(0)
        , directories_count(0)
        , total_size_of_files(0) {}
};

static bool scan_resources_folder(Path &path, ResourcesScanResult &result) {
    std::optional<long> last_dir_location;

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

        // copy the item
        bool success;
        if (d_type == DT_REG) {
            // copy file
            result.files_count += 1;
            struct stat fs;
            success = stat(path.get(), &fs) == 0;
            result.total_size_of_files += fs.st_size;
        } else if (d_type == DT_DIR) {
            result.directories_count += 1;
            success = scan_resources_folder(path, result);
        } else {
            success = true;
        }

        // restore previous state
        path.pop();

        if (!success) {
            break;
        }
    }

    return true;
}

class BootstrapProgressReporter {
private:
    buddy::resources::ProgressHook progress_hook;
    std::optional<ResourcesScanResult> scan_result;
    BootstrapStage current_stage;
    unsigned files_copied;
    unsigned directories_copied;
    unsigned bytes_copied;
    std::optional<unsigned> last_reported_percent_done;

    unsigned percent_done() {
        if (scan_result.has_value() == false) {
            return false;
        }
        const unsigned points_per_file = 100;
        const unsigned points_per_directory = 100;
        const unsigned points_per_byte = 1;

        unsigned total_points = scan_result->files_count * points_per_file
            + scan_result->directories_count * points_per_directory
            + scan_result->total_size_of_files * points_per_byte;
        unsigned finished_points = files_copied * points_per_file
            + directories_copied * points_per_directory
            + bytes_copied * points_per_byte;

        return finished_points * 100 / total_points;
    }

public:
    BootstrapProgressReporter(buddy::resources::ProgressHook progress_hook, BootstrapStage stage)
        : progress_hook(progress_hook)
        , scan_result()
        , current_stage(stage)
        , files_copied(0)
        , directories_copied(0)
        , bytes_copied(0)
        , last_reported_percent_done(std::nullopt) {
    }

    void report() {
        progress_hook(percent_done(), current_stage);
    }

    void update_stage(BootstrapStage stage) {
        current_stage = stage;
        report();
    }

    void report_percent_done() {
        unsigned percent_done = this->percent_done();
        if (last_reported_percent_done != percent_done) {
            progress_hook(percent_done, current_stage);
            last_reported_percent_done = percent_done;
        }
    }

    void assign_scan_result(ResourcesScanResult result) {
        scan_result = result;
    }

    void reset() {
        files_copied = directories_copied = bytes_copied = 0;
        scan_result.reset();
        last_reported_percent_done = std::nullopt;
    }

    void did_copy_file() {
        files_copied += 1;
        report_percent_done();
    }

    void did_copy_directory() {
        directories_copied += 1;
        report_percent_done();
    }

    void did_copy_bytes(unsigned bytes) {
        bytes_copied += bytes;
        report_percent_done();
    }
};

static bool has_bbf_suffix(const char *fname) {
    char *dot = strrchr(fname, '.');

    if (dot == nullptr) {
        return 0;
    }

    return strcasecmp(dot, ".bbf") == 0;
}

static bool is_relevant_bbf_for_bootstrap(const char *fname, const buddy::resources::Revision &revision, buddy::bbf::TLVType tlv_entry) {
    std::unique_ptr<FILE, FILEDeleter> bbf(fopen(fname, "rb"));
    if (bbf.get() == nullptr) {
        log(LOG_SEVERITY_ERROR, "Failed to open %s", fname);
        return false;
    }

    uint32_t hash_len;
    if (!buddy::bbf::seek_to_tlv_entry(bbf.get(), tlv_entry, hash_len)) {
        log(LOG_SEVERITY_ERROR, "Failed to seek to resources revision %s", fname);
        return false;
    }

    buddy::resources::Revision bbf_revision;
    if (fread(&bbf_revision.hash[0], 1, revision.hash.size(), bbf.get()) != hash_len) {
        log(LOG_SEVERITY_ERROR, "Failed to read resources revision %s", fname);
        return false;
    }

    return bbf_revision == revision;
}

static bool copy_file(const Path &source_path, const Path &target_path, BootstrapProgressReporter &reporter) {
    std::unique_ptr<FILE, FILEDeleter> source(fopen(source_path.get(), "rb"));
    if (source.get() == nullptr) {
        log(LOG_SEVERITY_ERROR, "Failed to open file %s", source.get());
        return false;
    }

    std::unique_ptr<FILE, FILEDeleter> target(fopen(target_path.get(), "wb"));
    if (target.get() == nullptr) {
        log(LOG_SEVERITY_ERROR, "Failed to open file for writing %s", target.get());
        return false;
    }

    uint8_t buffer[128];

    while (true) {
        int read = fread(buffer, 1, sizeof(buffer), source.get());
        if (read > 0) {
            int written = fwrite(buffer, 1, read, target.get());
            if (read != written) {
                log(LOG_SEVERITY_ERROR, "Writing while copying failed");
                return false;
            }
            reporter.did_copy_bytes(written);
        }
        if (ferror(source.get())) {
            return false;
        }
        if (feof(source.get())) {
            return true;
        }
    }
}

[[nodiscard]] static bool remove_directory_recursive(Path &path) {
    while (true) {
        errno = 0;
        // get info about the next item in the directory
        std::unique_ptr<DIR, DIRDeleter> dir(opendir(path.get()));

        if (dir.get() == nullptr) {
            return false;
        }

        struct dirent *entry = readdir(dir.get());
        if (!entry && errno != 0) {
            return false;
        }

        // skip the entry immediately if "." or ".."
        while (entry && ((strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0))) {
            entry = readdir(dir.get());
        }

        // is the dir already empty?
        if (!entry) {
            break;
        }

        // save info and close the dir to save resources
        path.push(entry->d_name);
        auto d_type = entry->d_type;
        dir.reset();

        // remove the item
        bool success;
        if (d_type == DT_REG) {
            // copy file
            log(LOG_SEVERITY_INFO, "Removing file %s", path.get());
            success = remove(path.get()) == 0;
        } else if (d_type == DT_DIR) {
            log(LOG_SEVERITY_INFO, "Removing directory %s", path.get());
            success = remove_directory_recursive(path);
        } else {
            log(LOG_SEVERITY_WARNING, "Unexpected item %hhu: %s; skipping", d_type, path.get());
            success = true;
        }

        // restore previous state
        path.pop();

        if (!success) {
            log(LOG_SEVERITY_ERROR, "Error when removing directory %s (errno %i)", path.get(), errno);
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

static bool copy_resources_directory(Path &source, Path &target, BootstrapProgressReporter &reporter) {
    std::optional<long> last_dir_location;

    // ensure target directory exists
    mkdir(target.get(), 0700);

    while (true) {
        errno = 0;
        // get info about the next item in the directory
        std::unique_ptr<DIR, DIRDeleter> dir(opendir(source.get()));

        if (dir.get() == nullptr) {
            return false;
        } else if (last_dir_location.has_value()) {
            seekdir(dir.get(), last_dir_location.value());
            if (errno != 0) {
                log(LOG_SEVERITY_ERROR, "seekdir() failed: %i", errno);
                return false;
            }
        }

        struct dirent *entry = readdir(dir.get());
        if (!entry && errno != 0) {
            return false;
        } else if (!entry) {
            break;
        }

        // save current position
        last_dir_location = telldir(dir.get());
        if (errno != 0) {
            return false;
        }

        // skip the entry immediately if "." or ".."
        if (entry->d_type == DT_DIR && (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)) {
            continue;
        }

        // save info and close the dir to save resources
        source.push(entry->d_name);
        target.push(entry->d_name);
        auto d_type = entry->d_type;
        dir.reset();

        // copy the item
        bool success;
        if (d_type == DT_REG) {
            // copy file
            log(LOG_SEVERITY_INFO, "Copying file %s", source.get());
            success = copy_file(source, target, reporter);
            reporter.did_copy_file();
        } else if (d_type == DT_DIR) {
            log(LOG_SEVERITY_INFO, "Copying directory %s", source.get());
            success = copy_resources_directory(source, target, reporter);
            reporter.did_copy_directory();
        } else {
            log(LOG_SEVERITY_WARNING, "Unexpected item %hhu: %s; skipping", d_type, source.get());
            success = true;
        }

        // restore previous state
        source.pop();
        target.pop();

        if (!success) {
            return false;
        }
    }

    return true;
}

static bool find_suitable_bbf_file(const buddy::resources::Revision &revision, Path &bbf, buddy::bbf::TLVType &bbf_entry) {
    log(LOG_SEVERITY_DEBUG, "Searching for a bbf...");

    // open the directory
    std::unique_ptr<DIR, DIRDeleter> dir(opendir("/usb"));
    if (dir.get() == nullptr) {
        log(LOG_SEVERITY_WARNING, "Failed to open /usb directory");
        return false;
    }

    // locate bbf file
    bool bbf_found = false;
    struct dirent *entry;
    while ((entry = readdir(dir.get()))) {
        // check is bbf
        if (!has_bbf_suffix(entry->d_name)) {
            log(LOG_SEVERITY_DEBUG, "Skipping file: %s (bad suffix)", entry->d_name);
            continue;
        }
        // create full path
        bbf.set("/usb");
        bbf.push(entry->d_name);
        // check if the file contains required resources
        if (is_relevant_bbf_for_bootstrap(bbf.get(), revision, buddy::bbf::TLVType::RESOURCES_IMAGE_HASH)) {
            log(LOG_SEVERITY_INFO, "Found suitable bbf for bootstraping: %s", bbf.get());
            bbf_found = true;
            bbf_entry = buddy::bbf::TLVType::RESOURCES_IMAGE;
            break;
        } else if (is_relevant_bbf_for_bootstrap(bbf.get(), revision, buddy::bbf::TLVType::RESOURCES_BOOTLOADER_IMAGE_HASH)) {
            log(LOG_SEVERITY_INFO, "Found suitable bbf for bootstraping: %s", bbf.get());
            bbf_found = true;
            bbf_entry = buddy::bbf::TLVType::RESOURCES_BOOTLOADER_IMAGE;
            break;
        } else {
            log(LOG_SEVERITY_INFO, "Skipping file: %s (not compatible)", bbf.get());
            continue;
        }
    }

    if (!bbf_found) {
        log(LOG_SEVERITY_WARNING, "Failed to find a .bbf file");
        return false;
    }

    return true;
}

static bool do_bootstrap(const buddy::resources::Revision &revision, buddy::resources::ProgressHook progress_hook) {
    BootstrapProgressReporter reporter(progress_hook, BootstrapStage::LookingForBbf);
    Path source_path("/");

    reporter.report(); // initial report
    buddy::bbf::TLVType bbf_entry;
    if (!find_suitable_bbf_file(revision, source_path, bbf_entry)) {
        return false;
    }

    reporter.update_stage(BootstrapStage::PreparingBootstrap);

    // open the bbf
    std::unique_ptr<FILE, FILEDeleter> bbf(fopen(source_path.get(), "rb"));
    if (bbf.get() == nullptr) {
        log(LOG_SEVERITY_WARNING, "Failed to open %s", source_path.get());
        return false;
    }

    // mount the filesystem stored in the bbf
    ScopedFileSystemLittlefsBBF scoped_bbf_mount(bbf.get(), bbf_entry);
    if (!scoped_bbf_mount.mounted()) {
        log(LOG_SEVERITY_INFO, "BBF mounting failed");
        return false;
    }

    // calculate stats for better progress reporting
    ResourcesScanResult scan_result;
    source_path.set("/bbf");
    if (!scan_resources_folder(source_path, scan_result)) {
        log(LOG_SEVERITY_ERROR, "Failed to scan the /bbf directory");
    }
    log(LOG_SEVERITY_INFO, "To copy: %i files, %i directories, %i bytes",
        scan_result.files_count, scan_result.directories_count, scan_result.total_size_of_files);
    reporter.assign_scan_result(scan_result);

    // open the bbf's root dir
    std::unique_ptr<DIR, DIRDeleter> dir(opendir("/bbf"));
    if (dir.get() == nullptr) {
        log(LOG_SEVERITY_WARNING, "Failed to open /bbf directory");
        return false;
    }

    // clear installed resources
    Path target_path("/internal/res");
    buddy::resources::InstalledRevision::clear();
    if (remove_recursive_if_exists(target_path) == false) {
        log(LOG_SEVERITY_ERROR, "Failed to remove the /internal/res directory");
        return false;
    }

    // copy the resources
    reporter.update_stage(BootstrapStage::CopyingFiles);
    source_path.set("/bbf");
    if (!copy_resources_directory(source_path, target_path, reporter)) {
        log(LOG_SEVERITY_ERROR, "Failed to copy resources");
        return false;
    }

    // calculate the hash of the resources we just installed
    buddy::resources::Hash current_hash;
    if (!buddy::resources::calculate_directory_hash(current_hash, "/internal/res")) {
        log(LOG_SEVERITY_ERROR, "Failed to calculate hash of /internal/res directory");
        return false;
    }

    if (revision.hash != current_hash) {
        log(LOG_SEVERITY_ERROR, "Installed resources but the hash does not match!");
        return false;
    }

    // save the installed revision
    if (!buddy::resources::InstalledRevision::set(revision)) {
        log(LOG_SEVERITY_ERROR, "Failed to save installed resources revision");
        return false;
    }

    return true;
}

bool buddy::resources::bootstrap(const buddy::resources::Revision &revision, ProgressHook progress_hook) {
    while (true) {
        bool success = do_bootstrap(revision, progress_hook);
        if (success) {
            log(LOG_SEVERITY_INFO, "Bootstrap successful");
            return true;
        } else {
            log(LOG_SEVERITY_INFO, "Bootstrap failed. Retrying in a moment...");
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

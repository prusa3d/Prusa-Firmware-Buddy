#include "transfer.hpp"

#include <string.h>
#include <memory>
#include <sys/stat.h>

#include <logging/log.hpp>
#include <version/version.hpp>
#include <common/bsod.h>
#include <common/crc32.h>
#include <common/unique_dir_ptr.hpp>
#include <common/filename_type.hpp>
#include <common/filepath_operation.h>
#include <lang/i18n.h>

LOG_COMPONENT_REF(transfers);
using namespace transfers;

namespace {

using FileOffset = long;

struct SerializedString {
    long offset = 0;
    size_t size = 0;

    static SerializedString serialize(const char *str, FILE *file, bool &error) {
        const size_t len = strlen(str) + 1;
        if (fwrite(str, len, 1, file) != 1) {
            error = true;
        }
        return SerializedString { ftell(file) - static_cast<long>(len), len };
    }

    template <typename T>
    const char *deserialize(T get_data_ptr) const {
        return static_cast<const char *>(get_data_ptr(offset, size));
    }
};

struct SerializedTransfer {
    // what data are valid
    PartialFile::State partial_file_state;

    // crc of the partial file state only
    uint32_t partial_file_state_crc;

    // for compatibility check
    SerializedString fw_version;

    // basic url info
    SerializedString host;
    uint16_t port = 0;
    SerializedString url_path_or_hash;
    // Is this an inline transfer (running inside a websocket?)
    //
    // In such case, we use hash instead of url path, but we add the team_id below.
    bool is_inline;
    uint64_t team_id = 0;
    uint32_t orig_size = 0;

    uint32_t transfer_id;

    // encryption info
    std::optional<Download::EncryptionInfo> encryption_info;

    // raw data referenced by the above structure continue after this
    // at the end of the raw data, there is a crc32 of the range [fw_version, end_of_file)
};

using EndOfDataCRC = uint32_t;

} // namespace

bool Transfer::make_backup(FILE *file, const Download::Request &request, const PartialFile::State &state, const Monitor::Slot &slot) {
    SerializedTransfer transfer;
    // Initialize all the bits, so even the padding ones are defined and the CRC works.
    memset((void *)&transfer, 0, sizeof(SerializedTransfer));

    bool error = false;
    fseek(file, sizeof(SerializedTransfer), SEEK_SET);

    transfer.partial_file_state = state;
    transfer.partial_file_state_crc = crc32_calc((const uint8_t *)&state, sizeof(state));
    transfer.fw_version = SerializedString::serialize(version::project_version_full, file, error);
    if (const auto *encrypted = get_if<Download::Request::Encrypted>(&request.data); encrypted != nullptr) {
        transfer.host = SerializedString::serialize(encrypted->host, file, error);
        transfer.port = encrypted->port;
        transfer.url_path_or_hash = SerializedString::serialize(encrypted->url_path, file, error);
        transfer.is_inline = false;

        // encryption
        if (encrypted->encryption.get()) {
            transfer.encryption_info = *encrypted->encryption;
        }
    } else {
        const auto &in = get<Download::Request::Inline>(request.data);
        transfer.url_path_or_hash = SerializedString::serialize(in.hash, file, error);
        transfer.team_id = in.team_id;
        transfer.orig_size = in.orig_size;
        transfer.is_inline = true;
    }
    // Must be available, we are holding the slot.
    transfer.transfer_id = slot.id();

    if (error) {
        log_error(transfers, "error while serializing backup");
        return false;
    }

    if (fseek(file, 0, SEEK_SET) != 0 || fwrite(&transfer, sizeof(transfer), 1, file) != 1) {
        log_error(transfers, "error while writing backup");
        return false;
    }

    // crc32 of the range [fw_version, end_of_file)
    if (fseek(file, offsetof(SerializedTransfer, fw_version), SEEK_SET) != 0) {
        log_error(transfers, "error while seeking to crc32");
        return false;
    }
    EndOfDataCRC crc = 0;

    while (true) {
        auto byte = fgetc(file);
        if (byte == EOF && feof(file)) {
            break;
        } else if (byte == EOF) {
            return false;
        } else {
            crc = crc32_calc_ex(crc, (uint8_t *)&byte, 1);
        }
    }

    fseek(file, 0, SEEK_END);
    if (fwrite(&crc, sizeof(crc), 1, file) != 1) {
        return false;
    }

    return true;
}

bool Transfer::update_backup(FILE *file, const PartialFile::State &state) {
    if (fseek(file, offsetof(SerializedTransfer, partial_file_state), SEEK_SET) != 0) {
        return false;
    }
    if (fwrite(&state, sizeof(state), 1, file) != 1) {
        return false;
    }
    uint32_t new_crc = crc32_calc((const uint8_t *)&state, sizeof(state));
    if (fwrite(&new_crc, sizeof(new_crc), 1, file) != 1) {
        return false;
    }
    return true;
}

std::optional<Transfer::RestoredTransfer> Transfer::restore(FILE *file) {
    if (fseek(file, 0, SEEK_SET) != 0) {
        return std::nullopt;
    }
    SerializedTransfer transfer;
    // Initialize all the bits, so even the padding ones are defined and the CRC works.
    memset((void *)&transfer, 0, sizeof(SerializedTransfer));
    if (fread(&transfer, sizeof(transfer), 1, file) != 1) {
        return std::nullopt;
    }

    // check crc of the partial file state
    {
        uint32_t crc = crc32_calc((const uint8_t *)&transfer.partial_file_state, sizeof(transfer.partial_file_state));
        if (crc != transfer.partial_file_state_crc) {
            log_error(transfers, "crc mismatch while restoring transfer");
            return std::nullopt;
        }
    }

    // check fw version
    {
        if (fseek(file, transfer.fw_version.offset, SEEK_SET) != 0) {
            return std::nullopt;
        }

        // Fail if even version lengths differ
        const size_t project_version_full_size = strlen(version::project_version_full);
        if (transfer.fw_version.size != project_version_full_size + 1) { // +1 for terminating 0 byte not included in strlen
            return std::nullopt;
        }

        // Check version string byte by byte as its size and content cannot be trusted (not covered by CRC that we just checked)
        for (size_t pos = 0; pos < transfer.fw_version.size && pos < project_version_full_size; pos++) {
            char c;
            if (fread(&c, 1, 1, file) != 1) {
                return std::nullopt;
            }
            if (c != version::project_version_full[pos]) {
                return std::nullopt;
            }
        }
    }

    return RestoredTransfer(file, transfer.partial_file_state, transfer.transfer_id);
}

std::variant<Transfer::Error, Transfer::Complete, PartialFile::State> Transfer::load_state(const char *path) {
    struct stat st = {};
    if (stat(path, &st) != 0) {
        // The base path doesn't exist (or is not reachable for other reasons),
        // neither as a dir nor as file. Not recoverable.
        return Error {
            .msg = N_("The path does not exist")
        };
    }

    if (S_ISREG(st.st_mode)) {
        // The base path itself is a file, no partial file in play at all, so
        // it's complete.
        return Complete {};
    }

    MutablePath mp(path);
    mp.push(backup_filename);

    auto f = unique_file_ptr(fopen(mp.get(), "r"));
    if (f.get() == nullptr) {
        // Failed to open. This is usually because the download file isn't
        // there (the file is completely downloaded, either as the base path or
        // as a complete "partial" file). But in this case, we better double-check.
        if (errno == ENOENT || errno == ENOTDIR) {
            return Complete {};

        } else if (errno == EBUSY) {
            // Busy - something else might be accessing the file. In that case, we don't throw the error, but report that nothing is available.
            // The function should try again later and when we should be able to get to the file.
            return PartialFile::State {};

        } else {
            return Error {
                .msg = N_("The file disappeared")
            };
        }
    }

    auto transfer = restore(f.get());

    if (!transfer.has_value()) {
        // The download file is there, but can't be read. We _know_ the file is
        // not complete, but not the details.
        return Error {
            .msg = N_("File transfer error")
        };
    }

    return transfer->get_partial_file_state();
}

Transfer::RestoredTransfer::RestoredTransfer(FILE *file, PartialFile::State state, TransferId id)
    : file(file)
    , partial_file_state(state)
    , data_buffer(nullptr)
    , data_buffer_size(0)
    , id(id) {
}

const void *Transfer::RestoredTransfer::get_data_ptr(size_t offset, size_t size) {
    assert(data_buffer != nullptr);
    if (data_buffer == nullptr) {
        log_error(transfers, "No data buffer available");
        return nullptr;
    }

    const auto data_start = sizeof(SerializedTransfer);
    const auto data_end = data_start + data_buffer_size;
    assert(offset >= data_start && offset + size <= data_end);
    if (offset < data_start || offset + size > data_end) {
        log_error(transfers, "Data offset/size out of range");
        return nullptr;
    }

    return data_buffer.get() + offset - data_start;
}

std::optional<Download::Request> Transfer::RestoredTransfer::get_download_request() {
    SerializedTransfer transfer;

    // read the transfer structure
    if (fseek(file, 0, SEEK_SET) != 0 || fread(&transfer, sizeof(transfer), 1, file) != 1) {
        return std::nullopt;
    }

    // get end of data
    if (fseek(file, 0, SEEK_END) != 0) {
        return std::nullopt;
    }
    const FileOffset data_end = ftell(file) - sizeof(EndOfDataCRC);

    // prepare data buffer
    if (!data_buffer) {
        FileOffset data_offset = sizeof(SerializedTransfer);
        const size_t data_length = data_end - data_offset;
        // Check file size is sane, its CRC has not been checked yet
        if (data_length > max_size) {
            log_error(transfers, "Corrupted backup?");
            return std::nullopt;
        }
        data_buffer = std::make_unique<char[]>(data_length);
        data_buffer_size = data_length;

        if (fseek(file, data_offset, SEEK_SET) != 0 || fread(data_buffer.get(), data_length, 1, file) != 1) {
            log_error(transfers, "Failed to read data buffer");
            return std::nullopt;
        }
    }

    // validate data buffer crc
    {
        uint32_t crc = 0;
        uint32_t crc_transfer_start = offsetof(SerializedTransfer, fw_version);

        crc = crc32_calc_ex(crc, ((uint8_t *)&transfer) + crc_transfer_start, sizeof(transfer) - crc_transfer_start);
        crc = crc32_calc_ex(crc, (uint8_t *)data_buffer.get(), data_buffer_size);
        EndOfDataCRC read_crc;
        if (fread(&read_crc, sizeof(read_crc), 1, file) != 1) {
            log_error(transfers, "Failed to read data buffer crc");
            return std::nullopt;
        }
        if (crc != read_crc) {
            log_error(transfers, "Data buffer crc mismatch");
            return std::nullopt;
        }
    }

    auto get_data_ptr = [this](size_t offset, size_t size) -> const void * {
        return this->get_data_ptr(offset, size);
    };

    if (transfer.is_inline) {
        return Download::Request(
            transfer.url_path_or_hash.deserialize(get_data_ptr),
            transfer.team_id,
            transfer.orig_size);
    } else {
        return Download::Request(
            transfer.host.deserialize(get_data_ptr),
            transfer.port,
            transfer.url_path_or_hash.deserialize(get_data_ptr),
            transfer.encryption_info.has_value() ? std::make_unique<Download::EncryptionInfo>(*transfer.encryption_info) : nullptr);
    }
}

Transfer::RecoverySearchResult Transfer::search_transfer_for_recovery(Path &path) {
    // First, check we already have the USB connected, because otherwise we
    // would just assume there's no index file and nothing to recover.
    unique_dir_ptr dir(opendir("/usb"));
    if (!dir) {
        return RecoverySearchResult::WaitingForUSB;
    }
    dir.reset();

    auto index = unique_file_ptr(fopen(transfer_index, "r"));
    if (index.get() == nullptr) {
        return RecoverySearchResult::NothingToRecover;
    }

    for (;;) {
        switch (next_in_index(index, path)) {
        case IndexIter::Ok: {
            const auto r = transfers::transfer_check(path.as_destination(), TransferCheckValidOnly::yes);
            if (r.backup_file_found && r.partial_file_found) {
                return RecoverySearchResult::Success;
            }
            break;
        }
        case IndexIter::IndividualError:
        case IndexIter::Skip:
            continue;
        case IndexIter::Eof:
        case IndexIter::FatalError:
            // TODO: Do we want some kind of "there was an error" indicator?
            // But what would we do with it on the caller side?
            return RecoverySearchResult::NothingToRecover;
        }
    }
}

Transfer::IndexIter Transfer::next_in_index(unique_file_ptr &index, Path &out) {
    if (char *path = fgets(out.get_buffer(), Path::maximum_length(), index.get()); path != nullptr) {
        out.reset_flags();
        // Eat end of line
        size_t len = strlen(path);
        while (len > 0) {
            if (path[len - 1] == '\n' || path[len - 1] == '\r') {
                path[len - 1] = '\0';
                len--;
            } else {
                break;
            }
        }
        if (len == 0) {
            // Skip empty lines
            return IndexIter::Skip;
        }

        struct stat st;
        if (stat(path, &st) == 0) {
            if (!S_ISDIR(st.st_mode)) {
                // Not a dir any more, already cleaned up, skip
                return IndexIter::Skip;
            }
        } else {
            if (errno != ENOENT) {
                return IndexIter::IndividualError;
            } else {
                // Doesn't exist, leftover in index
                return IndexIter::Skip;
            }
        }

        // Vadil "transfer in progress" (potentially)
        return IndexIter::Ok;
    } else {
        return ferror(index.get()) ? IndexIter::FatalError : IndexIter::Eof;
    }
}

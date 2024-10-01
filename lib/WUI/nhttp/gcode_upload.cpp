#include "gcode_upload.h"
#include "handler.h"
#include "splice.h"
#include "../../src/common/filename_type.hpp"
#include "../wui_api.h"

#include <common/stat_retry.hpp>
#include <transfers/files.hpp>
#include <transfers/changed_path.hpp>

#include <sys/stat.h>
#include <cassert>
#include <cstring>
#include <unistd.h>

extern "C" {

// Inject for tests, which are compiled on systems without it in the header.
size_t strlcpy(char *, const char *, size_t);
}

namespace nhttp::printer {

using handler::Continue;
using handler::RequestParser;
using handler::StatusPage;
using handler::Step;
using http::Status;
using splice::Result;
using std::array;
using std::make_tuple;
using std::move;
using std::nullopt;
using std::optional;
using std::string_view;
using std::tuple;
using transfers::ChangedPath;
using transfers::CHECK_FILENAME;
using transfers::Monitor;
using transfers::next_transfer_idx;
using transfers::PartialFile;
using transfers::transfer_name;
using transfers::USB_MOUNT_POINT;
using transfers::USB_MOUNT_POINT_LENGTH;

using Type = ChangedPath::Type;
using Incident = ChangedPath::Incident;

GcodeUpload::GcodeUpload(PutParams &&uploader, Monitor::Slot &&slot, bool json_errors, size_t length, size_t upload_idx, PartialFile::Ptr &&file, UploadedNotify *uploaded)
    : upload(move(uploader))
    , monitor_slot(move(slot))
    , uploaded_notify(uploaded)
    , size_rest(length)
    , json_errors(json_errors)
    , cleanup_temp_file(true)
    , tmp_upload_file(move(file))
    , file_idx(upload_idx)
    , filename_checked(false) {
}

GcodeUpload::GcodeUpload(GcodeUpload &&other)
    : upload(move(other.upload))
    , monitor_slot(move(other.monitor_slot))
    , uploaded_notify(other.uploaded_notify)
    , size_rest(other.size_rest)
    , json_errors(other.json_errors)
    , cleanup_temp_file(other.cleanup_temp_file)
    , tmp_upload_file(move(other.tmp_upload_file))
    , file_idx(other.file_idx)
    , filename_checked(other.filename_checked) {
    // The ownership of the temp file is passed to the new instance.
    other.cleanup_temp_file = false;
}

GcodeUpload &GcodeUpload::operator=(GcodeUpload &&other) {
    upload = move(other.upload);
    uploaded_notify = other.uploaded_notify;
    size_rest = other.size_rest;
    json_errors = other.json_errors;

    cleanup_temp_file = other.cleanup_temp_file;
    // The ownership of the temp file is passed to the new instance.
    other.cleanup_temp_file = false;

    tmp_upload_file = move(other.tmp_upload_file);
    file_idx = other.file_idx;
    filename_checked = other.filename_checked;

    return *this;
}

GcodeUpload::~GcodeUpload() {
    if (cleanup_temp_file) {
        // This is area is entered in the last instance in a move chain. Note
        // that even in that case, the tmp_upload_file may be null (already
        // closed, before attempt to rename it) and the temp file to remove not
        // exist (because it got moved).
        //
        // This code is mostly responsible for cleanup during failure cases
        // (the success case being the above).
        //
        // We still perform the attempt to remove it even in case the file is
        // already closed (and therefore nullptr) because there _is_ a failure
        // case where we closed the file before renaming and failed to rename.
        // Even in that case we want to remove the temp file.
        tmp_upload_file.reset();
        const auto fname = transfer_name(file_idx);
        remove(fname.begin());
    } else {
        // Leftover open file in moved object :-O
        assert(tmp_upload_file.get() == nullptr);
    }
}

GcodeUpload::UploadResult GcodeUpload::start(const RequestParser &parser, UploadedNotify *uploaded, bool json_errors, PutParams &&uploadParams) {
    // Note: authentication already checked by the caller.
    // Note: We return errors with connection-close because we don't know if the client sent part of the data.

    // One day we may want to support chunked and connection-close, but we are not there yet.
    if (!parser.content_length.has_value()) {
        return StatusPage(Status::LengthRequired, StatusPage::CloseHandling::ErrorClose, json_errors);
    }

    if (parser.content_length.value() == 0) {
        // Somehow, we never get our ::step called later on due to that. We
        // could hack around it, but supporting 0-sized files seems like a
        // niche thing to spend the time on. At least report it doesn't work
        // instead of getting into some weird transfer-stuck state that never
        // goes away.
        return StatusPage(Status::NotImplemented, parser, "0-sized files aren't implemented");
    }

    const char *path = uploadParams.filepath.data();
    auto slot = Monitor::instance.allocate(Monitor::Type::Link, path, *parser.content_length, parser.print_after_upload);
    if (!slot.has_value()) {
        // FIXME: Is this the right status to return? Change would need to be
        //  defined in the API spec first.
        return StatusPage(Status::Conflict, parser, "Another transfer in progress");
    }

    const size_t file_idx = next_transfer_idx();
    const auto fname = transfer_name(file_idx);

    auto preallocated = PartialFile::create(fname.begin(), *parser.content_length);
    if (const char **err = get_if<const char *>(&preallocated); err != nullptr) {
        return StatusPage(Status::InsufficientStorage, StatusPage::CloseHandling::ErrorClose, json_errors, nullopt, *err);
    } else {
        return GcodeUpload(move(uploadParams), move(*slot), json_errors, *parser.content_length, file_idx, move(get<PartialFile::Ptr>(preallocated)), uploaded);
    }
}

UploadHooks::Result GcodeUpload::data(string_view data) {
    assert(tmp_upload_file);
    if (tmp_upload_file->write(reinterpret_cast<const uint8_t *>(data.begin()), data.size())) {
        return make_tuple(Status::Ok, nullptr);
    } else {
        // Data won't fit into the flash drive -> Insufficient stogare.
        return make_tuple(Status::InsufficientStorage, "USB write error or USB full");
    }
}

namespace {

    UploadHooks::Result prepend_usb_path(string_view filename, char *filepath_out, size_t filepath_size) {
        if ((filename.length() + USB_MOUNT_POINT_LENGTH) >= filepath_size) {
            // The Request header fields too large is a bit of a stretch...
            return make_tuple(Status::RequestHeaderFieldsTooLarge, "File name too long");
        }
        strlcpy(filepath_out, USB_MOUNT_POINT, USB_MOUNT_POINT_LENGTH + 1);
        strncat(filepath_out, filename.data(), filename.length());
        filepath_out[USB_MOUNT_POINT_LENGTH + filename.length()] = '\0';

        return make_tuple(Status::Ok, nullptr);
    }

    UploadHooks::Result make_dir(string_view dir) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla" // TODO: person who knows a reasonable buffer size should refactor this code to not use variable length array
        char path[USB_MOUNT_POINT_LENGTH + dir.length() + 1];
#pragma GCC diagnostic pop

        auto error = prepend_usb_path(dir, path, sizeof(path));
        if (std::get<0>(error) != Status::Ok) {
            return error;
        }
        if (mkdir(path, 0777) != 0) {
            // teoretically could fail not on the first
            // folder and we would then leave some previous
            // folder in place ,but this is improbable...
            if (errno != EEXIST) {
                return make_tuple(Status::InternalServerError, "Failed to create a folder");
            }
        }
        return make_tuple(Status::Ok, nullptr);
    }

    UploadHooks::Result make_dirs(string_view path) {
        size_t pos = path.find('/');
        while (pos != path.npos) {
            if (auto error = make_dir(path.substr(0, pos)); std::get<0>(error) != Status::Ok) {
                return error;
            }
            pos = path.find('/', pos + 1);
        }

        return make_tuple(Status::Ok, nullptr);
    }

    std::variant<bool, UploadHooks::Result> file_dir_exists(string_view path) {
        size_t pos = path.find_last_of('/');
        if (pos == path.npos) {
            return true;
        }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla" // TODO: person who knows a reasonable buffer size should refactor this code to not use variable length array
        char last_dir[pos + USB_MOUNT_POINT_LENGTH + 1];
#pragma GCC diagnostic pop
        auto error = prepend_usb_path(path.substr(0, pos), last_dir, sizeof(last_dir));
        if (std::get<0>(error) != Status::Ok) {
            return error;
        }
        struct stat st;
        if (stat(last_dir, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                return true;
            } else {
                return make_tuple(Status::Conflict, "Conflicting upload path");
            }
        }
        return false;
    }

    template <class F>
    UploadHooks::Result try_rename(const char *src, const char *dest_fname, bool overwrite, F f) {
        char fn[FILE_NAME_BUFFER_LEN];

        auto error = prepend_usb_path(dest_fname, fn, sizeof(fn));
        if (std::get<0>(error) != Status::Ok) {
            return error;
        }

        struct stat st = {};
        int stat_result = stat_retry(fn, &st);
        if (stat_result == 0 && S_ISREG(st.st_mode) && overwrite) {
            int result = remove(fn);
            if (result == -1) {
                remove(src);
                switch (errno) {
                case EBUSY:
                    return make_tuple(Status::Conflict, "File is busy");
                case EISDIR:
                    // Shouldn't happen due to st_mode already checked?
                    return make_tuple(Status::Conflict, "Is a directory or ongoing transfer");
                default:
                    return make_tuple(Status::InternalServerError, "Unknown error");
                }
            }
        } else if (stat_result == 0 && S_ISDIR(st.st_mode) && overwrite) {
            return make_tuple(Status::Conflict, "Is a directory or ongoing transfer");
        } else if (stat_result == 0) {
            remove(src);
            return make_tuple(Status::Conflict, "File already exists");
        }

        int result = rename(src, fn);
        if (result != 0) {
            // Backup errno before calling other things
            int old_errno = errno;
            remove(src); // No check - no way to handle errors anyway.
            switch (old_errno) {
            case EISDIR:
                return make_tuple(Status::Conflict, "Is a directory or ongoing transfer");
            case ENOTDIR:
                return make_tuple(Status::NotFound, "Destination directory does not exist");
            default:
                // we already checked for existence of the file, so we guess
                // it is weird file name/forbidden chars that contain (422 Unprocessable Entity).
                return make_tuple(Status::UnprocessableEntity, "Filename contains invalid characters");
            }
        } else {
            return f(fn);
        }
    }

    class PutTransfer final : public splice::Transfer {
    public:
        GcodeUpload::UploadedNotify *uploaded_notify = nullptr;
        PartialFile::Ptr f;
        array<char, FILE_PATH_BUFFER_LEN> filepath;
        bool print_after_upload;
        bool overwrite;
        size_t file_idx;

        virtual PartialFile *file() const override {
            return f.get();
        }
        // TODO: alias for the type, probably unify with the UploadHooks::Result
        virtual optional<tuple<http::Status, const char *>> done() override {
            assert(f != nullptr);
            assert(monitor_slot.has_value());
            bool cleanup_temp_file = true;
            const auto fname = transfer_name(file_idx);
            tuple<http::Status, const char *> error { Status::InternalServerError, "Unknown error" };
            switch (result) {
            case Result::Ok: {
                // Remove the "/usb/" prefix
                // FIXME: Why? Relict of old / Post / multipart upload?
                const char *final_filename = filepath.data() + USB_MOUNT_POINT_LENGTH;

                // create directories if uploading points to non existing one
                if (error = make_dirs(final_filename); get<0>(error) != Status::Ok) {
                    goto CLEANUP;
                }
                if (!f->sync()) {
                    error = { Status::InsufficientStorage, "Couldn't sync to file" };
                    goto CLEANUP;
                }

                // Close the file first, otherwise it can't be moved
                f.reset();
                error = try_rename(fname.begin(), final_filename, overwrite, [&](char *filename) -> UploadHooks::Result {
                    monitor_slot->done(Monitor::Outcome::Finished);
                    ChangedPath::instance.changed_path(filename, Type::File, Incident::Created);
                    cleanup_temp_file = false;
                    if (uploaded_notify != nullptr) {
                        if (uploaded_notify(filename, print_after_upload)) {
                            return make_tuple(Status::Ok, nullptr);
                        } else {
                            return make_tuple(Status::Conflict, "Can't print right now");
                        }
                    } else {
                        return make_tuple(Status::Ok, nullptr);
                    }
                });
                break;
            }
            case Result::CantWrite:
                error = { Status::InsufficientStorage, "USB write error or USB full" };
                break;
            case Result::Stopped:
                error = { Status::ServiceTemporarilyUnavailable, "Upload stopped from connect" };
                break;
            case Result::ClosedByClient:
                error = { Status::BadRequest, "Connection closed by client" };
                break;
            case Result::Timeout:
                error = { Status::RequestTimeout, "Connection timeout" };
                break;
            }
        CLEANUP:
            f.reset();
            if (cleanup_temp_file) {
                remove(fname.begin());
            }
            return error;
        }

        virtual tuple<size_t, size_t> write(const uint8_t *in, size_t in_size, uint8_t *out, size_t out_size) override {
            const size_t write_size = std::min(in_size, out_size);
            memcpy(out, in, write_size);
            return make_tuple(write_size, write_size);
        }

        virtual bool progress(size_t len) override {
            assert(monitor_slot.has_value());
            monitor_slot->progress(len);
            return !monitor_slot->is_stopped();
        }
    };

    // TODO: A better place to have this? Or dynamic allocation? Share with the connect uploader?
    PutTransfer put_transfer;
} // namespace

UploadHooks::Result GcodeUpload::check_filename(const char *filename) const {
    if (!filename_is_printable(filename)) {
        return make_tuple(Status::UnsupportedMediaType, "Not a GCODE");
    }

    // Note: If the directory we want to upload to doesn't exist,
    // we assume the filename is OK. If it is not OK for any reason
    // (weird chars in filename or whatever), we just fail at the end of the download
    // which is still fine. Alternativelly we would have to create all those dirs
    // and then delete them, we don't want to do that for simplicity and performance.
    if (auto result = file_dir_exists(filename); holds_alternative<bool>(result)) {
        if (!get<bool>(result)) {
            return make_tuple(Status::Ok, nullptr);
        }
    } else {
        return get<UploadHooks::Result>(result);
    }

    if (upload.overwrite) {
        char filepath[FILE_NAME_BUFFER_LEN];
        auto error = prepend_usb_path(filename, filepath, sizeof(filepath));
        if (std::get<0>(error) != Status::Ok) {
            return error;
        }

        if (wui_is_file_being_printed(filepath)) {
            return make_tuple(Status::Conflict, "File is busy");
        } else {
            return make_tuple(Status::Ok, nullptr);
        }
    }
    // We try to create and then delete the file. This places an empty file
    // there for a really short while, but it's there anyway ‒ something could
    // detect it. Any better way to check if the file name is fine?
    FILE *tmp = fopen(CHECK_FILENAME, "wb");
    if (!tmp) {
        return make_tuple(Status::InternalServerError, "Failed to create a check temp file");
    }
    fclose(tmp);
    return try_rename(CHECK_FILENAME, filename, false, [](const char *fname) -> UploadHooks::Result {
        remove(fname);
        return make_tuple(Status::Ok, nullptr);
    });
}

UploadHooks::Result GcodeUpload::finish(const char *final_filename, bool start_print) {
    const auto fname = transfer_name(file_idx);

    // create directories if uploading points to non existing one
    if (auto error = make_dirs(final_filename); get<0>(error) != Status::Ok) {
        return error;
    }

    // In the "POST" mode (old / legacy), we don't know the exact size in
    // advance, we have only an upper limit. Therefore, the pre-allocated size
    // is a bit bigger and we need to truncate it.
    size_t written = tmp_upload_file->tell();
    if (!tmp_upload_file->sync()) {
        return std::make_tuple(Status::InsufficientStorage, "Couldn't sync to file");
    }
    // Close the file first, otherwise it can't be moved
    tmp_upload_file.reset();
    if (truncate(fname.begin(), written) != 0) {
        return std::make_tuple(Status::InsufficientStorage, "Couldn't set length of file");
    }
    return try_rename(fname.begin(), final_filename, upload.overwrite, [&](char *filename) -> UploadHooks::Result {
        monitor_slot.done(Monitor::Outcome::Finished);
        ChangedPath::instance.changed_path(filename, Type::File, Incident::Created);
        if (uploaded_notify != nullptr) {
            if (uploaded_notify(filename, start_print)) {
                return make_tuple(Status::Ok, nullptr);
            } else {
                return make_tuple(Status::Conflict, "Can't print right now");
            }
        } else {
            return make_tuple(Status::Ok, nullptr);
        }
    });
}

void GcodeUpload::step(string_view input, bool terminated_by_client, uint8_t *, size_t, Step &out) {
    if (terminated_by_client && size_rest > 0) {
        out = Step { 0, 0, StatusPage(Status::BadRequest, StatusPage::CloseHandling::ErrorClose, json_errors, nullopt, "Truncated body") };
        return;
    }

    const size_t read = std::min(input.size(), size_rest);
    if (monitor_slot.is_stopped()) {
        monitor_slot.done(Monitor::Outcome::Stopped);
        out = Step { 0, 0, StatusPage(Status::ServiceTemporarilyUnavailable, StatusPage::CloseHandling::ErrorClose, json_errors, nullopt, "Upload stopped from connect") };
        return;
    }

    // remove the "/usb/" prefix
    const char *filename = upload.filepath.data() + USB_MOUNT_POINT_LENGTH;

    static_cast<void>(input);
    auto filename_error = check_filename(filename);
    if (std::get<0>(filename_error) != Status::Ok) {
        out = Step { read, 0, StatusPage(std::get<0>(filename_error), StatusPage::CloseHandling::ErrorClose, json_errors, nullopt, std::get<1>(filename_error)) };
        return;
    }

    assert(put_transfer.f == nullptr); // No other transfer is happening at the moment.
    put_transfer.f = move(tmp_upload_file);
    tmp_upload_file.reset(); // Does move really set it to nullptr, or is it just a copy?
    put_transfer.set_monitor_slot(move(monitor_slot));
    put_transfer.uploaded_notify = uploaded_notify;
    put_transfer.filepath = upload.filepath;
    put_transfer.print_after_upload = upload.print_after_upload;
    put_transfer.overwrite = upload.overwrite;
    put_transfer.file_idx = file_idx;
    cleanup_temp_file = false;
    out = Step { 0, 0, make_tuple(&put_transfer, size_rest) };
}

} // namespace nhttp::printer

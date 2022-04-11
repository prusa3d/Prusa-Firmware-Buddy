#include "gcode_upload.h"
#include "upload_state.h"
#include "file_info.h"
#include "handler.h"
#include "../../src/common/gcode_filename.h"

#include <cassert>
#include <cstring>

extern "C" {

// Inject for tests, which are compiled on systems without it in the header.
size_t strlcpy(char *, const char *, size_t);
size_t strlcat(char *, const char *, size_t);
}

#define USB_MOUNT_POINT        "/usb/"
#define USB_MOUNT_POINT_LENGTH (strlen(USB_MOUNT_POINT))
// Length of the temporary file name. Due to the template below, we are sure to
// fit.
static const size_t TMP_BUFF_LEN = 20;
static const char *const UPLOAD_TEMPLATE = USB_MOUNT_POINT "%zu.tmp";

namespace nhttp::printer {

using handler::Continue;
using handler::RequestParser;
using handler::StatusPage;
using handler::Step;
using std::get;
using std::make_tuple;
using std::move;
using std::string_view;

void GcodeUpload::FileDeleter::operator()(FILE *f) {
    fclose(f);
}

GcodeUpload::GcodeUpload(UploadState uploader, bool json_errors, size_t length, size_t upload_idx, FilePtr file, UploadedNotify *uploaded)
    : uploader(move(uploader))
    , uploaded_notify(uploaded)
    , size_rest(length)
    , json_errors(json_errors)
    , tmp_upload_file(move(file))
    , file_idx(upload_idx) {
}

GcodeUpload::~GcodeUpload() {
    // The file would close on its own without us doing anything. But we
    // also want to remove it, which we do manually here.
    //
    // Note that the default move constructor/operator works fine, it sets
    // the original to null.
    //
    // It may so happen it's already renamed, in which case the remove simply
    // fails.
    tmp_upload_file.reset();
    char fname[TMP_BUFF_LEN];
    snprintf(fname, sizeof fname, UPLOAD_TEMPLATE, file_idx);
    remove(fname);
}

void GcodeUpload::delete_file() {
}

GcodeUpload::UploadResult GcodeUpload::start(const RequestParser &parser, UploadedNotify *uploaded, bool json_errors) {
    // Note: authentication already checked by the caller.
    // Note: We return errors with connection-close because we don't know if the client sent part of the data.

    const auto boundary = parser.boundary();
    if (boundary.empty()) {
        return StatusPage(Status::BadRequest, false, json_errors, "Missing boundary");
    }

    // One day we may want to support chunked and connection-close, but we are not there yet.
    if (!parser.content_length.has_value()) {
        return StatusPage(Status::LengthRequired, json_errors, false);
    }

    static size_t upload_idx = 0;
    const size_t file_idx = upload_idx++;
    char fname[TMP_BUFF_LEN];
    snprintf(fname, sizeof fname, UPLOAD_TEMPLATE, file_idx);
    FilePtr file(fopen(fname, "wb"));
    if (!file) {
        // Missing USB -> Insufficient storage.
        return StatusPage(Status::InsufficientStorage, false, json_errors, "Missing USB drive");
    }

    char boundary_cstr[boundary.size() + 1];
    memcpy(boundary_cstr, boundary.begin(), boundary.size());
    boundary_cstr[boundary.size()] = '\0';
    UploadState uploader(boundary_cstr);

    if (const auto err = uploader.get_error(); get<0>(err) != Status::Ok) {
        return StatusPage(get<0>(err), false, json_errors, get<1>(err));
    }

    return GcodeUpload(move(uploader), json_errors, *parser.content_length, file_idx, move(file), uploaded);
}

Step GcodeUpload::step(string_view input, bool terminated_by_client, uint8_t *, size_t) {
    uploader.setup(this);
    if (terminated_by_client && size_rest > 0) {
        return { 0, 0, StatusPage(Status::BadRequest, false, json_errors, "Truncated body") };
    }

    const size_t read = std::min(input.size(), size_rest);

    uploader.feed(input.substr(0, read));
    size_rest -= read;

    if (const auto err = uploader.get_error(); get<0>(err) != Status::Ok) {
        return { read, 0, StatusPage(get<0>(err), false, json_errors, get<1>(err)) };
    }

    if (size_rest == 0) {
        // Note: We do connection-close here. We are lazy to pass the
        // can-keep-alive flag around and it's unlikely one would want to reuse
        // the upload connection anyway.
        if (uploader.done()) {
            char filename[FILE_NAME_BUFFER_LEN + 5];
            strcpy(filename, USB_MOUNT_POINT);
            const char *orig_filename = uploader.get_filename();
            strlcpy(filename + USB_MOUNT_POINT_LENGTH, orig_filename, sizeof(filename) - USB_MOUNT_POINT_LENGTH);
            return { read, 0, FileInfo(filename, false, json_errors, true) };
        } else {
            return { read, 0, StatusPage(Status::BadRequest, false, json_errors, "Missing file") };
        }
    } else {
        return { read, 0, Continue() };
    }
}

UploadHooks::Result GcodeUpload::data(std::string_view data) {
    assert(tmp_upload_file);
    const size_t written = fwrite(data.begin(), 1, data.size(), tmp_upload_file.get());
    if (written < data.size()) {
        // Data won't fit into the flash drive -> Insufficient stogare.
        return make_tuple(Status::InsufficientStorage, "USB write error or USB full");
    } else {
        return make_tuple(Status::Ok, nullptr);
    }
}

UploadHooks::Result GcodeUpload::finish(const char *final_filename, bool start_print) {
    const uint32_t fname_length = strlen(final_filename);

    if (!filename_is_gcode(final_filename)) {
        return make_tuple(Status::UnsupportedMediaType, "Not a GCODE");
    }

    char filename[FILE_NAME_BUFFER_LEN];
    if ((fname_length + USB_MOUNT_POINT_LENGTH) >= sizeof(filename)) {
        // The Request header fields too large is a bit of a stretch...
        return make_tuple(Status::RequestHeaderFieldsTooLarge, "File name too long");
    } else {
        strlcpy(filename, USB_MOUNT_POINT, USB_MOUNT_POINT_LENGTH + 1);
        strlcat(filename, final_filename, FILE_PATH_BUFFER_LEN - USB_MOUNT_POINT_LENGTH);
    }

    char fname[TMP_BUFF_LEN];
    snprintf(fname, sizeof fname, UPLOAD_TEMPLATE, file_idx);

    // Close the file first, otherwise it can't be moved
    tmp_upload_file.reset();
    int result = rename(fname, filename);
    if (result != 0) {
        // Most likely the file name already exists and rename refuses to overwrite (409 conflict).
        // It could _also_ be weird file name/forbidden chars that contain (422 Unprocessable Entity).
        // Try to guess which one.
        FILE *attempt = fopen(filename, "r");
        if (attempt) {
            fclose(attempt);
            return make_tuple(Status::Conflict, "File already exists");
        } else {
            return make_tuple(Status::UnprocessableEntity, "Filename contains invalid characters");
        }
    }

    /*
     * TODO: Starting print of the just-uploaded file is temporarily disabled.
     *
     * See https://dev.prusa3d.com/browse/BFW-2300.
     *
     * Once we have time to deal with all the corner-cases, race conditions and
     * collisions caused by that possibility, we will re-enable.
     */
    if (uploaded_notify != nullptr) {
        uploaded_notify(filename, false);
    }

    if (start_print) {
        return make_tuple(Status::NotImplemented, "Starting a print remotely is not yet implemented");
    } else {
        return make_tuple(Status::Ok, nullptr);
    }
}

}

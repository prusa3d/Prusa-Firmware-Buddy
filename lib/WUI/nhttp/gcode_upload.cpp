#include "gcode_upload.h"
#include "upload_state.h"
#include "file_info.h"
#include "handler.h"
#include "gen_once.h"

#include "../wui_api.h"

#include <cassert>
#include <cstring>

namespace nhttp::printer {

using handler::Continue;
using handler::GenOnce;
using handler::RequestParser;
using handler::StatusPage;
using handler::Step;
using std::string_view;

namespace {

    const constexpr UploadHandlers handlers = {
        wui_upload_begin,
        wui_upload_data,
        wui_upload_finish,
    };

}

bool GcodeUpload::have_instance = false;

void GcodeUpload::UploaderDeleter::operator()(Uploader *uploader) {
    uploader_finish(uploader);
}

GcodeUpload::GcodeUpload(UploaderPtr uploader, size_t length)
    : uploader(std::move(uploader))
    , size_rest(length) {
    assert(!have_instance);
    have_instance = true;
}

GcodeUpload::GcodeUpload(GcodeUpload &&other)
    : uploader(std::move(other.uploader))
    , size_rest(other.size_rest) {
    other.holds_instance = false;
}

GcodeUpload &GcodeUpload::operator=(GcodeUpload &&other) {
    uploader = std::move(other.uploader);
    size_rest = other.size_rest;
    other.holds_instance = false;
    return *this;
}

GcodeUpload::~GcodeUpload() {
    if (holds_instance) {
        assert(have_instance);
        have_instance = false;
    }
}

GcodeUpload::UploadResult GcodeUpload::start(const RequestParser &parser) {
    // Note: authentication already checked by the caller.
    // Note: We return errors with connection-close because we don't know if the client sent part of the data.

    if (have_instance) {
        return StatusPage(Status::ServiceTemporarilyUnavailable, false, "Only one upload at a time is possible");
    }

    const auto boundary = parser.boundary();
    if (boundary.empty()) {
        return StatusPage(Status::BadRequest, false, "Missing boundary");
    }

    // One day we may want to support chunked and connection-close, but we are not there yet.
    if (!parser.content_length.has_value()) {
        return StatusPage(Status::LengthRequired, false);
    }

    char boundary_cstr[boundary.size() + 1];
    memcpy(boundary_cstr, boundary.begin(), boundary.size());
    boundary_cstr[boundary.size()] = '\0';
    UploaderPtr uploader(uploader_init(boundary_cstr, &handlers));

    if (!uploader) {
        return StatusPage(Status::ServiceTemporarilyUnavailable, false, "Out of memory");
    }

    if (uint16_t err = uploader_error(uploader.get()); err != 0) {
        return StatusPage(static_cast<Status>(err), false);
    }

    return GcodeUpload(std::move(uploader), *parser.content_length);
}

Step GcodeUpload::step(string_view input, bool terminated_by_client, uint8_t *, size_t) {
    if (terminated_by_client && size_rest > 0) {
        return { 0, 0, StatusPage(Status::BadRequest, false, "Truncated body") };
    }

    const size_t read = std::min(input.size(), size_rest);

    uploader_feed(uploader.get(), input.begin(), read);
    size_rest -= read;

    if (const uint16_t err = uploader_error(uploader.get()); err != 0) {
        return { read, 0, StatusPage(static_cast<Status>(err), false) };
    }

    // Hack: Eventually, we'll fold the uploader into here, so we won't have to
    // throw the filename here and there all the time.
    char filename[FILE_NAME_BUFFER_LEN + 5];
    strcpy(filename, "/usb/");
    uploader_get_filename(uploader.get(), filename + 5);

    if (size_rest == 0) {
        // Note: We do connection-close here. We are lazy to pass the
        // can-keep-alive flag around and it's unlikely one would want to reuse
        // the upload connection anyway.
        if (uploader_finish(uploader.release())) {
            /*
             * FIXME: This passes the file name in a global variable. We shall
             * flatten the Uploader into this class and have access to the file
             * name there and generate the response ourselves.
             */
            return { read, 0, FileInfo(filename, false, true) };
        } else {
            return { read, 0, StatusPage(Status::BadRequest, false, "Missing file") };
        }
    } else {
        return { read, 0, Continue() };
    }
}

}
